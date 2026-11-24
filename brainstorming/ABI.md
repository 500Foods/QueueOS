# QueueOS Application Binary Interface (ABI)

## 1. Purpose

This document describes the Application Binary Interface (ABI) for QueueOS.

The ABI defines the contract between applications and the operating system. It establishes how applications access OS functionality, submit work, receive results, and interact with system services.

A central objective is to maintain **one stable application-facing ABI across all supported CPU architectures**.

The ABI should allow an application to be developed against QueueOS without requiring its author to implement architecture-specific versions of ordinary OS interactions.

The internal implementation may differ between architectures, but the externally visible contract must remain consistent.

This document describes the intended ABI architecture. Exact binary layouts, calling conventions, executable formats, and versioning rules remain to be specified.

## 2. Design Principles

The ABI should follow these principles:

- **Architecture independence:** The application-facing contract remains consistent across supported CPU architectures.
- **Queue-centric interaction:** Queues are the primary mechanism for applications to interact with OS services.
- **Stable contracts:** Applications should not depend on internal OS implementation details.
- **Minimal overhead:** ABI operations should avoid unnecessary copying, serialization, context switching, and intermediate layers.
- **Explicit data representation:** Structures and values crossing the ABI boundary must have well-defined representations.
- **Controlled access:** Operations are subject to the application's granted Rights.
- **Version compatibility:** The OS must be able to identify and handle ABI compatibility requirements.
- **Extensibility:** The ABI should be capable of evolving without unnecessarily breaking existing applications.
- **Language neutrality:** The ABI should not require applications to use one particular programming language.

## 3. ABI Scope

The ABI defines the application-facing contract for interaction with QueueOS.

Its scope includes:

- Application startup and initialization.
- Queue creation, access, and submission.
- Operation completion and result retrieval.
- Event delivery.
- Memory and buffer exchange.
- Access to OS services.
- Error reporting.
- Application lifecycle interactions.
- Authorization-related responses.
- Compatibility and version negotiation.

The ABI does not necessarily define the internal implementation of these facilities.

For example, it may define how an application submits a storage request without specifying how the storage service accesses physical blocks or how the database engine manages its internal structures.

Those implementation details belong to the relevant OS components.

## 4. One ABI Across Architectures

QueueOS is intended to support multiple CPU architectures without maintaining a separate application interface for each one.

The ABI must therefore avoid exposing architecture-specific assumptions unnecessarily.

This includes avoiding implicit dependence on:

- CPU register layouts.
- Native pointer sizes.
- Compiler-specific structure packing.
- Architecture-specific calling conventions.
- Native integer widths.
- Endianness.
- Unspecified alignment behaviour.

Where the ABI requires binary structures, their sizes, field offsets, alignment, and interpretation must be explicitly defined.

Native CPU details may be used internally by an implementation, but they must not accidentally become part of the application-facing contract.

### 4.1 Internal architecture-specific implementations

The ABI does not require identical internal implementations on every architecture.

An implementation may use architecture-specific instructions, optimized memory operations, or different low-level mechanisms.

The requirement is that applications observe the same defined behaviour.

This allows QueueOS to optimize for available hardware without fragmenting its application ecosystem.

### 4.2 Native executable code

A native application executable will necessarily contain machine-specific instructions.

Architecture independence of the ABI does not mean that one compiled native binary must run on every CPU architecture.

It means that an application targeting a supported architecture uses the same OS interface and behavioural contract as an application targeting another supported architecture.

Architecture-independent executable environments may provide additional portability, but they are a separate concern.

## 5. Queue Interface

The queue interface is the central part of the application ABI.

Applications submit work through queues rather than relying on a large collection of unrelated service-specific entry mechanisms.

The ABI should provide consistent operations for:

- Accessing an authorized queue.
- Submitting work.
- Receiving completion information.
- Retrieving operation results.
- Waiting for work or completion where appropriate.
- Cancelling operations where supported.
- Receiving events.
- Releasing queue resources.

The exact queue representation is not yet established.

The ABI should describe queue behaviour and the application-visible contract without requiring queues to use a particular internal data structure.

### 5.1 Service operations

Individual OS services define the operations available through their interfaces.

For example, a storage service may support read, write, and object-management operations, while a networking service exposes connection and communication operations.

The queue infrastructure provides common submission and completion mechanisms.

Service-specific operation definitions describe the actual work being requested.

This separation should allow new services and operations to be introduced without redesigning the fundamental queue interface.

### 5.2 Request representation

Requests need a defined representation that applications and services can interpret consistently.

The ABI must eventually specify:

- Operation identification.
- Request metadata.
- Payload representation.
- Request identity or correlation.
- Data ownership and lifetime.
- Completion and error representation.

The design should avoid requiring every payload to be serialized into a copied message.

Where safe and practical, references to managed buffers or other shared resources may be more efficient.

## 6. Memory and Data Representation

Data crossing the ABI boundary must have an unambiguous representation.

The ABI should define the interpretation of externally visible values independently of the compiler and CPU architecture.

