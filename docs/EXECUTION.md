# QueueOS Execution Model

## 1. Purpose

This document describes the execution model of QueueOS: how applications and operating-system components interact, how work is submitted and processed, and how the system coordinates concurrent activity.

The central architectural principle is that **queues are the primary interface through which applications interact with the operating system**.

Rather than making conventional system calls the defining application-to-OS boundary, QueueOS is being designed around the submission, processing, and completion of work through queues.

The exact queue implementation and scheduling mechanisms remain open design questions. This document establishes the intended direction without prematurely committing to particular data structures or execution strategies.

## 2. Design Principles

The execution model should follow these principles:

- **Queue-centric:** Queues provide the common interface for submitting work and receiving results.
- **Consistent plumbing:** The OS provides common queue infrastructure while individual services define the semantics of the operations they support.
- **Minimal overhead:** Avoid unnecessary context switches, data copying, serialization, and intermediate layers.
- **Concurrency:** Independent work should be able to proceed concurrently where resources permit.
- **Controlled access:** Queue access is governed by the application's granted Rights.
- **Architecture independence:** Application-facing execution interfaces should remain consistent across supported CPU architectures.
- **Explicit lifecycle:** Work, queues, and their associated resources should have well-defined lifecycles.
- **Recoverability:** Failures should not leave the system in an ambiguous or inconsistent state.

The execution model should not assume that conventional processes, threads, or system calls must remain the fundamental abstractions. They may be useful implementation mechanisms, but their role should follow from the architecture rather than dictate it.

## 3. The Queue-Centric Model

Applications interact with OS functionality by submitting work to queues.

A queue provides a defined point of interaction between a producer of work and the component responsible for processing it.

For example, an application might submit requests relating to:

- File and object storage.
- Database operations.
- Network communication.
- Desktop and window management.
- Notifications and events.
- Device access.
- Other OS-provided services.

The receiving component processes the request according to the semantics of its service and makes the result available through the appropriate queue interface.

The OS should provide consistent underlying mechanisms for these interactions without requiring every service to implement its own unrelated communication infrastructure.

### 3.1 Services define their own semantics

Queue infrastructure and service functionality are separate concerns.

The queue system should provide the common mechanisms needed to submit, receive, and complete work. Individual services define:

- Which operations are supported.
- What information an operation requires.
- What constitutes successful completion.
- What errors can be returned.
- Whether an operation produces additional events.
- What resources or Rights are required.

This allows different OS services to use the same underlying execution model without forcing them into an unnecessarily rigid interface.

### 3.2 Queue endpoints

A queue may have one or more producers and consumers, depending on its intended use.

The architecture must eventually define how queue endpoints are identified, discovered, accessed, and associated with their owners.

It should not assume that every queue has exactly one producer and one consumer.

The precise endpoint model remains open.

## 4. Work and Completion

The fundamental unit of execution is a submitted operation or request.

Conceptually, an operation passes through the following stages:

1. A producer submits work to a queue.
2. The queue makes the work available for processing.
3. An appropriate service or execution component accepts the work.
4. The operation is processed.
5. Completion, failure, or an intermediate result is communicated to the appropriate recipient.

This is a logical description, not a commitment to a particular number of internal transitions.

For example, submission and processing might occur within the same execution context when that is efficient. The architecture should not require a thread switch, process transition, or data copy merely to preserve an abstract separation between these stages.

### 4.1 Request identity

The system will need a way to associate completion information with the original request.

A request identifier is one possible mechanism. Other approaches may be appropriate depending on the final queue representation.

The mechanism should support multiple outstanding operations without requiring consumers to rely on submission order alone.

### 4.2 Results and errors

Operations need a consistent way to communicate their outcomes.

At a minimum, the model should distinguish:

- Successful completion.
- Successful completion with returned data.
- Failure.
- Cancellation.
- Operations that remain pending.

Individual services may define more specific result types.

Error information should be sufficiently structured to allow applications to respond appropriately without depending on implementation-specific internal details.

## 5. Synchronous and Asynchronous Operations

The queue model should accommodate both synchronous and asynchronous use.

A synchronous operation allows the caller to wait for its result before continuing. An asynchronous operation allows the caller to continue performing other work while the operation is processed.

These should be understood as different ways of consuming the same underlying execution facilities, rather than necessarily requiring separate service implementations.

The exact interface remains undecided.

Areas requiring further design include:

- Whether synchronous operations are implemented as wrappers around asynchronous requests.
- How waiting is represented.
- Whether completion can be delivered through another queue.
- How cancellation interacts with an operation already in progress.
- How timeouts are represented and enforced.

The architecture should avoid making asynchronous operation unnecessarily difficult or imposing blocking behaviour on services that can operate concurrently.

## 6. Scheduling and Dispatch

