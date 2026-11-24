# QueueOS — System Architecture

**Document status:** Architectural overview  
**Revision:** 0.1  
**Scope:** System-wide architecture and component relationships

## 1. Introduction

QueueOS is designed around a fundamentally different relationship between the operating system, its storage, applications and execution environment.

Rather than treating the operating system as a collection of independently installed binaries operating on top of a conventional filesystem, QueueOS places its own persistent system components within an integrated database architecture.

The database is not simply a service used by the operating system. It is a fundamental part of the operating system itself.

The architecture is intended to provide:

- A self-contained, bootable operating system.
- Consistent system state and persistence.
- A queue-oriented application interface.
- A stable ABI across processor architectures.
- Independently manageable OS components.
- A clear distinction between the operating system and installed applications.
- Efficient execution without unnecessary abstraction or communication overhead.

This document describes the overall architectural model. Individual subsystems are specified separately.

## 2. Fundamental System Model

QueueOS consists of three principal architectural domains.

### 2.1 The OS Database

The OS database contains the persistent components necessary to operate QueueOS.

It is an integral part of the operating system rather than an independently deployed database service.

The database must be independently sufficient to provide a bootable, operational system.

Its contents include the operating system's own components, their associated resources and the information required to initialize and maintain the system.

The database is OS-specific. It is not intended to become a general-purpose container for installed applications.

### 2.2 The Native OS Environment

The native OS environment executes the components provided by the OS database.

These include the foundational system implementation and the components required to provide the operating system's functionality.

Examples include:

- Core system functionality.
- Database execution and management.
- Lua runtime.
- Desktop engine.
- Zsh shell.
- Scheduling and execution services.
- System APIs and application interfaces.

These components are part of QueueOS itself and are loaded directly from the OS database.

They should not automatically be treated as conventional external applications or independent services.

### 2.3 The Application Environment

Installed applications exist outside the OS database.

Applications use the interfaces provided by QueueOS to access system functionality and resources.

The separation allows the operating system to remain self-contained while applications can be installed, upgraded, removed and managed independently.

The distinction is based on architectural ownership, not simply whether a component is executable.

For example, an upgraded OS file picker may be distributed as part of an OS update even though it is also an executable component.

The classification of components should therefore reflect their role in the operating system.

## 3. Architectural Relationships

The following diagram illustrates the conceptual relationships between the principal domains.

```text
+------------------------------------------------------+
|                     APPLICATIONS                     |
|                                                      |
|       Installed applications and user software      |
|                                                      |
+--------------------------+---------------------------+
                           |
                           | QueueOS ABI
                           | Primary interface: Queues
                           |
+--------------------------v---------------------------+
|                    NATIVE OS ENVIRONMENT             |
|                                                      |
|  +-------------+  +-------------+  +--------------+  |
|  | Desktop     |  | Lua Runtime |  | Zsh          |  |
|  | Engine      |  |             |  |              |  |
|  +-------------+  +-------------+  +--------------+  |
|                                                      |
|  +-----------------------------------------------+   |
|  | Core OS Functionality and System Interfaces  |   |
|  +-----------------------------------------------+   |
|                                                      |
+--------------------------+---------------------------+
                           |
                           | Integrated execution
                           | and persistent state
                           |
+--------------------------v---------------------------+
|                       OS DATABASE                    |
|                                                      |
|  Bootable OS components                              |
|  System resources                                    |
|  Persistent system state                             |
|  Component definitions                               |
|  System configuration                                |
|                                                      |
+------------------------------------------------------+
```

This is a logical architecture diagram, not a finalized execution or memory layout.

In particular, it does not prescribe process boundaries, privilege transitions, memory ownership or the physical implementation of database access.

Those decisions belong to the relevant subsystem specifications.

## 4. Execution Architecture

QueueOS uses queues as its primary application-to-operating-system interface.

Applications interact with system functionality through the QueueOS ABI, with queues providing the principal mechanism for submitting work and receiving results.

This approach is intended to provide a consistent communication model without requiring every system service to implement identical internal behavior.

### 4.1 Queue-oriented interaction

Queues are an architectural primitive.

They should be suitable for a variety of interactions, including:

- Application requests.
- System service access.
- Event delivery.
- Asynchronous operations.
- Resource requests.
- Communication between system components.

The queue infrastructure should provide consistent underlying mechanisms while allowing individual services to define their own semantics.

### 4.2 Avoid unnecessary prescription

QueueOS should standardize the queue plumbing rather than unnecessarily standardizing every conversation between components.

Individual services should retain the freedom to define appropriate request structures, responses, event formats and operational behavior.

### 4.3 Execution efficiency

The architecture should not introduce process boundaries, serialization, copying or context transitions merely for architectural purity.

Execution boundaries should exist where they provide meaningful benefits, particularly for security, isolation or fault containment.

Performance and correctness must be evaluated together.

Detailed queue semantics, scheduling and execution models are specified separately.

## 5. Application Interface and ABI

