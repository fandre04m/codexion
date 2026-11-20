# Codexion — Learning Notes and Implementation Guide

This is the plain-language companion to CODEXION_WORK_PLAN.md.

- The work plan says what we build and test next.
- These notes explain what a concept means and where we will use it.

Only record implementation details here after the code works and has been tested.

## Current project state

The program parses eight arguments, allocates coder/dongle arrays, initializes
simulation-wide synchronization objects, dongles, and coder elements, and has
a monotonic time helper. It cleans up current resources but does not yet create
threads, schedule dongles, log state messages, or monitor burnout.

## Vocabulary

| Term | Meaning here |
| --- | --- |
| Thread | Independently scheduled work inside one program. Each coder will have one; the monitor will have one too. |
| Shared state | Information more than one thread can access, such as a dongle's availability. |
| Data race | Unsynchronized simultaneous access to shared state where at least one access changes it. |
| Mutex | A lock allowing one thread at a time to inspect/change a defined group of values. |
| Condition variable | An efficient notification channel for threads waiting for state to possibly change. |
| FIFO | First request has priority. |
| EDF | Coder with the earliest burnout deadline has priority. |
| Cooldown | Time after release when a dongle cannot be assigned. |
| Monitor | Separate thread that detects burnout and stops the simulation. |

## 1. Shared state and data races

Threads do not take turns automatically. The OS can pause one thread at any instruction and run another.

    Coder 1 reads: in_use is false
    Coder 2 reads: in_use is false
    Coder 1 writes: in_use is true
    Coder 2 writes: in_use is true

Both coders believe they own the same dongle. That is a data race.

The rule is not “lock everything.” Identify shared data and give it a clear protection rule.

| Shared information | Protection rule | Status |
| --- | --- | --- |
| dongle.in_use | Hold the dongle mutex while reading/changing it | Understood; not coded |
| dongle.ready_at | Hold the dongle mutex while reading/changing it | Understood; not coded |
| Stop state (`end_sim`, reason) | Hold the simulation-state mutex | Understood; not coded |
| Coder deadline / completed-compiles / finished flag | Hold the simulation-state mutex | Understood; not coded |
| Number of finished coders | Hold the simulation-state mutex | Understood; not coded |
| Scheduler heap, requests, grants, any reservations | Hold the simulation-state mutex | Understood; not coded |
| One complete output line | Hold the output mutex | Understood; not coded |

Parsed configuration (`time_compile`, scheduler choice, and similar values)
is written before threads exist and never changes, so it needs no mutex.

The scheduler mutex and dongle mutexes have different jobs:

    state/scheduler mutex -> decides who is entitled to a pair
    dongle mutex           -> protects the physical state of one dongle

In the current header, these are named `sim_mtx` and `dongle_mtx`;
`output_mtx` is for serialized output and `state_changed` is the shared
condition variable. These objects and the per-dongle mutexes are initialized
and cleaned up. Runtime locking and waking behaviour remain to be implemented
and tested; initialization happens before any threads exist.

For a pair assignment, the scheduler takes the state mutex first, then locks
the two required dongles in ascending ID order. It checks entitlement,
priority, cooldown, and physical availability, then updates both dongles
before releasing either decision. No code may acquire these locks in the
opposite direction (dongle first, then state), because that could deadlock.

## 2. Mutexes

A mutex is a key to a small shared notebook. It is separate from the resource described in that notebook.

    Mutex unlocked + in_use false  -> dongle can potentially be claimed
    Mutex unlocked + in_use true   -> dongle is assigned; state can be checked safely
    Mutex locked                   -> one thread is making a protected decision

Basic pattern:

    lock(the_mutex);
    inspect/change data protected by this mutex;
    unlock(the_mutex);

Rules:

- Lock before inspecting/changing values protected by that mutex.
- Unlock as soon as the related decision is complete.
- The thread that locks a mutex also unlocks it.
- Do not hold a resource mutex while compiling, debugging, refactoring, printing, or sleeping.
- Do not return through an error path while holding a mutex.
- If two mutexes must be held together, acquire them in one consistent order.

The existing safe_mutex_handle wrapper reports pthread mutex errors; it does not change these mutex rules.

## 3. Condition variables

Mutexes protect state. Condition variables let a thread sleep until state may have changed instead of busy waiting:

    while (!dongle_is_available)
        ;

A condition variable is a bell, not the condition itself:

    Protected state says whether I may proceed.
    Condition variable says the state may have changed.

Waiting always uses a while loop:

    lock(&mutex);
    while (!can_proceed && !stopped)
        pthread_cond_wait(&condition, &mutex);
    /* mutex is held again; inspect state once more */
    unlock(&mutex);

pthread_cond_wait atomically releases the mutex and blocks the thread. On wake-up it re-locks the mutex before returning. A blocked thread is not runnable, so it uses essentially no CPU.

Use a loop because a wake-up does not promise this thread may proceed: another thread may use the resource first, and POSIX permits spurious wake-ups.

After changing relevant protected state:

    lock(&mutex);
    change protected state;
    pthread_cond_broadcast(&condition);
    unlock(&mutex);

pthread_cond_signal wakes one waiter. pthread_cond_broadcast wakes all waiters on that condition variable. They re-acquire the mutex one at a time and each checks its own rule. Broadcast is useful on shutdown so every blocked coder can exit.

pthread_cond_timedwait adds an absolute deadline. It returns after a signal/broadcast or when the deadline is reached (ETIMEDOUT), and has re-locked the mutex before returning.

### Timed waits and the two clock scales

The simulation uses `CLOCK_MONOTONIC` for its own deadlines, cooldowns, and
elapsed log times. A default `pthread_cond_timedwait`, however, interprets its
absolute `struct timespec` deadline with `CLOCK_REALTIME`. The timestamps are
therefore not interchangeable.

To wait until a monotonic target, first calculate a duration using the
simulation clock, then add only that duration to a fresh real-time reading:

    remaining = monotonic_target - monotonic_now
    wait_deadline = realtime_now + remaining

For example, if a cooldown ends at monotonic 3100 ms and monotonic time is
2500 ms, the remaining wait is 600 ms. Read `CLOCK_REALTIME` and add those
600 ms to produce the absolute deadline passed to `pthread_cond_timedwait`.
When the wait returns—after a broadcast, timeout, or spurious wake-up—the
thread checks the real condition again using monotonic time.

Never pass a `CLOCK_MONOTONIC` timestamp directly to a default timed wait: it
would be interpreted as a real-time calendar timestamp and can time out at the
wrong moment.

A correction to our earlier discussion: if the system clock changes while a
default timed wait is blocked, the wait can still end too early or too late.
Rechecking monotonic time handles an early return but cannot wake a wait delayed
by a backward clock adjustment. We must revisit this limitation when designing
timed waits within the subject's allowed functions.

## 4. Time, burnout, and deadlines

The subject measures burnout from compile start to next compile start:

    deadline = last_compile_start + time_to_burnout

Example:

    compile starts: 100 ms
    compile duration: 200 ms
    burnout duration: 800 ms
    next compile must start by: 900 ms

It must not start by 1100 ms. The compile phase consumes 200 ms of the 800 ms budget.

### Which clock?

clock_gettime can request different clocks. We will use CLOCK_MONOTONIC: it measures real elapsed time from an arbitrary point and does not move backward if the calendar clock is adjusted.

This is not CPU-usage time. CPU time only advances while a thread/process executes, so it barely advances while a coder waits or sleeps. It would be wrong for burnout.

gettimeofday is the permitted calendar/wall-clock option. It is acceptable, but monotonic time is more robust for durations.