Queues and scheduling are related, but they are not the same thing.

Queues represent pending work and its associated interaction model. Scheduling determines when and where that work is executed.

The OS will need a mechanism to select work, allocate execution resources, and dispatch operations to the appropriate service components.

The scheduler should account for:

- Work availability.
- Resource availability.
- Service requirements.
- Priority, where applicable.
- Fairness between competing workloads.
- Responsiveness and latency.
- System-level work that must remain available even under heavy application load.

The scheduler should not assume that one queue corresponds to one thread, process, or dedicated worker.

A single execution component may service multiple queues, and a queue may be serviced by different execution resources over time, if the chosen implementation supports this.

### 6.1 Scheduling policy

The scheduling policy has not yet been established.

Possible considerations include priority levels, weighted scheduling, deadlines, fairness, and service-specific scheduling requirements.

The design should avoid introducing a complex general-purpose scheduling policy before there is a demonstrated need for it.

### 6.2 Dispatch

Dispatch is the mechanism by which available work is assigned to an execution component.

The implementation should minimize unnecessary transitions and make efficient use of the underlying hardware.

Whether dispatch is performed by a central scheduler, distributed service-level schedulers, or a combination remains open.

## 7. Concurrency and Ordering

QueueOS must support concurrent operations without making correctness dependent on accidental execution order.

The queue model needs to define the ordering guarantees provided to producers and consumers.

Potential guarantees include:

- Submission order.
- Completion order.
- Per-producer ordering.
- Explicit dependencies between operations.
- No ordering guarantee unless explicitly requested.

These guarantees should be selected according to actual service requirements rather than imposing a single global ordering rule.

For example, independent file operations may be processed concurrently, while a sequence of related database operations may require defined ordering.

### 7.1 Synchronization

The execution model will require synchronization mechanisms for shared resources and coordinated operations.

These mechanisms should be designed to avoid unnecessary contention and should not force all queue activity through a single global lock.

The precise synchronization primitives and memory-ordering requirements remain implementation decisions.

### 7.2 Backpressure

A producer must not be able to submit work indefinitely without regard to the receiving service's ability to process it.

The queue infrastructure needs a way to deal with sustained differences between submission and processing rates.

Possible mechanisms include:

- Bounded queues.
- Submission limits.
- Producer notification.
- Temporary rejection of new work.
- Resource-aware admission control.

The final design should make overload behaviour explicit rather than allowing uncontrolled memory or resource consumption.

## 8. Cancellation and Timeouts

Some operations may no longer be needed by the time they are processed.

The execution model should provide a defined approach to cancellation and timeouts.

Cancellation may have different meanings depending on the state of an operation:

- Not yet accepted.
- Accepted but not started.
- Currently executing.
- Completed, but the result has not yet been collected.

An operation that has already produced an external side effect may not be reversible. Cancellation must not imply rollback unless the service explicitly provides that guarantee.

Timeouts should similarly distinguish between a caller no longer being willing to wait and the underlying operation actually being terminated.

## 9. Events and Notifications

Not all communication with an application is necessarily the result of an explicit request.

OS services may also need to notify applications about events.

Examples include:

- A file or object becoming available.
- A network connection changing state.
- A device being connected or disconnected.
- A desktop event occurring.
- A background operation completing.
- A service reporting a change in state.

The queue model should accommodate both request/response interactions and event delivery.

An event is conceptually different from a completion result: it may be generated independently of any particular application request.

The exact event subscription, delivery, filtering, and retention mechanisms remain open.

## 10. Resource Management

Execution consumes resources, including CPU time, memory, queue capacity, and service-specific resources.

The OS must provide mechanisms to prevent one application or service from exhausting resources needed by others.

Resource management should be connected to the application authorization model.

An application's granted Rights determine which OS facilities it may access. Resource limits and scheduling policies determine how permitted work competes for available resources.

These are related but distinct concerns:

- **Rights** determine whether an application is authorized to perform an operation.
- **Resource management** determines whether and when the system can accommodate that operation.

The queue infrastructure should make it possible to enforce these constraints without requiring every service to independently recreate the entire authorization and resource-management model.

## 11. Security and Queue Access

Queues are an important part of the OS security boundary.

The system must establish:

- Who owns a queue.
- Which applications may submit work.
- Which components may consume work.
- Which applications may receive results or events.
- Which Rights are required for particular operations.
- How access is revoked when an application or service is stopped or uninstalled.

Possession of a queue reference should not automatically grant unrestricted access to every operation supported by the receiving service.

Authorization must be checked at the appropriate boundary, and access to returned data must be protected as well as access to the original request.

The implementation should also consider queue exhaustion, malformed requests, unauthorized submissions, and attempts to interfere with another application's work.

## 12. Data Transfer and Memory

