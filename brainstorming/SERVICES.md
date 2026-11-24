# SERVICES.md

## 1. Purpose

This document describes the role of services in QueueOS, including how they expose functionality, interact with the queue execution model, and integrate with the operating system.

A service is an OS-managed provider of functionality that can be requested by applications or other OS components.

Services are an important part of QueueOS, but they must not introduce a second, competing application-to-OS interface. The queue-centric execution model and the application-facing ABI remain the common foundation.

This document establishes architectural expectations without prematurely defining implementation details such as worker processes, threads, scheduling algorithms, or queue persistence.

## 2. Architectural role

Services provide capabilities that should be available independently of any particular application.

Examples might include:

- Filesystem and storage operations.
- Networking.
- Authentication and identity.
- Device management.
- Notifications.
- Time and clock management.
- Printing.
- Audio and video.
- Desktop and graphical environment functionality.
- Application discovery and lifecycle management.
- System configuration and administration.

These are examples of potential services, not a finalized list of mandatory components.

A service may be implemented as part of the native OS environment, use a shared runtime, or depend on other services. Its implementation language should not determine how clients interact with it.

## 3. Services and queues

### 3.1 Queue-based interaction

Services should expose their functionality through the QueueOS execution model.

A client submits a request through the appropriate queue interface. The service processes the request and makes the result available through the defined completion mechanism.

The intended interaction is conceptually:

1. A client identifies the service and operation it requires.
2. The client submits a request.
3. The OS makes the request available for execution.
4. The service processes the request.
5. The result or error is returned to the client.

The precise queue topology, dispatch mechanism, and ownership of individual queues remain open design questions.

### 3.2 Common interaction model

Services should follow common conventions for:

- Request identification.
- Operation identification.
- Input and output data.
- Completion and error reporting.
- Cancellation where supported.
- Rights verification.
- Version and compatibility information.

The ABI should define the common mechanisms. Individual services define their own operations and data structures.

A service should not require clients to understand its internal implementation.

### 3.3 Synchronous and asynchronous operations

The queue architecture should support both synchronous and asynchronous usage patterns.

A client may need an immediate result, or it may submit work and continue executing while the service processes the request.

The distinction should be part of the service interface rather than an implementation-specific requirement imposed on the client.

The exact semantics of synchronous calls, completion notifications, timeouts, and cancellation remain to be defined.

## 4. Service identity and discovery

Services need stable identities that clients can use to locate and interact with them.

A service identity should be independent of:

- Its implementation language.
- Its physical memory location.
- Its internal execution mechanism.
- The CPU architecture on which it runs.

The system should provide a consistent way to discover available services and determine which operations they expose.

Discovery should be capable of communicating relevant information such as:

- Service identity.
- Supported interface version.
- Available operations.
- Availability state.
- Required Rights.
- Dependencies, where relevant.

The exact naming scheme, registry structure, and discovery API remain unresolved.

## 5. Service lifecycle

Services require a managed lifecycle.

At a conceptual level, a service may move through states such as:

- Defined: known to the OS but not initialized.
- Starting: initialization is in progress.
- Available: ready to accept requests.
- Degraded: operating with reduced functionality.
- Stopping: no longer accepting new work.
- Stopped: not currently running.
- Failed: unable to provide its functionality.

These are proposed logical states. The final state machine and transition rules have not been established.

The OS should be responsible for managing service availability and coordinating lifecycle transitions.

### 5.1 Startup

Some services will be necessary before the system can become operational. Others may be started only when needed.

Startup dependencies must be considered explicitly. For example, a service that depends on persistent storage cannot be fully initialized before the relevant storage functionality is available.

The system should avoid unnecessary startup dependencies that make the OS difficult to recover or operate in a reduced environment.

### 5.2 On-demand activation

Services that are not required continuously may be activated when requested.

This could reduce resource consumption and avoid maintaining unnecessary background activity.

Whether activation is handled by a dedicated service manager, the queue dispatcher, or another OS component remains undecided.

### 5.3 Shutdown and restart

Services should have defined shutdown behavior.

Where practical, the OS should allow a service to stop accepting new requests, finish or cancel outstanding work, release resources, and report its final state.

Restarting a service should not silently invalidate unrelated client state or leave requests permanently unresolved.

The exact recovery and restart guarantees will depend on the service and its operations.

## 6. Service dependencies

Services may depend on other services.

Dependencies should be declared or otherwise discoverable so that the OS can manage initialization and availability.

The design should account for:

- Required dependencies.
- Optional dependencies.
- Dependency failures.
- Circular dependencies.
- Startup ordering.
- Runtime loss of a dependency.
- Recovery after a dependency becomes available again.

A service should distinguish between a required dependency and an optional enhancement.

The failure of an optional dependency should not necessarily make the entire service unavailable.

Dependency management should avoid creating a rigid startup chain in which one nonessential component prevents the rest of the OS from operating.

## 7. Services and Rights

Services are part of the OS security boundary.

A client requesting an operation must have the appropriate Rights, and the service must not assume that the caller is authorized merely because the request reached its queue.

The service interface should make authorization requirements explicit.

Examples include:

- Reading or modifying files.
- Accessing network resources.
- Managing devices.
- Changing system configuration.
- Starting or stopping other services.
- Accessing another application's data.

The distinction between Features and Rights remains fundamental:

- Features describe functionality an application or service provides.
- Rights describe what that application or service is permitted to do.

A service may expose many Features while requiring different Rights for individual operations.

Rights enforcement should be consistent with `SECURITY.md` and the application-facing ABI.

The exact representation, propagation, and verification of Rights remain open design questions.