### One shared time scale

    start_time = monotonic time just before simulation begins
    now        = current monotonic time when a decision is needed
    elapsed    = now - start_time                 (printed timestamp)
    deadline   = last_compile_start + time_to_burnout
    ready_at   = release_time + dongle_cooldown

At the start, every coder's first deadline is based on start_time. When a coder receives both dongles and is truly about to compile, record last_compile_start immediately before logging “is compiling.”

At a deadline, the monitor and worker must not make contradictory decisions.
The worker must check the current deadline under the shared lock before
recording a new compile start; it cannot renew an already expired deadline
just because the monitor has not run yet. The monitor detects burnout and
records the stop decision.

The monitor exists because a blocked/sleeping worker cannot reliably detect its own deadline. It must announce burnout within the subject's 10 ms allowance.

## 5. Atomic pair assignment and scheduler fairness

### Why hand out a pair together?

If every coder takes one dongle and waits for another, all can become stuck:

    Coder 1 holds left dongle and waits for right.
    Coder 2 holds left dongle and waits for right.
    ...

For multiple coders, the design goal is that a worker never physically holds one dongle indefinitely while waiting for its second. The scheduler makes one controlled decision to grant both required dongles, then the worker logs two acquisitions and starts compiling. The evaluation's one-coder case deliberately takes only the sole dongle and waits for burnout.

If dongle A is ready at 20 ms and B at 50 ms, compiling cannot begin before 50 ms. Taking A early does not help that coder compile earlier, but it can prevent another coder from using A.

### Priority still comes first

The subject requires FIFO/EDF when multiple coders request the same dongle.
Our working design uses one heap per dongle: each heap directly shows its
highest-priority pending requester. The subject does not explicitly specify
the number of heaps; we chose this approach because its per-dongle priority
and atomic pair checks are easier for us to understand.

Suppose coder 2 has priority for dongles 2 and 3. Dongle 2 is free, dongle 3 is unavailable, and coder 1 also wants dongle 2. Under our working strict-priority interpretation, coder 1 cannot borrow dongle 2 ahead of coder 2. This can leave a dongle idle but prevents a lower-priority use from delaying coder 2.

The scheduler can derive priority from the heap heads; whether additional
reservation bookkeeping is needed remains open. A request for an unrelated
pair can still be granted. Strict priority is our interpretation of the
fairness wording, not a proven liveness guarantee: test it with the evaluation
timings and revisit it if necessary.

### Atomic means one protected scheduler decision

“Atomic pair assignment” does not mean two physical objects move at the exact
same nanosecond. It means no other scheduler operation can observe or create a
half-finished assignment.

    wrong:  assign dongle 2 to coder 2
            another scheduler action runs
            assign dongle 3 to coder 2

    right:  while the scheduler mutex is held:
                check priority, readiness, and pair state
                assign the pair state together
            release the scheduler mutex

No multi-coder worker physically holds one dongle while waiting for its partner.
The one-coder evaluation case is a separate exception. If used, a reservation
is scheduler bookkeeping only.

### FIFO, EDF, and the heap

Each waiter has a scheduler request.

- FIFO: smaller request-arrival sequence wins.
- EDF: smaller burnout deadline wins.
- Equal EDF deadline: request sequence, then coder ID.

A coder stores its reusable request fields directly in `t_coder`: `req_order`,
`deadline`, and `req_state`. On submission, it will receive an arrival order
and a burnout-deadline snapshot calculated from protected `last_comp_start`
plus `time_burnout`. Each required dongle's heap stores a pointer to that same
coder. The coder must remain valid and its comparison keys unchanged until both
heap entries are removed.

Each dongle has an embedded two-slot `wait_heap` and a `count` initialized to
zero. Its at-most-two neighboring coders can each have one pending request;
empty slots need not be read. This storage needs no separate allocation. The
one-coder path remains special.

The proposed request states are `NOT_SUBMITTED`, `PENDING`, and `GRANTED`:

    NOT_SUBMITTED -> PENDING -> GRANTED -> NOT_SUBMITTED