Queue operations will often involve data larger than the request metadata itself.

The execution model should avoid unnecessary copying, particularly for large objects and high-throughput operations.

Possible approaches include:

- Direct references to managed buffers.
- Shared-memory regions.
- Buffer ownership transfer.
- Reference-counted objects.
- Explicit data transfer where isolation requires it.

These are candidate techniques, not settled implementation choices.

The final approach must balance performance, memory safety, isolation, and architecture independence.

A queue should not require every payload to be serialized and copied simply because it crosses a logical service boundary.

Where serialization or copying is necessary, the reason should be explicit.

## 13. Queue Persistence and Recovery

Not every queue necessarily needs to be persistent.

Transient queues may be appropriate for short-lived interactions, notifications, and other operations whose loss during shutdown is acceptable.

Other operations may need to survive service restarts or system interruptions.

The architecture should distinguish between:

- Transient work.
- Work that can be safely retried.
- Work whose completion must be recorded.
- Durable work that must survive a restart.

The OS Database provides the persistent storage foundation for QueueOS, but this does not imply that every queue must be backed by a database table or that every request must be written to persistent storage.

Durability should be a deliberate property of an operation or service.

Recovery semantics must also account for operations that may have completed externally before a failure prevented their completion from being recorded.

The system must not assume that blindly retrying every interrupted operation is safe.

## 14. Relationship to the ABI

QueueOS requires a stable application-facing ABI across supported CPU architectures.

The queue interface is a central part of that contract.

The ABI will need to define the externally visible representation of queue operations, including the conventions required to submit work and receive results.

It should avoid exposing architecture-specific implementation details unnecessarily.

The ABI should describe the contract between applications and the OS, not dictate how queues are implemented internally.

Internal implementations may differ between architectures as long as they preserve the agreed external behaviour.

## 15. Relationship to Other OS Components

The execution model provides common infrastructure for OS services.

It is not intended to replace the functionality of those services.

For example:

- The database component defines database operations and their semantics.
- The storage component defines storage operations and their semantics.
- The networking component defines network operations and their semantics.
- The desktop component defines desktop and window-management operations and their semantics.

These components use the queue infrastructure to interact with applications and other OS components.

This separation allows the OS to maintain a consistent execution model while individual components evolve independently.

## 16. Open Design Questions

The following questions require further investigation before the execution model can be considered fully specified.

### Queue representation

- Are queues kernel-managed objects, shared-memory structures, or another abstraction?
- What is the minimum metadata required for an operation?
- How are queue references represented and validated?
- Can queues be composed, linked, or multiplexed?

### Ownership and lifecycle

- Who creates queues?
- Can applications create their own queues?
- How are queues discovered and published?
- What happens to pending work when a queue is closed?
- How are queues associated with application and service lifecycles?

### Execution

- What is the relationship between queues, execution contexts, and workers?
- Is there a central scheduler, distributed scheduling, or a hybrid?
- How are priorities and resource limits represented?
- What guarantees are provided for ordering and fairness?

### Communication

- How are synchronous operations represented?
- How are completion results delivered?
- How are events subscribed to and delivered?
- What are the cancellation and timeout semantics?

### Persistence

- Which classes of operations need durability?
- How are interrupted operations recovered?
- How are duplicate submissions and safe retries handled?

### Performance

- Which operations can share memory directly?
- What synchronization mechanisms are appropriate?
- How should queue capacity and backpressure be managed?
- Which optimizations are justified by measured workloads?

These questions should be resolved incrementally, with prototypes and benchmarks where appropriate.

## 17. Current Direction and Commitments

The following distinctions are important when extending this document.

### Established direction

- Queues are the primary application-to-OS interface.
- Queue infrastructure should be consistent across OS services.
- Individual services define their own operation semantics.
- The execution model should support concurrent work.
- The application-facing interface should be compatible with the stable, architecture-independent ABI.
- Application Rights govern access to OS functionality.
- Unnecessary copying, serialization, and execution transitions should be avoided.

### Not yet established

- The physical representation of queues.
- Whether queues are kernel objects or implemented elsewhere.
- Whether queues are bounded or unbounded.
- The relationship between queues and threads, workers, or processes.
- The scheduling and dispatch algorithms.
- Priority and fairness policies.
- Exact synchronous and asynchronous interfaces.
- Event subscription and delivery mechanisms.
- Queue persistence and recovery semantics.
- The specific memory-sharing and synchronization mechanisms.

Implementation decisions should preserve the established direction while treating the unresolved details as design work, not as existing requirements.

---

**Status:** Architectural direction established; execution mechanisms under design.

**Related documents:**
- `ARCHITECTURE.md`
- `DATABASE.md`
- `STORAGE.md`
- `BOOT.md`
- `AGENTS.md`
