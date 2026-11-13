# Codexion — Learning Notes and Implementation Guide

This is the plain-language companion to CODEXION_WORK_PLAN.md.

- The work plan says what we build and test next.
- These notes explain what a concept means and where we will use it.

Only record implementation details here after the code works and has been tested.

## Current project state

The program currently parses eight arguments, allocates coder/dongle arrays,
and has a monotonic millisecond-time helper. It does not yet create coder
threads, schedule dongles, log state messages, monitor burnout, or fully clean up.

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

At a deadline, the monitor and scheduler must not make contradictory decisions. Either compile start is safely recorded first and creates a new deadline, or burnout is recorded first and stops the simulation. Protection rules make only one result win.

The monitor exists because a blocked/sleeping worker cannot reliably detect its own deadline. It must announce burnout within the subject's 10 ms allowance.

## 5. Atomic pair assignment and scheduler fairness

### Why hand out a pair together?

If every coder takes one dongle and waits for another, all can become stuck:

    Coder 1 holds left dongle and waits for right.
    Coder 2 holds left dongle and waits for right.
    ...

The design goal is that a worker never physically holds one dongle indefinitely while waiting for its second. The scheduler makes one controlled decision to grant both required dongles, then the worker logs two acquisitions and starts compiling.

If dongle A is ready at 20 ms and B at 50 ms, compiling cannot begin before 50 ms. Taking A early does not help that coder compile earlier, but it can prevent another coder from using A.

### Priority still comes first

The subject requires FIFO/EDF when multiple coders request the same dongle.

Suppose coder 2 has priority for dongles 2 and 3. Dongle 2 is free, dongle 3 is unavailable, and coder 1 also wants dongle 2. Coder 1 must not borrow dongle 2 ahead of coder 2 merely because it might return it before dongle 3 is ready. That would make priority depend on luck and could delay the higher-priority coder until burnout.

The scheduler could explicitly mark dongle 2 as reserved for coder 2, or derive
coder 2's priority from pending requests without storing a reservation. Both
approaches remain open until the heap design. Either way, a worker does not
physically hold a dongle while waiting for its partner, and an unrelated pair
can still be granted. The exact fairness interpretation needs testing against
the subject.

This can leave a dongle briefly idle. It is an acceptable tradeoff because predictable per-dongle fairness is a stated requirement.

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

No worker physically holds one dongle while waiting for its partner. If used,
a reservation is scheduler bookkeeping only.

### FIFO, EDF, and the heap

Each waiter has a scheduler request.

- FIFO: smaller request-arrival sequence wins.
- EDF: smaller burnout deadline wins.
- Equal EDF deadline: request sequence, then coder ID.

A request is created when a coder starts trying to obtain its pair. It records
the coder, required dongles, arrival sequence, and current burnout deadline.
That deadline stays fixed while the request waits because the coder has not
started another compile in the meantime.

Fairness applies when requests share a dongle. Coders needing separate pairs
may both proceed. A lower-priority request cannot bypass a higher-priority
request just because its other dongle happens to be ready first.

The subject requires a custom priority queue implemented as a binary heap. A heap is an array with a special ordering rule: the current best request sits at index 0.

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

Multiple threads can mix output characters. One output mutex will make every state message a complete, unmixed line.

### 8. Stopping and cleanup

One burnout or universal completion stops the simulation. The shared state has:

    end_sim
    stop_reason = burnout | all_complete

The simulation-state mutex protects both values. The first valid thread to set
`end_sim` wins; later threads see it is already true and leave the original
reason unchanged.

The monitor is the only thread that may announce burnout. While holding the
state mutex, it confirms that a coder's current deadline has passed, sets the
stop state, prints the one required `burned out` line through the output
serializer, and broadcasts the state-change condition. Normal logging checks
the same stop state before printing, so no normal state line can follow that
burnout line.

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
| Initial allocation | Coder/dongle arrays allocated | Partial: normal cleanup missing |
| Time helpers | `current_time_ms` reads the monotonic clock and returns milliseconds, or `-1` on error; callers calculate elapsed time by subtraction | User checked elapsed time; failure path not injected |
| Initialization/cleanup | Not implemented | Not started |
| Logging | Not implemented | Not started |
| Heap scheduler | Not implemented | Not started |
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