### 6.1 Fixed-width types

Binary structures should use explicitly sized integer types where their width is part of the contract.

For example, an identifier intended to be 64 bits must remain 64 bits regardless of whether the application is running on a 32-bit or 64-bit architecture.

Native-sized types should be avoided in persistent or externally shared structures unless their interpretation is explicitly defined.

### 6.2 Pointers and references

Raw native pointers are architecture-specific and should not be used as portable identifiers or persistent references.

The ABI may permit pointers within a defined native execution context, but their scope and validity must be explicit.

For cross-component or shared-memory references, the design should consider architecture-independent handles, offsets, or other managed references.

The exact mechanism remains undecided.

### 6.3 Alignment and layout

Structures shared across the ABI boundary must have explicitly defined layout rules.

Compiler defaults must not determine the meaning of ABI-visible data.

Where structures contain reserved fields, their required values and future interpretation should be documented.

This will help preserve compatibility as the ABI evolves.

### 6.4 Endianness

The ABI must define byte order for binary representations that may be shared across architectures or persisted.

The chosen representation should be consistent and independent of the native endianness of the CPU.

## 7. Operation Completion and Errors

Applications need a consistent way to determine the outcome of submitted work.

The ABI should distinguish between:

- Successful completion.
- Successful completion with returned data.
- Failure.
- Pending operations.
- Cancellation.
- Timeout.
- Authorization denial.
- Resource exhaustion.
- Unsupported operations.

Services may define additional error information relevant to their functionality.

The ABI should provide a common representation for general error conditions while allowing service-specific detail.

Error handling should not depend on parsing human-readable strings.

### 7.1 Request correlation

Applications may have multiple outstanding operations.

The ABI needs a reliable mechanism to associate each completion with its originating request.

The mechanism should not require operations to complete in submission order unless the service explicitly guarantees that ordering.

### 7.2 Error namespaces

Service-specific errors should not collide with general ABI errors.

The exact error-code allocation and namespace structure remain to be defined.

## 8. Synchronous and Asynchronous Interfaces

The ABI should support both synchronous and asynchronous application interactions.

The underlying execution model remains queue-centric.

A synchronous interface may allow an application to wait for a particular operation to complete, while an asynchronous interface allows the application to continue working and collect completion information later.

The ABI should avoid creating separate and incompatible service interfaces merely to support these two interaction styles.

Whether synchronous operations are implemented as wrappers around asynchronous queue operations remains open.

The exact waiting, cancellation, and timeout conventions will be specified when the queue interface is finalized.

## 9. Authorization and Rights

The ABI must integrate with the QueueOS security model.

Applications may only perform operations permitted by their granted Rights.

The ABI should make authorization failures distinguishable from other operation failures.

However, the ABI itself is not the authority that grants permissions.

The OS maintains the authoritative authorization state and enforces it when protected operations are requested.

An application must not be able to bypass authorization by calling a lower-level ABI operation or constructing a request manually.

The ABI should also support services that expose operations with different permission requirements.

Possession of access to a service does not automatically authorize every operation that service provides.

## 10. Application Startup

The ABI must define how an application begins execution and establishes its initial relationship with the OS.

The startup contract will need to establish:

- Application identity.
- ABI compatibility information.
- Initial execution context.
- Access to permitted OS interfaces.
- Initial resource availability.
- Application entry point.
- Startup arguments or equivalent initialization data.

The application should not need to discover internal OS implementation details to initialize itself.

The precise startup structure and calling convention remain open.

## 11. OS Service Interfaces

The ABI provides the common contract through which applications interact with OS services.

Individual services define their own operation sets and associated data structures.

Examples include:

| Service | Example functionality |
|---|---|
| Storage | Read, write, create, and manage objects |
| Database | Execute queries and manage data |
| Networking | Establish connections and exchange data |
| Desktop | Create windows and interact with the desktop |
| Notifications | Register and receive notifications |
| Devices | Discover and access authorized hardware |

This list is illustrative rather than a finalized service catalogue.

The ABI should make it possible to add services and extend existing ones without requiring changes to unrelated application interfaces.

Service interfaces should have their own versioning and compatibility rules where necessary.

## 12. Executable Format and ABI Separation

The executable format and the ABI are related but distinct.

The executable format determines how application code is represented and loaded.

The ABI determines how that code interacts with the operating system.

ELF has been considered as a possible executable container.

This does not imply that QueueOS will adopt the Linux ABI, Linux system-call interface, or Linux execution model.

QueueOS may use an established executable format while providing its own application ABI.

The final executable format and loader contract remain open design decisions.

## 13. Language and Runtime Support

The ABI should be language-neutral.

Applications written in C, C++, or other compiled languages should be able to use the same underlying OS interfaces.

Interpreted languages and managed runtimes may also expose application functionality through the ABI.

Lua is one runtime being considered for the QueueOS environment.

A runtime may provide higher-level abstractions for applications, but it should ultimately use the same OS-facing contracts.

The ABI should not require every language runtime to independently implement low-level OS communication.

