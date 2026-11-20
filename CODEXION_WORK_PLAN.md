# Codexion — Working Plan

> Living document. Keep completed items checked, revise an item before changing
> its scope, and delete stale notes instead of letting this become a history log.
>
> Use `CODEXION_LEARNING_NOTES.md` alongside this plan for plain-language
> explanations and the implementation journal.

## Current snapshot — 2026-11-20

The current program parses arguments, initializes the simulation and its
coders and dongles, then cleans up and exits. It has a monotonic millisecond
helper, logger, stop helper, and interruptible phase wait, but does not start
the simulation threads. The next step is Phase 4: design and develop the
scheduler. An example invocation is:

```sh
./codexion 5 800 100 100 100 5 50 fifo
```

The initialization and failure-path test results recorded in Phase 2 are
from the earlier work session; they have not been rerun in this documentation
update.

## Requirement audit

| Area | Status | Evidence / gap |
| --- | --- | --- |
| C / compiler flags / required Make rules | Met so far | C, strict flags, the required Make rules, and `.PHONY: all` are present. |
| No global variables | Met so far | State is carried through `t_sim`; preserve this rule. |
| Exact eight required arguments | Met | `main` requires `ac == 9`; parser recognizes seven numeric fields plus `fifo`/`edf`. |
| Robust argument validation | Partial | Negative, malformed, overflow, zero coders, and bad schedulers are rejected. Other numeric fields currently accept zero; decide whether that is the intended policy. |
| Simulation data ownership | Partial | Arrays and elements are initialized and freed; `main` checks initialization. Thread ownership and joins remain. |
| One thread per coder | Missing | No worker entry point or thread creation in the flow. |
| Separate burnout monitor | Missing | No monitor, deadline tracking, or coordinated stop exists. |
| Two-dongle acquisition / one-coder case | Missing | Dongle pointers and individual state exist only as unused fields. |
| Mandatory cooldown | Missing | `ready_at` is initialized to zero but is not updated on release, checked, or waited on. |
| FIFO and EDF arbitration with custom heap | Missing | Scheduler name is parsed, but no queue/heap, request sequence, EDF key, or deterministic tie-breaker exists. |
| Deadlock / starvation prevention | Missing | No acquisition protocol exists yet. |
| Serialized exact state logs | Partial | The normal-line logger passed a single-thread formatting, elapsed-time, and stop-suppression test; concurrency, monitor, and error handling remain incomplete. |
| Stop on death or all required compiles | Missing | `end_sim` and its reason are initialized, but no runtime stop protocol exists. |
| Cleanup and partial-init safety | Partial | Stage/count-based cleanup covers current resources; recorded injected failure tests passed. Thread joins and their failure paths remain. |
| README required content | Missing | It has the required opening line and base headings, but Description is empty; usage, blocking cases, and synchronization sections are absent/incomplete. |

## Implementation order

### 1. Understand the rules and build the small foundations

This is a learning checkpoint, not a demand to know the design already. We
will discuss one item at a time, add the agreed rule here in plain language,
and only then write its smallest supporting code and test it. Do not add worker
or scheduler behaviour until every conceptual box is checked and its
explanation makes sense.

- [x] **Why shared state needs protection.** We understand that two threads
  reading/changing the same shared value without coordination can race and
  produce unreliable behaviour.
- [x] **Map shared state to protection.** Final map: the state/scheduler mutex
  protects stop state, coder deadlines/completion state, the finished-coder
  count, scheduler heap/requests/grants, and any reservations. Each dongle mutex
  protects its own `in_use` and `ready_at` physical state. Parsed settings are
  immutable after startup and need no mutex. Scheduler actions lock state
  first, then their dongles in ascending ID order; no code takes those locks
  in reverse order.
- [x] **Mutexes.** Confirmed the basic rule: lock before examining or changing
  the data it protects; unlock immediately after the related decision is
  complete. A mutex is a key to a tiny shared notebook, not a general-purpose
  pause button.
