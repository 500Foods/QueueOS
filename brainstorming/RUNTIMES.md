# QueueOS Runtime Architecture

## 1. Purpose

This document describes the runtime architecture of QueueOS and how application code is executed.

QueueOS should support applications written in different programming languages and execution environments without requiring each language to have its own independent interface to the operating system.

The common application ABI provides the boundary between applications and OS functionality. Runtimes operate above that boundary, providing language-specific execution environments and abstractions.

The architecture should accommodate:

- Native compiled applications.
- Interpreted applications.
- Managed execution environments.
- Shared language runtimes.
- Application-specific runtime components.

Lua is one runtime being considered for QueueOS, but the architecture should not make the entire OS dependent on one language or execution environment.

## 2. Design Principles

The runtime architecture should follow these principles:

- **Common ABI:** All application runtimes ultimately interact with the OS through the same application-facing ABI.
- **Language neutrality:** The OS should not require applications to use a particular programming language.
- **Native efficiency:** Applications should be able to execute native code without unnecessary runtime overhead.
- **Shared infrastructure:** Common runtimes should not need to be duplicated in every application package.
- **Controlled dependencies:** Applications should declare the runtime capabilities and versions they require.
- **Isolation:** Runtime environments must respect the same application authorization and resource boundaries as native applications.
- **Interoperability:** Different languages and runtimes should be able to use compatible OS services and exchange data through defined interfaces.
- **Architecture independence:** Runtime-facing OS interfaces should remain consistent across supported CPU architectures.
- **Measured optimization:** Runtime choices should be evaluated using actual workloads rather than assumptions about language performance.

## 3. Runtime Layers

The runtime architecture can be understood as three conceptual layers.

### 3.1 Native execution

Native applications execute machine code directly on the target CPU.

They use the QueueOS ABI to interact with the operating system.

Native execution provides the opportunity for maximum performance and direct control over application implementation.

It does not imply unrestricted access to OS internals or hardware resources.

Native applications remain subject to the same authorization, resource-management, and isolation requirements as other applications.

### 3.2 Language runtimes

A language runtime provides the execution environment required by applications written in a particular language.

Examples include:

- Lua.
- Other interpreted languages.
- Managed language environments.
- Application-specific execution engines.

A runtime may provide facilities such as:

- Code loading.
- Memory management.
- Language-level data structures.
- Garbage collection, where applicable.
- Exception handling.
- Standard libraries.
- Module loading.
- Runtime-specific debugging.

These are runtime responsibilities, not necessarily OS responsibilities.

### 3.3 Application frameworks

An application framework provides higher-level facilities built on top of a runtime or directly on the native ABI.

Examples might include:

- User-interface frameworks.
- Networking libraries.
- Database libraries.
- Application service bindings.
- Common application components.

Frameworks should not become independent, competing interfaces to the OS.

They should use the common ABI and preserve the OS security model.

## 4. Native Execution

Native execution is a fundamental part of QueueOS.

Applications compiled for a supported CPU architecture should be able to execute directly using the platform's native instruction set.

Native applications interact with QueueOS through the application ABI.

The ABI should provide a consistent contract regardless of the underlying CPU architecture.

### 4.1 Executable format

ELF has been considered as a possible executable format.

This is separate from the ABI itself.

QueueOS may use ELF as an executable container while providing its own loader behaviour, application startup contract, and OS interface.

Linux ABI compatibility is not an established requirement.

The executable format and loader remain open design decisions.

### 4.2 Native libraries

Applications may depend on shared native libraries.

The runtime and application package architecture should provide a way to identify, locate, and validate these dependencies.

Shared libraries should be reusable without forcing every application to include a private copy.

Library compatibility and versioning must be managed explicitly.

The exact linking model remains undecided.

## 5. Lua Runtime

Lua is one of the candidate language environments for QueueOS.

Its relatively small implementation, embeddability, and suitability for extending native applications make it worth considering as a shared OS runtime.

Lua could serve several purposes:

- General application development.
- Application scripting.
- Configuration and automation.
- Service extensions.
- Higher-level OS tooling.
- Application-specific logic.

The intention is not necessarily to implement the entire OS in Lua.

Native C remains the preferred implementation language for foundational system functionality where direct control, performance, and low-level integration are important.