`PENDING` means the request is in both heaps; a protected atomic pair grant
removes both references and marks it `GRANTED`. After release, it can return to
`NOT_SUBMITTED` and later be reused. Cancellation and shutdown transitions
still need design. The state and heap operations are protected by `sim_mtx`.

There is no separate scheduler thread in this design. Workers execute shared
scheduler code and wait on `state_changed` when blocked. A broadcast prompts
them to recheck; a cooldown may require a timed wake-up even if nobody else
signals. The exact wake-up and deadline rules still need design and tests.

Fairness applies when requests share a dongle. Coders needing separate pairs
may both proceed. Under our working policy, a lower-priority request cannot
bypass a higher-priority pending request on a shared dongle just because its
other dongle happens to be ready first.

The subject requires a custom priority queue implemented as a binary heap. A heap is an array with a special ordering rule: the current best request sits at index 0.

#### Binary heap refresher

The tree is a visualization of array positions; it needs no parent/child
pointers. A min-heap keeps each parent at least as high in priority as its
children. Siblings and the array as a whole are not necessarily sorted.

    index i: parent = (i - 1) / 2, children = 2 * i + 1 and 2 * i + 2

Insert at the end, then swap upward with the parent while the new item has
higher priority. To remove the root, save it, move the last item to the root,
then swap downward with the higher-priority child until the rule holds. The
scheduler supplies the FIFO/EDF comparison and decides which grants are legal;
the heap only organizes its entries by priority.

## Discussed concepts awaiting implementation

### 6. Cooldown

    ready_at = release_time + dongle_cooldown

So “not in use” and “ready to grant” are different facts. If a coder releases
a dongle at 200 ms and cooldown is 50 ms, then `in_use` becomes false but
`ready_at` is 250 ms. At 220 ms, no coder may receive that dongle.

Treat the related values as independent:

| Fact | Meaning |
| --- | --- |
| in_use | A worker physically has the dongle. |
| ready_at | The earliest time cooldown permits assignment. |
An explicit reservation, if chosen later, would not cancel cooldown.

On release, protected code clears `in_use`, calculates `ready_at`, then notifies
the scheduler. The notification does not bypass cooldown. A timed wait can
sleep until the nearest `ready_at`, or wake sooner after a new request, release,
or stop event.

Later tests will infer release time from compile end and check that no following
acquisition occurs before compile_start + time_to_compile + cooldown.

### 7. Serialized logging

Multiple threads can mix output characters. The initialized output mutex is
intended to make each state message a complete, unmixed line. A first normal-line
logger passed single-thread and basic concurrent tests. The stop check must be coordinated with
printing: checking `end_sim` and releasing its mutex before printing would let
the monitor announce burnout in between, followed by a normal line. The
logger has a `sim_mtx_held` flag: when false, it takes and releases `sim_mtx`;
when true, its caller must hold that mutex and retains it on return. The stop
check applies in both cases. Whenever both mutexes are needed, take `sim_mtx` first and
`output_mtx` second, then release them in reverse order. Holding `sim_mtx`
through the normal stop check and print prevents a burnout decision from
slipping between them. Keep this protected work brief; printing can itself
block, which is a tradeoff of this ordering.

Each coder's messages follow its own action order: two dongle acquisitions,
then compiling, debugging, and refactoring. Other coders' lines may occur
between them. A stop can suppress the remaining normal messages, leaving a
partial sequence.

The monitor must be able to print its authorized burnout line after setting
`end_sim`, while normal lines are suppressed after stop. The flag-based lock
ownership paths passed single-thread checks; a three-thread test exercised normal
logging and stop suppression. Broader stress, output buffering, and monitor
timing remain to be tested.
During development, detailed runtime error propagation is deferred; diagnostics
can help investigate failures. Currently a failed clock read can lead to an
invalid timestamp, and mutex/output failures are not handled. Revisit the
plausible failures and cleanup before evaluation.

