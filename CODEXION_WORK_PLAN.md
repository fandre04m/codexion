# Codexion — Working Plan

> Living document. Keep completed items checked, revise an item before changing
> its scope, and delete stale notes instead of letting this become a history log.
>
> Use `CODEXION_LEARNING_NOTES.md` alongside this plan for plain-language
> explanations and the implementation journal.

## Current snapshot — 2026-11-09

The project compiles cleanly with `-Wall -Wextra -Werror -pthread` and the
parser rejects a zero coder count and an unknown scheduler. The successful
command below currently exits with status 0 and prints no simulation output:

```sh
./codexion 5 800 100 100 100 5 50 fifo
```

`codexion_subject.md` is currently untracked, and `later_info.txt` is shown as
deleted by Git. Those pre-existing repository changes are not part of this
plan; do not restore, remove, or commit them accidentally while working on the
implementation.

## Requirement audit

| Area | Status | Evidence / gap |
| --- | --- | --- |
| C / compiler flags / required Make rules | Partial | C and strict flags are present. `all`, `clean`, `fclean`, and `re` exist, but `.PHONY` says `al` rather than `all`. |
| No global variables | Met so far | State is carried through `t_sim`; preserve this rule. |
| Exact eight required arguments | Met | `main` requires `ac == 9`; parser recognizes seven numeric fields plus `fifo`/`edf`. |
| Robust argument validation | Partial | Negative, malformed, overflow, zero coders, and bad schedulers are rejected. Decide and document intended handling for zero values in the other numeric fields. |
| Simulation data ownership | Partial | Arrays are allocated, but dongles/coders are not initialized and failure/success cleanup is missing. `init_data()` failure is ignored by `main`. |
| One thread per coder | Missing | No worker entry point or thread creation in the flow. |
| Separate burnout monitor | Missing | No monitor, deadline tracking, or coordinated stop exists. |
| Two-dongle acquisition / one-coder case | Missing | Dongle pointers and individual state exist only as unused fields. |
| Mandatory cooldown | Missing | `ready_at` exists but is never set, checked, or waited on. |
| FIFO and EDF arbitration with custom heap | Missing | Scheduler name is parsed, but no queue/heap, request sequence, EDF key, or deterministic tie-breaker exists. |
| Deadlock / starvation prevention | Missing | No acquisition protocol exists yet. |
| Serialized exact state logs | Missing | There is no elapsed-time function, output mutex, or state logger. Colour escape codes must never appear in required state lines. |
| Stop on death or all required compiles | Missing | `end_sim` is not protected, read, or set. |
| Cleanup and partial-init safety | Missing | No joins, mutex/condition destruction, or frees after normal init. |
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
  grants access only when both required dongles are available. A priority
  higher-priority request may wait for its partner, but no worker physically holds only one
  dongle; no other scheduler action can interleave with the pair decision.
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
- [ ] Encode the agreed shared-state fields and lock-ownership interface in the
  header without starting threads or scheduling dongles.

**Finalized locking approach:** coders submit pair requests to one scheduler
queue. The state/scheduler mutex protects queue order, any reservations, grants,
coder state, and stop state; its condition variable wakes threads after a
meaningful state change. A scheduler action takes that mutex, then locks the
needed dongles in ascending ID order before inspecting/changing their physical
state (`in_use`, `ready_at`). It grants a pair in one decision, so a worker
never physically holds only one dongle while waiting for the other.

**Exit checkpoint:** you can describe, in your own words, (1) what a mutex and
a condition variable do here, (2) why one-at-a-time acquisition can deadlock,
(3) how FIFO differs from EDF, and (4) how the monitor can safely stop every
thread. The plan records the agreed invariants before any struct/code change.

### 2. Build safe initialization and teardown

- [ ] Zero-initialize `t_sim` in `main` before parsing.
- [ ] Make every allocation/init return status and check it in `main`.
- [ ] Initialize the output mutex, simulation/scheduler mutex and condition,
  then every dongle mutex, before creating threads.
- [ ] Fill coder IDs, compile counters, `sim` pointers, and circular left/right
  dongle references. For one coder, represent exactly one dongle and handle
  that special case deliberately.
- [ ] Add a single cleanup function that can safely unwind each partially
  initialized stage, then destroys initialized synchronization objects and
  frees both arrays.
- [ ] Correct `.PHONY` to list `all`.

**Exit checkpoint:** a small temporary initialization test (or a normal run
with thread creation still disabled) succeeds and all allocation/init failures
have one cleanup path.

### 3. Implement logging and lifecycle primitives

- [ ] Add `log_state(coder, message)` that locks one output mutex, checks the
  stop rule, and prints exactly one required line with elapsed milliseconds.
- [ ] Keep errors on `stderr`; state lines go to `stdout` and contain no colour
  codes or extra wording.
- [ ] Add helpers to atomically query/request a stop and to notify all waiters.
- [ ] Implement interruptible phase waiting (compile, debug, refactor) so a
  detected burnout ends workers promptly.

**Exit checkpoint:** a temporary single-thread path produces complete,
non-interleaved state lines with timestamps measured from one shared start.

### 4. Implement the scheduler and cooldown

- [ ] Implement the required custom binary heap: storage, compare, push, peek,
  pop, and removal/cancellation of a stopped request. Do not use a library
  priority queue.
- [ ] Assign a strictly increasing request sequence while holding the scheduler
  lock; calculate the EDF deadline from protected coder state.
- [ ] Dispatch only an entitled request whose required dongles are both available
  and past `ready_at`; grant both atomically, then wake its coder. Allow
  unrelated eligible pairs to proceed even when a higher-priority pair waits.
- [ ] On release, set each dongle's `ready_at` to release time plus cooldown,
  update any scheduler bookkeeping, notify waiters, and prevent early grants.
- [ ] Ensure two `has taken a dongle` lines precede every `is compiling` line.
- [ ] Specify the one-coder path: it can take the sole dongle but cannot obtain
  a second one, so the monitor must report burnout without a deadlock.

**Exit checkpoint:** focused runs demonstrate FIFO request order, EDF priority
order, deterministic equal-deadline order, and a measurable cooldown delay.

### 5. Implement workers, monitor, and completion

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

## Decision log

- 2026-11-09 — No implementation files were changed during the initial audit.
  The next coding task is design/state ownership followed by initialization and
  teardown; writing worker threads before that would duplicate synchronization
  decisions.