### 5.1 Shared runtime

A shared Lua runtime could provide a common execution environment for multiple applications.

Applications would declare the Lua runtime version or capabilities they require rather than bundling an independent interpreter by default.

This could reduce duplication and simplify runtime maintenance.

The precise runtime lifecycle and sharing model remain open.

### 5.2 Lua and the ABI

Lua applications should not require a separate OS interface.

The runtime should expose QueueOS ABI functionality through Lua bindings.

For example, an application might use a Lua API to:

- Access an authorized service.
- Submit a queue operation.
- Receive a completion.
- Subscribe to an event.
- Access permitted storage.

The Lua bindings translate language-level operations into the common ABI contract.

They should not bypass authorization or introduce a separate permission system.

### 5.3 Native extensions

Lua may need native extensions for performance-sensitive operations or access to specialized functionality.

These extensions must respect the application's execution and authorization boundaries.

An extension should not automatically receive greater authority than the application that loads it.

The exact native module format and loading policy remain undecided.

## 6. Runtime Distribution and Dependencies

Runtimes may be provided by the OS or installed as independently managed components.

The application package model should allow applications to declare their runtime requirements.

For example, an application might require:

- A specific runtime.
- A minimum runtime version.
- A particular language feature.
- An optional runtime module.

The OS should determine whether the required environment is available before attempting execution.

### 6.1 Shared runtimes

A shared runtime avoids duplicating the same interpreter or execution engine across application packages.

This is particularly attractive for small systems where storage consumption and memory usage matter.

However, sharing a runtime introduces compatibility and lifecycle considerations.

Updating a shared runtime must not unexpectedly break installed applications.

### 6.2 Application-specific runtimes

Some applications may require a private runtime or a particular runtime version.

The architecture should allow this where necessary without making application-specific bundling the default.

The trade-off between shared and private runtime instances should be based on compatibility, isolation, and resource requirements.

### 6.3 Runtime versioning

Runtimes require explicit version identification.

Applications should declare the versions or capabilities they require.

The OS should avoid replacing a runtime with an incompatible version while applications still depend on the previous contract.

The exact versioning and side-by-side installation strategy remains open.

## 7. Memory Management

Different runtimes may have different memory-management requirements.

Native applications may manage memory directly or use libraries that provide their own allocation mechanisms.

Lua and other managed environments may provide automatic memory management.

The OS must still account for the memory consumed by every application and runtime.

Runtime-managed memory does not exempt an application from OS resource limits.

The architecture should also consider:

- Memory allocation overhead.
- Garbage-collection pauses.
- Memory fragmentation.
- Shared runtime memory.
- Resource exhaustion.
- Memory isolation.
- Interaction with OS-managed buffers.

The precise memory-management interface between runtimes and the OS remains undecided.

## 8. Runtime Scheduling and Queues

Runtimes must integrate with the QueueOS execution model.

An application running inside a language runtime should be able to submit work through the same queue infrastructure as a native application.

The runtime may provide language-level abstractions for asynchronous operations, event handling, and waiting.

However, those abstractions should preserve the underlying queue semantics.

The runtime should not require a separate scheduling architecture merely because it uses an interpreted or managed language.

The relationship between runtime execution contexts, OS queues, and scheduler resources remains an implementation decision.

## 9. Security and Isolation

Runtimes are part of the application execution environment and must respect the QueueOS security model.

An application running under Lua should have the same underlying authorization constraints as an equivalent native application.

A runtime must not become a way to bypass application Rights.

Security considerations include:

- Access to OS services.
- Native extension loading.
- Module loading.
- File and storage access.
- Network access.
- Memory isolation.
- Runtime debugging interfaces.
- Inter-application communication.

Native extensions deserve particular attention because they execute machine code within the runtime environment.

The runtime should not grant extensions unrestricted access merely because they are loaded by an authorized application.

The precise isolation mechanism will depend on the final execution architecture.

## 10. Runtime and Application Lifecycle

The runtime lifecycle is related to, but distinct from, the application lifecycle.

A shared runtime may serve multiple applications.

A runtime may also be started and stopped independently of individual application operations, depending on its implementation.

The OS must account for:

- Runtime initialization.
- Application attachment.
- Module loading.
- Application execution.
- Runtime errors.
- Application termination.
- Runtime shutdown.
- Resource cleanup.