Language-specific libraries and bindings may be provided as convenience layers above the ABI.

These libraries must not introduce incompatible semantics or bypass OS authorization.

## 14. ABI Versioning and Compatibility

The ABI must evolve without unnecessarily breaking existing applications.

The design should distinguish between:

- The overall ABI version.
- Individual service interface versions.
- Optional capabilities.
- Runtime versions.
- Executable format versions.

An application should be able to declare the ABI version and service interfaces it requires.

The OS should be able to determine whether it can execute the application before starting it.

### 14.1 Compatible extensions

New functionality should preferably be introduced in a way that does not change the meaning of existing operations.

Possible approaches include:

- New operation identifiers.
- Optional fields.
- Versioned service interfaces.
- Explicit capability discovery.
- Reserved fields for future extension.

These are design options, not finalized rules.

### 14.2 Breaking changes

Changes that alter the interpretation or behaviour of existing ABI operations must be treated as compatibility-breaking changes.

The project should avoid introducing breaking changes casually.

Where breaking changes become necessary, the transition strategy should be documented and should account for installed applications.

## 15. Performance and Optimization

The ABI should be designed with performance as a first-class consideration.

It should not impose unnecessary overhead on common operations.

Potential sources of overhead include:

- Data copying.
- Serialization and deserialization.
- Excessive validation layers.
- Context switches.
- Redundant authorization checks.
- Excessive queue transitions.
- Unnecessary memory allocation.

These costs should be measured rather than assumed.

### 15.1 Fast paths

Frequently used operations may benefit from optimized paths.

However, fast paths must preserve the same security and behavioural guarantees as ordinary operations.

An optimization must not become a separate, weaker authorization mechanism.

### 15.2 Architecture-specific optimization

Architecture-specific optimizations are permitted internally.

They must preserve the ABI contract and must not introduce application-visible differences that violate the defined behaviour.

## 16. Documentation and Testing

Every ABI operation must be documented sufficiently for an independent application developer to use it correctly.

Documentation should define:

- Operation purpose.
- Required inputs.
- Returned values.
- Error conditions.
- Required Rights.
- Data ownership and lifetime.
- Ordering guarantees.
- Cancellation behaviour.
- Compatibility requirements.

The ABI should have conformance tests that can be run against implementations on every supported architecture.

Tests should verify both the binary contract and observable behaviour.

Where practical, the same application-level test suite should be usable across architectures.

This is essential to demonstrating that QueueOS really provides one consistent application ABI.

## 17. Open Design Questions

The following questions remain unresolved.

### Binary contract

- What calling convention will the ABI use?
- What are the canonical integer and structure types?
- How are handles and references represented?
- What are the rules for alignment and padding?
- What byte order is used for shared binary representations?

### Queue interface

- What are the exact queue operations?
- How are requests and completions represented?
- How are queue references acquired and released?
- How are events delivered?
- What are the cancellation and timeout conventions?

### Memory

- How are buffers shared between applications and services?
- Are zero-copy operations supported?
- How are ownership and lifetime managed?
- How are shared-memory references represented across architectures?

### Startup and execution

- What is the application startup contract?
- What executable formats will be supported?
- How is ABI compatibility declared and negotiated?
- How are runtime environments integrated?

### Versioning

- What constitutes an ABI-breaking change?
- How are service interface versions represented?
- How are optional capabilities discovered?
- How long must older ABI versions remain supported?

### Performance

- Which operations require optimized fast paths?
- What is the minimum overhead for a queue submission?
- How should architecture-specific optimizations be exposed, if at all?
- What benchmarks will define acceptable performance?

These questions should be resolved through implementation, testing, and measurement rather than speculative complexity.

## 18. Current Direction and Commitments

### Established direction

- QueueOS provides one stable application-facing ABI across supported CPU architectures.
- Queues are the primary application-to-OS interaction mechanism.
- The ABI defines the external contract, not the internal implementation.
- Applications interact with OS services through defined interfaces.
- Authorization and granted Rights apply to ABI operations.
- The ABI should be language-neutral.
- The ABI should avoid unnecessary copying, serialization, and execution overhead.
- Architecture-specific optimizations must preserve the common contract.
- Executable format and ABI are separate concerns.
- ELF has been considered, but Linux ABI compatibility is not an established goal.

### Not yet established

- Calling convention.
- Binary structure definitions.
- Queue operation identifiers and layouts.
- Handle and reference representation.
- Memory-sharing model.
- Error-code allocation.
- Startup contract.
- Executable format and loader.
- ABI and service versioning rules.
- Runtime integration details.
- Conformance-test specification.

Implementation decisions should preserve the established direction while treating unresolved mechanisms as design work.

---

**Status:** ABI architectural direction established; binary contract and implementation under design.

**Related documents:**
- `ARCHITECTURE.md`
- `APPLICATIONS.md`
- `DATABASE.md`
- `EXECUTION.md`
- `SECURITY.md`
- `STORAGE.md`
- `AGENTS.md`
