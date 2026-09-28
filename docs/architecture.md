# Architecture and tradeoffs

## State machine

The executor has two admission states: open and closed. The queue mutex linearizes submissions and close. A submission admitted before close drains normally; one reaching the mutex after close is rejected. Workers stop when the executor is closed and its queue is empty.

## Ownership

A packaged task owns the user callable and result state. A shared pointer makes its queue wrapper copyable for `std::function`, while the original callable may remain move-only. The caller owns the future. The executor owns every worker thread.

Construction reserves worker storage before starting threads. If thread creation fails partway through, the constructor closes and joins already-started workers before rethrowing. Destruction closes then joins all workers.

## Synchronization

One mutex protects the queue and counters. The condition-variable predicate handles spurious wakeups and shutdown. User code executes outside the mutex. Notifications happen after releasing it.

`accepted = queued + active + completed` holds at each stats snapshot. Rejected counts attempted admissions rejected by capacity or closed state, not allocations that throw before admission. Tasks still running are not part of queue capacity.

## Alternatives considered

A blocking submit can deadlock nested producers and hides overload. Returning an empty optional lets a caller choose retry, shedding or fallback. Work stealing adds complexity and makes FIFO dispatch harder to describe; it is deliberately absent. Cancellation needs a cooperative task contract and is not implied by close.

## Cost model

Queue operations are amortized O(1), protected by one mutex. Each admitted task allocates a packaged task/future state and potentially queue storage. There is no claim of scalability beyond measured workloads; contention should be profiled before sharding or introducing lock-free structures.