- [x] **Condition variables.** Understood their job: a coder sleeps while a
  required condition is false (for example, its pair is not available) and is
  woken to check again when the state changes. The condition is always checked
  in a loop while holding its mutex.
- [x] **Time and deadlines.** Agreed on one elapsed monotonic millisecond
  clock using `clock_gettime(CLOCK_MONOTONIC, ...)`. `last_compile_start` is
  recorded when both dongles are exclusively assigned, immediately before
  `is compiling`; its deadline is that time plus `time_to_burnout`. The monitor
  compares this deadline with the clock. For a default `pthread_cond_timedwait`,
  calculate remaining time with the monotonic clock, then add that duration to
  a fresh `CLOCK_REALTIME` value to form its required absolute deadline; never
  pass a monotonic timestamp directly to that wait.
- [x] **Atomic pair assignment.** Understood: a scheduler-protected decision
  grants access only when both required dongles are available. A higher-priority
  request may wait for its partner, but a multi-coder worker does not hold only
  one dongle while waiting; no scheduler action can interleave with the pair
  decision. The evaluation's single-coder case is a separate exception.
- [x] **Cooldown.** Understood: releasing a dongle sets
  `ready_at = release_time + dongle_cooldown`; it cannot be assigned before
  that time even when `in_use` is false. A request may wait through the
  cooldown, and timed waiting can avoid busy polling.
- [x] **Fairness policies.** Understood the priority keys: FIFO uses request
  arrival sequence; EDF uses earlier burnout deadline. Equal EDF deadlines use
  arrival sequence, then coder ID, so runs stay deterministic.
- [x] **Partially available pairs.** Working fairness interpretation: a lower
  priority request cannot bypass a higher-priority request on a shared dongle,
  while unrelated pairs can proceed. Choose between explicit reservations and
  deriving entitlement from pending requests when designing the heap; test both
  contention and unrelated progress against the subject's wording.
- [x] **Stopping.** Agreed protocol: one protected stop state has a reason
  (`burnout` or `all_complete`), and the first valid stop decision wins. The
  monitor alone announces burnout; the worker that changes the protected
  finished-coder count to `n_coders` requests normal completion. A broadcast
  wakes every blocked worker, which releases any assigned pair safely and
  exits. Normal logs check this state, so none can follow a burnout line.

**Foundation-code checkpoint:** the Phase 1 design boxes are complete. Before
coding, discuss any remaining questions; then implement and test the smallest
reusable pieces that follow directly from them: the monotonic millisecond-time
helper and the agreed state/locking interface in the header. This is
deliberately not worker, monitor, heap, or resource-grant code; those remain
in later phases where their full prerequisites are initialized and testable.

- [x] Implement and test the monotonic millisecond-time helper, including
  elapsed time from a saved start timestamp. The user tested elapsed time;
  clock-read failure returns `-1` but has not been deliberately injected.
- [x] Declare the currently agreed shared-state and synchronization fields in
  the header; document lock ownership in the learning notes. Scheduler and
  monitor-specific fields remain for their later design checkpoints. These
  declarations do not themselves enforce the locking protocol; the current
  synchronization objects are initialized in Phase 2.

**Working locking approach:** the state/scheduler mutex protects scheduling
decisions, coder state, and stop state; its condition variable wakes waiters.
Pair grants use that mutex, then the needed dongle mutexes in ascending ID
order. Per-dongle heaps and strict priority for pending requests are the current
working direction; validate fairness, unrelated progress, and EDF liveness
against the subject and evaluation timings as the design develops.

**Exit checkpoint — complete:** you can describe, in your own words, (1) what a mutex and
a condition variable do here, (2) why one-at-a-time acquisition can deadlock,
(3) how FIFO differs from EDF, and (4) how the monitor can safely stop every
thread. The plan records the working invariants and remaining scheduler questions.