## 8. Services and the OS Database

The OS Database is the authoritative persistent representation of the system's required state.

OS components, including service-related components, may be stored in the OS Database and loaded directly from it.

The distinction between an OS component and an installed application must remain clear:

- OS components belong to the operating system and may be necessary for its operation.
- Installed applications exist outside the OS Database.
- Applications may consume service functionality without becoming part of the OS itself.

The OS Database may contain service definitions, configuration, component metadata, or other information necessary to initialize services.

However, not all service state necessarily belongs in persistent storage.

Transient operational state, such as active requests and temporary resource allocations, may be maintained in memory.

The persistence requirements of each service should be determined by its function.

## 9. Built-in and optional services

Not every service needs to be part of the minimum operational system.

It is useful to distinguish between:

**Core services**

Services required for the OS to reach or maintain its basic operational state.

**System services**

Services that provide general operating system functionality but may not be required during the earliest boot stages.

**Optional services**

Services that provide additional capabilities and may be installed, enabled, or activated independently.

These categories are conceptual. The final classification and service inventory remain to be established.

The initial system should be kept as small as practical, with optional functionality added without requiring changes to the foundational execution model.

## 10. Service isolation and implementation

A service should have a defined interface and controlled access to resources.

Its internal implementation should be replaceable without requiring clients to change, provided the service continues to satisfy its published interface contract.

Possible implementation models include:

- Native OS code.
- A shared runtime.
- A dedicated runtime instance.
- A separately isolated execution context.
- A combination of native and interpreted components.

No single execution model is assumed for all services.

In particular, this document does not establish that every service must run as a conventional process or thread.

The relationship between services, execution contexts, queues, and scheduling must be designed together with the execution architecture.

## 11. Failure handling

Services must be designed with failure in mind.

The OS should be able to distinguish between:

- A failed request.
- An unavailable service.
- A service that is temporarily degraded.
- A service that has stopped.
- A service that has failed unexpectedly.

Clients should receive meaningful completion or error information rather than waiting indefinitely for an operation that cannot complete.

The architecture should also consider:

- Requests outstanding when a service fails.
- Requests submitted during service restart.
- Duplicate requests after a client retries.
- Partial completion of multi-step operations.
- Recovery of service-owned state.

The appropriate recovery behavior will differ between services. A read-only query, a filesystem modification, and a device-control operation cannot necessarily share identical retry semantics.

The OS should not imply that an operation is safe to retry unless the relevant service contract guarantees it.

## 12. Performance and resource management

Services must fit into the broader QueueOS performance model.

The design should avoid unnecessary:

- Data copying.
- Serialization and deserialization.
- Context transitions.
- Memory allocation.
- Queue traversal.
- Synchronization.
- Runtime startup overhead.

The queue architecture should allow efficient communication between clients and services while maintaining the required isolation and security guarantees.

Service resource usage should be observable and, where appropriate, constrained.

Potential resource considerations include:

- Memory consumption.
- CPU time.
- Outstanding requests.
- Queue capacity.
- Open resources.
- Network and storage bandwidth.

The appropriate limits and accounting mechanisms remain unresolved.

Performance decisions should be supported by benchmarks rather than assumptions about which execution model is inherently fastest.

## 13. Observability and diagnostics

Services should provide enough operational information for the OS to diagnose failures and understand resource usage.

Useful diagnostic information may include:

- Current service state.
- Initialization and shutdown results.
- Request and completion counts.
- Error counts.
- Resource consumption.
- Dependency availability.
- Restart and recovery events.

Diagnostics must respect the security model. Operational visibility should not automatically grant access to application data or other sensitive service state.

The logging and monitoring architecture should be consistent across native components and runtime-based services.

## 14. Service versioning and compatibility

Service interfaces must be versioned independently of their internal implementations.

An implementation update should not unnecessarily break existing clients.

The service contract should identify its supported interface version and define how incompatible changes are handled.

The system should consider:

- Backward-compatible additions.
- Deprecated operations.
- Incompatible interface changes.
- Runtime and ABI compatibility.
- Service replacement during an OS update.

Service versioning should align with the ABI and OS Database generation model.

## 15. Open design questions

The following decisions remain open:

1. What is the exact service identity and naming scheme?
2. How are services registered and discovered?
3. Is there a dedicated service manager, or is service lifecycle management distributed among OS components?
4. How are service queues created, assigned, and managed?
5. How does the scheduler interact with service execution?
6. What are the exact service lifecycle states and transition rules?
7. How are dependencies declared and resolved?
8. Which services are mandatory for the minimum operational OS?
9. Which services should support on-demand activation?
10. How are service failures and outstanding requests handled?
11. What isolation guarantees apply to different service types?
12. How are service Rights represented and enforced?
13. Which service configuration and state belong in the OS Database?
14. How are service updates coordinated with OS Database generations?
15. What diagnostic and resource-accounting interfaces should be standardized?

These questions should be resolved incrementally as the execution, ABI, security, and database designs mature.

## 16. Current architectural commitments

The following principles are established or strongly intended:

- Services provide reusable OS functionality to applications and other OS components.
- Queue-based interaction is the common foundation for service requests.
- The application-facing ABI remains the common interface.
- Individual services define their own operations and data structures.
- Service implementations should be replaceable without unnecessarily affecting clients.
- Services must enforce the applicable Rights.
- The OS manages service availability and lifecycle.
- OS components remain distinct from installed applications.
- Service implementation details must not force a conventional process or thread model onto QueueOS.
- Performance and isolation decisions must be evaluated together.

The exact service manager, registry, queue topology, lifecycle implementation, and execution model remain open.