The system should define what happens when a shared runtime fails while multiple applications depend on it.

Whether applications receive independent runtime instances or share execution resources is not yet established.

## 11. Interoperability

Different languages and runtimes should be able to use the same OS services.

A native C application and a Lua application should be able to submit equivalent operations through the ABI.

Likewise, they should be able to exchange information through defined service interfaces without requiring one to understand the internal representation of the other's runtime.

The ABI provides the common boundary.

Language bindings provide convenient interfaces within each language.

The architecture should avoid defining separate versions of OS functionality for every supported language.

## 12. Performance Considerations

Runtime performance should be evaluated using realistic workloads.

Relevant considerations include:

- Startup time.
- Memory consumption.
- Execution throughput.
- Garbage-collection overhead.
- Queue submission overhead.
- Data conversion costs.
- Native extension overhead.
- Runtime sharing efficiency.

A runtime should not be selected solely because it is small or popular.

Its suitability depends on the intended workload and the costs it introduces into the overall system.

### 12.1 Native and interpreted boundaries

Crossing between native code and an interpreted runtime can introduce overhead.

The architecture should avoid unnecessary transitions, particularly in frequently executed paths.

Performance-sensitive operations may be implemented natively and exposed to higher-level runtimes through well-defined interfaces.

This should be based on measured needs rather than assuming that every application requires native implementation.

## 13. Runtime Diagnostics and Debugging

QueueOS should provide a consistent way to diagnose application and runtime failures.

Runtime-specific debugging tools may be appropriate, but they should integrate with the OS's application lifecycle and diagnostic facilities.

Useful information may include:

- Application identity.
- Runtime identity and version.
- Startup failures.
- Module loading errors.
- Unhandled exceptions.
- Resource exhaustion.
- Queue operation failures.
- Runtime termination reasons.

Diagnostic interfaces must respect application isolation and should not expose one application's private data to another.

The exact debugging and diagnostic mechanisms remain open.

## 14. Open Design Questions

The following questions require further investigation.

### Native execution

- Which executable format will be supported?
- What loader behaviour is required?
- What native linking model will be used?
- How are shared libraries identified and versioned?

### Lua

- Which Lua version or implementation should be used?
- Will Lua be a system-provided runtime?
- Will applications share runtime instances or use separate instances?
- How will native modules be packaged and loaded?
- Which OS bindings should be provided by default?

### Other runtimes

- Which additional languages or runtimes are worth supporting?
- Should managed execution environments be supported?
- How are runtime dependencies declared?
- How are incompatible runtime versions managed?

### Execution and resources

- How are runtimes scheduled?
- How are runtime memory allocations accounted for?
- How are runtime failures isolated?
- How do runtime execution contexts relate to OS queues?
- How are shared runtime resources managed?

### Security

- How are native extensions restricted?
- How is module loading authorized?
- How are runtime debugging facilities protected?
- How is application identity preserved through runtime operations?

These questions should be addressed as the native execution and ABI implementations mature.

## 15. Current Direction and Commitments

### Established direction

- Applications use the common QueueOS ABI regardless of programming language.
- Native compiled applications are supported as a fundamental execution model.
- Runtime environments operate above the common ABI.
- Lua is a runtime under consideration.
- C is the preferred language for foundational native OS functionality.
- Runtimes must respect application Rights and OS resource controls.
- Runtime abstractions should not create separate OS authorization or communication models.
- Runtime overhead should be measured and minimized.

### Not yet established

- Executable format and loader.
- Native linking model.
- Shared library format and versioning.
- Lua version and implementation.
- Runtime instance-sharing model.
- Runtime packaging and update strategy.
- Native extension interface.
- Support for additional languages.
- Runtime scheduling and memory-management integration.
- Runtime debugging and diagnostics interfaces.

Implementation decisions should preserve the established direction while treating unresolved mechanisms as design work.

---

**Status:** Runtime architecture direction established; runtime selection and integration under design.

**Related documents:**
- `ABI.md`
- `APPLICATIONS.md`
- `ARCHITECTURE.md`
- `DATABASE.md`
- `EXECUTION.md`
- `SECURITY.md`
- `STORAGE.md`
- `AGENTS.md`