### 2. Build safe initialization and teardown

- [x] Zero-initialize `t_sim` in `main` before parsing.
- [x] Check the existing allocation/init status in `main`; check future steps too.
- [x] Initialize the output mutex, simulation mutex and condition before threads.
- [x] Initialize every dongle's fields and mutex, tracking how many mutexes
  succeeded. Normal one- and five-dongle runs passed Valgrind; a simulated
  third-mutex failure exercised partial cleanup.
- [x] Fill coder IDs, compile counters, `sim` pointers, and circular left/right
  dongle references. The one-coder acquisition special case remains for the
  scheduler phase.
- [x] Exercise simulated failures of the simulation mutexes, condition, both
  allocations, and a dongle mutex. Cleanup uses the recorded stage/count and
  frees the arrays; Valgrind reported no memory errors in the injected runs.

**Exit checkpoint — complete:** normal initialization and the recorded injected
allocation/init failures were exercised with thread creation still disabled.

### 3. Implement logging and lifecycle primitives

**Normal logging:** `log_state` has a `sim_mtx_held` flag: it takes and
releases `sim_mtx` only when the caller does not already hold it. It checks
`end_sim`, then uses `output_mtx` for printing. Lock order is `sim_mtx` then
`output_mtx`; a caller-held `sim_mtx` remains held on return. Both flag paths
and stop suppression were checked in single-thread tests.

Normal logging checks the stop state while `sim_mtx` remains held through
printing. The monitor's authorized burnout line must still be printable after
it sets the stop state; the exact interface for distinguishing it from normal
messages remains to be designed. Monitor timing and output blocking/buffering
need further design and testing. During development, detailed runtime error
propagation is deferred: existing diagnostics help investigation, but a clock
failure currently allows an invalid timestamp and lock/output failures are not
handled. Revisit plausible failures and cleanup before evaluation.

- [x] Create the normal-message `log_state` function with a `sim_mtx_held`
  flag, stop check, elapsed timestamp, and serialized output.
- [x] Test `log_state` with `sim_mtx_held` both false and true, including
  stop suppression and the caller retaining its lock in the true case.
- [x] Test normal-line formatting and elapsed time, and suppress a line after
  stop in a temporary single-thread path; Valgrind reported no errors or leaks.
- [x] Stop helper: implement a shared
  stop request for burnout, completion, and runtime errors. Under `sim_mtx`,
  the first request sets `end_sim` and `stop_reason` and broadcasts
  `state_changed`; later requests leave the original state unchanged. A lock
  ownership flag follows the logger's caller-held contract.
- [x] Test stop requests with both lock-ownership paths and competing reasons;
  confirm only the first request reports a new stop. A separate threaded test
  confirmed a blocked waiter wakes; Valgrind reported no errors or leaks.
- [x] Implement interruptible phase waiting (compile, debug, refactor).
  User tested 200 ms and zero-duration waits, an already-stopped simulation,
  and a 1000 ms threaded wait interrupted after about 100 ms. The threaded
  test used a sleep rather than a handshake to establish waiter readiness.
- [x] Exercise concurrent normal logging and suppression after a stop. The
  user's three-thread test produced complete lines before stop, interrupted
  waits, and no normal line from the thread started afterward. Broader stress
  testing and burnout-specific ordering remain for later validation.

**Exit checkpoint — complete:** the stop helper and interruptible waits are tested, and
normal logging tests cover formatting, serialization, and stop suppression.
Burnout-output ordering and worker action order are tested in Phase 5.

### 4. Implement the scheduler and cooldown

**Working design:** each coder owns one reusable request; each dongle has a
heap containing references to its pending requesters. A request stores its coder,
FIFO arrival sequence, and EDF deadline snapshot; the comparison depends on
the selected policy. A request state distinguishes not submitted, pending,
and granted. Scheduler operations are shared code executed by workers under
`sim_mtx`, not a separate scheduler thread. The exact C fields and interfaces
are to be discussed as each step is reached.