### 8. Stopping and cleanup

One burnout or universal completion stops the simulation. The shared state has:

    end_sim
    stop_reason = none | burnout | all_complete | error

The simulation-state mutex protects both values. The first valid thread to set
`end_sim` wins; later threads see it is already true and leave the original
reason unchanged.

The `stop_handler` helper accepts a stop reason and, under `sim_mtx`, records
the first stop and broadcasts `state_changed`. Like the logger, it will have a
flag describing whether its caller already holds the mutex. It returns `true`
when it recorded the stop and `false` when the simulation was already stopped.
Callers should request an actual stop reason, not `STOP_NONE`.

The monitor is the only thread that may announce burnout. While holding
`sim_mtx`, it will confirm the deadline, request `STOP_BURNOUT`, and print the
one authorized `burned out` line only if its request recorded the stop. The
monitor uses a separate burnout-capable output path because normal logging
suppresses messages after stop. A broadcast inside the helper can happen before
the line: waiters cannot recheck the state until the monitor releases `sim_mtx`.
Normal logging checks that same state, so no normal line follows burnout.

Normal completion is counted after a compile phase finishes. Each coder has a
protected completed-compile count and `finished` flag. When a worker reaches
the requirement for the first time, it marks itself finished and increments
the protected `finished_coders` count exactly once. The worker that changes
that count to `n_coders` sets `end_sim` with the `all_complete` reason and
broadcasts. A finished coder no longer requests dongles.

After a stop broadcast, a coder waiting for a pair or in an interruptible
phase wait wakes, checks `end_sim`, releases any assigned pair safely, and
exits. The main thread joins every created worker and the monitor before it
destroys mutexes/conditions or frees allocations.

## Implementation journal

Update this only after an item exists and has been tested.

| Component | What exists now | Test status |
| --- | --- | --- |
| Argument parser | Eight arguments, numeric validation, FIFO/EDF validation | Built; valid/invalid examples checked |
| Initial allocation | Coder/dongle arrays allocated and freed | Normal runs and both injected allocation failures passed Valgrind |
| Time helpers | `current_time_ms` reads the monotonic clock and returns milliseconds, or `-1` on error; callers calculate elapsed time by subtraction | User checked elapsed time; failure path not injected |
| Shared-state declarations | Known state, mutexes, and condition variable declared in the structs | Project builds; runtime synchronization behaviour not yet implemented |
| Initialization/cleanup | Simulation objects tracked by stage; dongle mutexes tracked by count; coder fields and circular references initialized | Simulated initialization failures cleaned up; user reports normal coder-initialization retest had no errors or leaks |
| Logging | Normal-line logger has a flag for caller-held `sim_mtx` | Ownership paths and basic three-thread logging/stop suppression tested; stress, error handling and monitor path remain |
| Stop helper | First stop records its reason and broadcasts; a flag describes mutex ownership | Both ownership paths and competing reasons checked; separate waiting-thread broadcast test passed Valgrind |
| Phase waiting | `request_wait` waits until a monotonic phase deadline or stop, using a realtime condition-variable deadline | User tested normal, zero, already-stopped, and threaded interruption cases; waiter readiness was not synchronized by a handshake |
| Heap scheduler | Coder request fields and two-slot dongle heap storage declared; comparator drafted | Comparator reverses FIFO/EDF due to `strcmp` check and needs correction and tests; heap operations not implemented |
| Coder workers | Not implemented | Not started |
| Burnout monitor | Not implemented | Not started |
| README/test suite | Not implemented | Not started |

## Questions to ask for every concurrent feature

1. Which thread can access this value?
2. Which mutex protects it?
3. Is checking and changing it one indivisible decision?
4. If a thread waits, which state change wakes it?
5. Can a stop request wake it too?
6. What focused test proves the feature works?