QueueOS will provide a single stable application binary interface across supported CPU architectures.

Applications should not need separate operating-system interfaces merely because the underlying processor architecture changes.

The ABI defines the boundary between applications and the operating system.

It should provide consistent mechanisms for:

- Queue access.
- System functionality.
- Resource interaction.
- Application initialization.
- Error handling.
- Data exchange.

Architecture-specific implementation details must remain behind this interface.

### 5.1 Executable containers

ELF may be used as an executable container.

Its use does not imply Linux compatibility, Linux system calls or adoption of the Linux execution environment.

The executable format and the operating-system ABI are separate concerns.

Further executable-format and ABI decisions belong in the interfaces specification.

## 6. Storage and Persistence

The OS database is responsible for the persistent representation of QueueOS itself.

The database is a core architectural component, not an optional facility that can be removed without changing the nature of the operating system.

Its responsibilities include maintaining the information necessary to reconstruct and operate the system.

The storage architecture must consider:

- Persistent system state.
- Component storage.
- Data integrity.
- Transactional consistency.
- Recovery.
- Updates.
- Resource allocation.
- Efficient access.

Installed applications remain outside the OS database.

The relationship between the OS database and external filesystems, including the precise boundary for accessing external storage, remains subject to further architectural specification.

See `STORAGE.md` for the detailed storage architecture.

## 7. Boot and Initialization

The boot process must establish the minimum environment necessary to access the OS database and initialize QueueOS.

A fundamental architectural requirement is that the database alone contains sufficient operating-system material to reach an operational state.

The boot process must therefore avoid depending on an independently installed collection of conventional OS components.

Bootstrapping, initial system blocks, hardware initialization, database activation and subsequent component loading will be specified independently.

See `BOOT.md` for the boot architecture.

## 8. Security and Application Rights

Security is an architectural responsibility spanning the entire system.

QueueOS distinguishes between application features and the rights granted to applications.

Features describe what an application does.

Rights describe which operating-system resources and operations the application is authorized to access.

Application installation should include an authorization presentation that identifies the rights being requested or granted.

The architecture must eventually establish the mechanisms for:

- Application identity.
- Resource ownership.
- Authorization.
- Access control.
- Isolation.
- System component privileges.
- Revocation of rights.

The precise security boundaries and enforcement mechanisms remain part of the security specification.

See `SECURITY.md` for further details.

## 9. Processor Architecture and Portability

QueueOS is intended to support multiple processor architectures through a common application interface.

The architecture should separate:

1. Processor-specific hardware initialization.
2. Architecture-specific native implementation.
3. Architecture-independent OS functionality.
4. The stable application ABI.

Architecture-specific optimizations are permitted where they provide measurable benefits, provided they do not compromise the common interface or portability.

The architecture must also account for differences in available hardware capabilities rather than assuming that all supported processors implement identical instructions or acceleration facilities.

QEMU/KVM will be used to establish and validate initial architecture support before broader physical hardware testing.

## 10. Component Architecture

QueueOS is intended to be modular.

Components should have clearly defined responsibilities and interfaces, without introducing unnecessary dependencies or communication overhead.

The distinction between an OS component and an installed application is architectural rather than purely technical.

A component can be executable and independently replaceable while still belonging to the operating system.

Likewise, an application may use operating-system functionality extensively without becoming part of the OS database.

The architecture should permit OS components to evolve independently where practical while preserving the integrity of the complete system.

## 11. Outstanding Architectural Questions

The following subjects require dedicated specifications or further design work.

| Subject | Outstanding work |
|---|---|
| Database internals | Physical representation, indexing, allocation and recovery |
| Boot architecture | Initial blocks, bootstrap stages and initialization sequence |
| Queue infrastructure | Queue representation, lifecycle and transport |
| Execution | Scheduling, concurrency and execution boundaries |
| ABI | Calling conventions, data structures and versioning |
| Application format | Executable containers and loading |
| External storage | Filesystem access and integration boundaries |
| Configuration | Ownership, persistence and configuration management |
| Security | Rights enforcement, isolation and resource ownership |
| Updates | Atomic OS updates, rollback and recovery |
| Hardware | Architecture-specific initialization and capabilities |

These questions should be resolved in their respective specifications rather than prematurely fixed in this overview.

## 12. Architectural Invariants

The following principles are fundamental to the current design:

1. The OS database is an integrated part of QueueOS.
2. The database must independently contain enough material to reach a bootable, operational OS.
3. Installed applications are outside the OS database.
4. OS components can be stored in and loaded directly from the database.
5. Queues are the primary application-to-OS interface.
6. QueueOS provides one stable ABI across supported CPU architectures.
7. ELF is an executable container choice, not a Linux compatibility commitment.
8. The architecture must avoid unnecessary execution and communication overhead.
9. Security and application Rights are fundamental architectural concerns.
10. Implementation decisions must preserve the distinction between established requirements and unresolved design questions.

Changes to these invariants require explicit architectural consideration and documentation.