The strict per-dongle rule—grant only when the request is first for both
dongles—is a working interpretation, not a proven consequence of the subject.
Keep unrelated pairs eligible and test the policy against the evaluation timings
and EDF liveness. Whether additional reservation bookkeeping is needed remains
open; priority must not be lost merely because a request waits for its pair.

#### 4.1 Requests and data ownership

- [ ] Discuss and declare the request record, its coder reference, fixed
  arrival/deadline keys, and `NOT_SUBMITTED`, `PENDING`, `GRANTED` states.
- [ ] Decide how each per-dongle heap stores request references, its capacity,
  and how queues are initialized and cleaned up. Account for partial failures.
- [ ] Specify the state transitions, lock ownership, shutdown/cancellation,
  and lifetime: do not reuse a request while either heap still refers to it.

**Checkpoint:** explain the ownership and lifecycle using one coder and two
dongles; the declarations and initialization build and clean up safely.

#### 4.2 Priority and heap operations

- [ ] Implement and test one comparator: FIFO by original request sequence;
  EDF by stored burnout deadline, then sequence and coder ID as tie-breakers.
  Assign the sequence and snapshot the deadline under `sim_mtx` on submission.
- [ ] Implement the custom array-backed heap operations needed by the design
  (including push, peek, pop, and removal/cancellation where required).
- [ ] Test empty, single-entry, insertion, removal, FIFO, EDF, and equal-deadline
  cases. Verify priority keys stay unchanged while requests are in the heaps.

**Checkpoint:** focused heap tests pass before scheduler grants are added.

#### 4.3 Submit, wait, and grant pairs

- [ ] Insert one request into both required dongle heaps as a coordinated
  operation. Decide rollback behavior if insertion cannot be completed.
- [ ] Check that the request heads both heaps, both distinct dongles are free
  and past cooldown, and the simulation is still running. Coordinate deadline
  checks with the monitor so an expired coder cannot start a new compile.
- [ ] Under `sim_mtx` and the relevant dongle mutexes in ascending ID order,
  remove both heap references, grant both dongles, and mark the request granted
  as one decision. Wake waiters after relevant state changes.
- [ ] Design worker condition waits and cooldown timed waits so a blocked worker
  rechecks on changes and an appropriate waiter wakes when cooldown ends even
  without another event. Establish the clock and timeout limitations.
- [ ] Test shared-dongle ordering, an unrelated pair progressing while another
  waits, later EDF arrivals, stop while pending, and safe removal from both heaps.

**Checkpoint:** focused scheduler tests show correct grants and no stuck waiters;
the full worker and monitor integration remains Phase 5.

#### 4.4 Release, cooldown, and single coder

- [ ] Release both granted dongles safely, set `ready_at` from release time plus
  cooldown, update request state, and notify waiters. Test zero and nonzero
  cooldown and ensure no early reuse.
- [ ] Design the evaluation's single-coder exception: take and log the sole
  dongle, never treat it as a pair or lock it twice, then wait for monitor
  burnout and clean up. Verify the final behavior during Phase 5 integration.

**Exit checkpoint:** focused tests demonstrate FIFO/EDF priority, deterministic
ties, unrelated progress, correct atomic grants, cancellation, and cooldown.
The evaluation's no-burnout timings and one-coder burnout are validated once
the worker and monitor exist; their results may require revisiting the policy.

### 5. Implement workers, monitor, and completion

- [ ] Design and test the monitor's authorized burnout output: while holding
  `sim_mtx`, confirm burnout, request the stop, and print only if that request
  recorded it. This path works after `end_sim` is true and coordinates with
  normal logging; test that no normal message follows the burnout line.
- [ ] Start all coder threads and the separate monitor only after recording a
  common simulation start time and initializing every coder deadline.
