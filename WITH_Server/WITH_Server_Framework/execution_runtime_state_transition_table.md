# Execution Runtime State Transition Table

## 1. Node State Set

```cpp
enum class ExecNodeState : uint8_t
{
    NotReady,
    Ready,
    Queued,
    Running,
    Succeeded,
    Failed,
    Canceled,
    Skipped
};
```

---

## 2. Scope State Set

```cpp
enum class ExecScopePhase : uint8_t
{
    Open,
    CancelRequested,
    Draining,
    Closed
};

enum : uint8_t
{
    ExecScopeFlag_None        = 0,
    ExecScopeFlag_HasFailure  = 1u << 0,
    ExecScopeFlag_WasCanceled = 1u << 1,
};
```

---

## 3. Node State Transition Table

### 3.1 Allowed Transitions

| Current State | Next State | Transition Owner | Transition Condition | Note |
|---|---|---|---|---|
| `NotReady` | `Ready` | dependency resolution path | `remainingDeps == 0` and scope is executable | normal ready |
| `NotReady` | `Canceled` | dependency resolution path | `remainingDeps == 0` and scope is canceled and skip is not allowed | canceled before execution |
| `NotReady` | `Skipped` | dependency resolution path | `remainingDeps == 0` and scope is canceled and skip is allowed by policy | skipped before execution |
| `Ready` | `Queued` | dispatcher | ready queue registration succeeded | duplicate enqueue forbidden |
| `Ready` | `Canceled` | cancel/drain path | scope cancel finalized before enqueue and skip is not allowed | lazy cancel |
| `Ready` | `Skipped` | cancel/drain path | scope cancel finalized before enqueue and skip is allowed | lazy skip |
| `Queued` | `Running` | worker | dequeue succeeded and pre-run validation passed | actual execution start |
| `Queued` | `Canceled` | worker | scope canceled after dequeue and before run, skip is not allowed | dequeued but not executed |
| `Queued` | `Skipped` | worker | scope canceled after dequeue and before run, skip is allowed | dequeued but skipped |
| `Running` | `Succeeded` | worker | function returned normally | normal completion |
| `Running` | `Failed` | worker | function failed or threw | failure recorded and may trigger scope cancel |

### 3.2 Forbidden Transitions

| Current State | Forbidden Next State | Reason |
|---|---|---|
| `Succeeded` | all states | terminal |
| `Failed` | all states | terminal |
| `Canceled` | all states | terminal |
| `Skipped` | all states | terminal |
| `Running` | `Canceled` | forced cancellation during execution is forbidden |
| `Running` | `Skipped` | skip after execution start is forbidden |
| `Queued` | `Ready` | reverse transition forbidden |
| `Ready` | `NotReady` | reverse transition forbidden |

### 3.3 Node Transition Rules

- `NotReady -> Ready/Canceled/Skipped`
  - only occurs at the point where the final dependency is resolved
- `Ready -> Queued`
  - only the dispatcher may perform this transition
- `Queued -> Running`
  - only a worker may perform this transition
- `Running -> Succeeded/Failed`
  - only a worker may perform this transition
- scope cancellation does not roll back a running node
- `Canceled` and `Skipped` are both terminal, but their semantics differ
  - `Canceled`: not executed because of cancellation
  - `Skipped`: omitted by policy

---

## 4. Scope State Transition Table

### 4.1 Scope Phase Transitions

| Current Phase | Next Phase | Transition Owner | Transition Condition | Note |
|---|---|---|---|---|
| `Open` | `CancelRequested` | worker / external cancel path | node failure inside scope or external cancel request | new normal execution must stop |
| `Open` | `Closed` | executor | all nodes in the scope reached terminal state normally | normal close |
| `CancelRequested` | `Draining` | executor | cancel request observed and drain sequence started | cleanup phase |
| `CancelRequested` | `Closed` | executor | no remaining nodes require cleanup | fast close allowed |
| `Draining` | `Closed` | executor | all nodes in the scope reached terminal state | canceled close |

### 4.2 Forbidden Scope Phase Transitions

| Current Phase | Forbidden Next Phase | Reason |
|---|---|---|
| `Closed` | all phases | terminal |
| `Draining` | `Open` | reopening forbidden |
| `CancelRequested` | `Open` | cancel withdrawal forbidden |

### 4.3 Scope Flag Transitions

#### `HasFailure`

| Current Value | Next Value | Transition Owner | Condition |
|---|---|---|---|
| `0` | `1` | worker | a node in the scope ended with `Failed` |
| `1` | `1` | retained | additional failures may occur |

#### `WasCanceled`