- [ ] Worker loop: request pair → log both acquisitions → record compile start
  → log/perform compile → release pair → log/perform debug → log/perform
  refactor → repeat.
- [ ] Increment a coder's completed-compile count under the chosen state lock;
  when every coder reaches `number_of_compiles_required`, request a normal
  stop exactly once.
- [ ] Make the monitor check every non-finished coder often enough that the
  burnout message is emitted within 10 ms of its deadline. It must serialize
  the death decision and log so no normal state line follows the death line.
- [ ] Join every successfully-created coder and the monitor, including stop and
  thread-creation failure paths, before cleanup.

**Exit checkpoint:** feasible runs finish due to the required compile count;
infeasible runs show one burnout line within the timing allowance and terminate
without hanging.

### 6. Validate, then document

- [ ] Review error handling across the project before evaluation, including
  clock, pthread, allocation, output, propagation, shutdown, and cleanup paths.
  In particular, review callers of `current_time_ms()` that do not check `-1`,
  and how a realtime-clock failure in `request_wait()` reaches the simulation.
- [ ] Update `README.md` in English with a real Description, exact build/run
  syntax, an example for each scheduler, and accurate Resources/AI disclosure.
- [ ] Add the required **Blocking cases handled** section: deadlock/Coffman
  avoidance, starvation/fairness, cooldown, monitor precision, and log
  serialization.
- [ ] Add the required **Thread synchronization mechanisms** section: each
  mutex/condition, the scheduler queue, coder/monitor communication, and how
  races are prevented.
- [ ] Run `make fclean && make`, then all tests below. Run the applicable
  Norm checker before final submission.
- [ ] Build/run a sanitizer variant outside the submission flags when available
  to check leaks, use-after-free, and undefined behaviour; restore the normal
  build afterward.

## Test matrix

Run each case repeatedly (not just once); capture output and exit status.

| Purpose | Example command | Expected result |
| --- | --- | --- |
| Normal FIFO completion | `./codexion 5 800 100 100 100 3 50 fifo` | Every coder reaches 3 compiles; no burnout; program exits. |
| Normal EDF completion | `./codexion 5 800 100 100 100 3 50 edf` | Same completion property, EDF queue policy used. |
| One coder | `./codexion 1 200 100 100 100 1 0 fifo` | Cannot get two dongles; exactly one timely burnout, no hang. |
| Intentional burnout | `./codexion 5 100 100 100 100 10 0 fifo` | One burnout line no later than roughly 10 ms after its deadline. |
| Cooldown | `./codexion 2 1000 50 0 0 2 200 fifo` | Reused dongles are not taken until at least 200 ms after release. |
| FIFO contention | Repeated short-phase FIFO run | Shared-dongle grants follow request arrival order. |
| EDF contention | Use unequal `last_compile_start` values via phase timing | Earlier deadline wins; equal deadlines use the stated deterministic tie-breaker. |
| Zero values policy | `./codexion 2 0 0 0 0 0 0 fifo` | Matches the documented validation decision; never spins or hangs. |
| Bad count | `./codexion 1 2 3` | Non-zero exit and no allocation/thread work. |
| Bad numeric fields | Replace an argument with `-1`, `abc`, `12x`, or `2147483648` | Non-zero exit, clear error, no crash. |
| Bad scheduler | `./codexion 2 800 100 100 100 1 0 round_robin` | Non-zero exit and no simulation. |
| Stress | Repeat a feasible 50+ coder run for FIFO and EDF | No deadlock, starvation, crash, or interleaved logs. |

## Working rules for us

- Treat `codexion_subject.md` as the source of truth over this document if a
  wording conflict appears; update this plan immediately after resolving it.
- Complete one exit checkpoint before moving to the next section.
- Keep commits focused by phase (design/init, scheduler, lifecycle, docs/tests).
- Add a short dated note below only for a decision that changes implementation;
  remove notes that no longer affect future work.