| Current Value | Next Value | Transition Owner | Condition |
|---|---|---|---|
| `0` | `1` | cancel path / worker | external cancel or propagated cancel due to failure |
| `1` | `1` | retained | reset is not allowed |

---

## 5. Node-State × Scope-State Coupling Rules

- `scopePhase == Open`
  - only in this state may `NotReady -> Ready` occur
- `scopePhase != Open`
  - even if dependencies are fully resolved, the node must not become `Ready`
  - if `AllowSkipOnCancel == true`
    - `NotReady -> Skipped`
  - otherwise
    - `NotReady -> Canceled`
- `Queued` nodes must revalidate the scope phase immediately before execution
  - if still executable
    - `Queued -> Running`
  - otherwise
    - `Queued -> Canceled` or `Queued -> Skipped`
- `Running` nodes are not rolled back by scope cancellation
- when `scopePhase == CancelRequested`
  - new normal execution entries are forbidden

---

## 6. Transition Responsibility Table

| Transition Type | Responsible Owner |
|---|---|
| `NotReady -> Ready/Canceled/Skipped` | dependency resolution path |
| `Ready -> Queued` | dispatcher |
| `Queued -> Running/Canceled/Skipped` | worker |
| `Running -> Succeeded/Failed` | worker |
| `Open -> CancelRequested` | worker or external cancel path |
| `CancelRequested -> Draining/Closed` | executor |
| `Draining -> Closed` | executor |

---

## 7. Atomic Transition Rules

- all node state transitions must use CAS
- `remainingDeps` must be decremented with `fetch_sub`
  - only the thread that changes the count from `1` to `0` may perform the successor-ready transition
- scope phase transitions must be monotonic
  - `Open -> CancelRequested -> Draining -> Closed`
  - reverse transition is forbidden
- scope flags are set-only
  - `HasFailure`: only `0 -> 1`
  - `WasCanceled`: only `0 -> 1`

---

## 8. Successor Resolution Rule

### `ResolveSuccessor(nodeId)`

1. decrement `remainingDeps[nodeId]`
2. if the result is not `0`, stop
3. if the result is `0`, inspect the scope phase
4. if scope phase is `Open`
   - `NotReady -> Ready`
5. if scope phase is not `Open`
   - if `AllowSkipOnCancel == true`
     - `NotReady -> Skipped`
   - otherwise
     - `NotReady -> Canceled`

---

## 9. Worker Dequeue Rule

### `StartExecution(nodeId)`

1. verify the node is in `Queued`
2. re-check the scope phase
3. if scope phase is `Open`
   - `Queued -> Running`
4. if scope phase is not `Open`
   - if `AllowSkipOnCancel == true`
     - `Queued -> Skipped`
   - otherwise
     - `Queued -> Canceled`

---

## 10. Worker Completion Rule

### `CompleteExecution(nodeId)`

#### normal completion
- `Running -> Succeeded`

#### failed completion
- `Running -> Failed`
- for the owning scope
  - set `HasFailure = 1`
  - set `WasCanceled = 1`
  - attempt `Open -> CancelRequested`

---

## 11. Scope Close Condition

A scope must not become `Closed` merely because a cancel request was issued.

### `Closed` transition condition

All conditions below must hold:

- every node in the scope is terminal
  - `Succeeded`
  - `Failed`
  - `Canceled`
  - `Skipped`
- no node remains in `Running`
- no executable node remains in queue

---

## 12. Terminal State Definition

### terminal node states
- `Succeeded`
- `Failed`
- `Canceled`
- `Skipped`

### non-terminal node states
- `NotReady`
- `Ready`
- `Queued`
- `Running`

---

## 13. Summary

### Node State Summary
- `NotReady`
  - after dependency resolution, moves to `Ready`, `Canceled`, or `Skipped`
- `Ready`
  - dispatched to `Queued`
  - may become `Canceled` or `Skipped` before enqueue
- `Queued`
  - worker moves it to `Running`
  - may become `Canceled` or `Skipped` immediately before execution
- `Running`
  - completes as `Succeeded` or `Failed`
- `Succeeded`, `Failed`, `Canceled`, `Skipped`
  - terminal
  - no further transition allowed

### Scope State Summary
- `Open`
  - normal progress
  - on failure or external request, moves to `CancelRequested`
  - on normal completion, may move to `Closed`
- `CancelRequested`
  - new normal execution is forbidden
  - may move to `Draining`
- `Draining`
  - unresolved and queued nodes are cleaned up
  - running nodes are awaited
  - on completion, moves to `Closed`
- `Closed`
  - terminal

### Scope Flag Summary
- `HasFailure`
  - records whether any node in the scope failed
- `WasCanceled`
  - records whether any cancellation request occurred
