# QueueOS Applications

## 1. Purpose

This document describes the application model used by QueueOS.

Applications are the primary means by which users extend the functionality of the operating system. They should be independently installable, identifiable, updateable, and removable without requiring changes to the core OS.

QueueOS is designed around a queue-centric execution model. Applications interact with OS functionality through the interfaces and services provided by the operating system, rather than depending on unrestricted access to internal OS components.

The application model must integrate with:

- The QueueOS execution and queue infrastructure.
- The OS Database and storage architecture.
- The Features and Rights authorization model.
- The architecture-independent application ABI.
- The installation, update, and removal lifecycle.

The precise package format and installation mechanisms remain open design questions.

## 2. Application Design Principles

The application architecture should follow these principles:

- **Independent installation:** Applications can be installed and removed without rebuilding the OS.
- **Explicit identity:** Every installed application has a stable identity.
- **Declared functionality:** Applications describe the Features they provide.
- **Explicit authorization:** Applications declare the Rights they request; the OS controls which Rights are granted.
- **ABI consistency:** Applications use the stable QueueOS application ABI.
- **Architecture independence:** Application interfaces should not unnecessarily depend on a particular CPU architecture.
- **Isolation:** Applications should not gain unrestricted access to other applications or OS internals.
- **Managed resources:** Application data, executable content, and metadata have defined ownership and lifecycles.
- **Controlled updates:** Updates should preserve application identity and must not silently expand authorization.
- **Minimal installation overhead:** The package and installation model should avoid unnecessary duplication and processing.

## 3. Application Identity

Each application must have a unique identity within QueueOS.

The identity should remain stable across application updates and should not depend on the application's display name, installation location, or current version.

Application identity will be used for:

- Installation records.
- Application metadata.
- Authorization and granted Rights.
- Application-owned data.
- Queue access and service interactions.
- Update management.
- Audit records.
- Uninstallation.

The exact identity format has not yet been established.

A reverse-domain identifier is one possible convention, but the canonical format should be selected independently of any particular package manager or distribution mechanism.

An application may also have a human-readable name, publisher information, description, and icon. These are presentation and discovery attributes, not substitutes for its stable identity.

## 4. Application Metadata

Applications need a machine-readable description of their identity, requirements, functionality, and authorization requests.

The metadata should provide enough information for QueueOS to install, identify, authorize, execute, update, and remove an application.

Candidate metadata fields include:

| Field | Purpose |
|---|---|
| Application ID | Stable application identity |
| Name | Human-readable application name |
| Version | Application release version |
| Publisher | Application publisher or maintainer |
| Description | Short application description |
| Features | Functionality provided by the application |
| Requested Rights | Permissions requested by the application |
| Entry point | Application startup entry point |
| ABI version | Required QueueOS application ABI |
| Dependencies | Other required components or services |
| Architecture | Supported execution architectures, where relevant |
| Package format version | Version of the application package specification |
| Icon and presentation metadata | Information used by the application interface |

This is a conceptual list, not a finalized manifest schema.

The manifest must distinguish application-provided information from information controlled by the OS.

In particular, an application may declare its requested Rights, but only the OS can record and enforce the Rights actually granted.

## 5. Features and Rights

QueueOS distinguishes between application Features and Rights.

### 5.1 Features

Features describe what an application provides.

Examples include:

- Email.
- Calendar.
- Contacts.
- Image editing.
- File management.
- Database administration.
- Network monitoring.
- Audio playback.

Features can help users discover applications and understand their purpose.

They may also allow the OS to organize applications into categories or expose their capabilities to other system components.

Features are descriptive. They do not grant permission to access OS resources.

### 5.2 Requested Rights

Applications declare the Rights they need to operate.

Examples include:

- Read or write selected files.
- Access a storage namespace.
- Use network services.
- Access a particular device.
- Display notifications.
- Communicate with another application.
- Access a specific OS service.

Requests should be scoped as narrowly as practical.

An application should be able to identify which Rights are essential and which enable optional functionality.

### 5.3 Granted Rights

The OS maintains the authoritative record of granted Rights.

An application cannot grant itself permissions by modifying its own manifest or installation metadata.

The installation and authorization process is described in `SECURITY.md`.

## 6. Application Packages

Applications must be distributable in a package format that contains the information and resources necessary for installation.

A package may contain:

- Application metadata.
- Executable code.
- Required application resources.
- Icons and presentation assets.
- Localization data.
- Supporting libraries or modules.
- Installation or migration metadata, where necessary.
- Integrity information.

The package format should be designed for efficient inspection, verification, installation, and updating.

It should not require an application to be installed into the OS Database.

The exact package container, compression format, signing mechanism, and internal directory structure remain undecided.

### 6.1 Package integrity

QueueOS should be able to verify that application packages have not been corrupted or modified unexpectedly.

Package integrity and publisher authenticity are related but separate concerns.

A checksum can detect accidental corruption, but it does not establish who published a package.

The package trust model, including signing and publisher verification, will be specified separately.

### 6.2 Package portability

The application package format should avoid unnecessary architecture-specific assumptions.

Where an application includes native executable code, its compatibility with the target architecture and QueueOS ABI must be established.

Applications may also include architecture-independent resources or executable modules.

The package format should accommodate these differences without requiring unrelated application metadata to be duplicated for every architecture.

## 7. Application Executables

Applications execute using the QueueOS application ABI.

The ABI defines the externally visible contract between applications and the operating system, including access to queues and OS services.

The application model should not require applications to depend directly on internal OS implementation details.

### 7.1 Executable format

ELF has been considered as a possible executable container.

This does not imply that QueueOS will implement Linux ABI compatibility or adopt Linux execution semantics.

The executable format and the application ABI are separate concerns:

- The executable format describes how executable content is represented and loaded.
- The ABI describes how applications interact with the operating system.

The final executable format remains an open design decision.

### 7.2 Application runtime

Applications may use native executable code, interpreted languages, or other supported execution environments.

The OS may provide shared runtime components for particular languages or application frameworks.

Lua is one runtime being considered as part of the broader QueueOS environment.

The application model should allow applications to use supported runtimes without requiring every application to bundle an independent copy.

Runtime availability, version compatibility, and isolation requirements will be addressed as the execution architecture develops.

## 8. Application Installation

Installation introduces an application into the system and establishes its identity, resources, and authorization state.

A conceptual installation sequence is:

1. Locate or receive the application package.
2. Inspect its metadata.
3. Validate package structure and integrity.
4. Check ABI and runtime compatibility.
5. Identify dependencies.
6. Present the application information and requested Rights to the user.
7. Obtain the user's authorization decisions.
8. Install application resources.
9. Register the application with the OS.
10. Make the application available for execution.

The exact sequence may vary as the implementation develops, but authorization must occur before the application can exercise newly requested Rights.

### 8.1 Installation authorization

The installation interface should display:

- Application name and publisher.
- Application description.
- Declared Features.
- Requested Rights.
- Required and optional permissions.
- Compatibility information.
- Any additional warnings relevant to installation.

Users should be able to understand what they are installing and what access they are granting.

An application should not silently acquire additional permissions through its installation process.

### 8.2 Dependencies

Applications may depend on shared OS services, runtimes, or other application components.

The package metadata should identify dependencies sufficiently for the OS to determine whether installation is possible.

Dependency resolution should avoid unnecessary duplication of shared components.

The exact dependency model, including version constraints and conflict resolution, remains open.

## 9. Application Storage

Application executable content and application data must be managed separately from the OS Database.

The OS Database contains the components and state necessary to operate QueueOS.

Installed applications are outside that database.

Application storage may include:

- Executable files.
- Resources and assets.
- Application configuration.
- User-generated data.
- Cache data.
- Logs.
- Application-specific databases.
- Temporary files.

The OS should provide a consistent way to identify and manage application-owned storage.

Applications should not need unrestricted access to the physical storage layout.

Instead, storage access should be provided through authorized OS interfaces.

The precise application storage namespace and directory layout remain undecided.

## 10. Application Execution Lifecycle

An application has a lifecycle managed by the OS.

Conceptually, the lifecycle includes:

1. Available for execution.
2. Starting.
3. Running.
4. Suspended or inactive, where supported.
5. Stopping.
6. Terminated.

These are logical lifecycle states, not a commitment to conventional process semantics.

The implementation must define how applications acquire execution resources, establish queue connections, initialize their runtime, and release resources when they stop.

### 10.1 Startup

When an application starts, the OS should establish the execution context and make the application's authorized interfaces available.

The application should not receive unrestricted access to OS services merely because it has started.

Startup should validate any required runtime, ABI, and service dependencies.

### 10.2 Termination

When an application stops, the OS must account for its outstanding work and resources.

This includes:

- Pending queue operations.
- Active service interactions.
- Open resources.
- Allocated memory.
- Event subscriptions.
- Temporary storage.
- Application-specific runtime state.

The treatment of outstanding operations should follow the relevant service and execution semantics.

### 10.3 Multiple instances

Whether an application may have multiple simultaneous instances is a property that should be declared or otherwise defined.

Some applications naturally support multiple instances, while others may require a single shared instance.

The application model should accommodate both without imposing a universal single-instance or multi-instance rule.

## 11. Application Updates

Updates replace or modify an installed application's executable content and resources while preserving its identity.

A conceptual update process is:

1. Identify the installed application.
2. Validate the new package.
3. Check compatibility and dependencies.
4. Compare requested Rights with the current authorization state.
5. Request authorization for any additional Rights.
6. Prepare the updated application resources.
7. Apply the update.
8. Preserve or migrate application data as required.
9. Record the new installed version.

An update must not silently expand the application's granted Rights.

### 11.1 Versioning

Applications need an explicit version identifier.

The versioning scheme should support compatibility checks and dependency resolution.

The exact versioning convention has not been finalized.

### 11.2 Data migration

Application updates may require changes to stored application data.

Migration should be explicit and should avoid leaving application data partially converted if an update fails.

Where practical, the system should provide a way to recover from an unsuccessful update.

The precise migration and rollback mechanisms remain open.

## 12. Application Removal

Uninstallation removes an application from the available application environment.

The OS should:

- Stop the application if it is running.
- Prevent new execution.
- Revoke its granted Rights.
- Remove its registration and discovery metadata.
- Invalidate associated queue references and access.
- Remove application executable resources.
- Handle dependencies and shared resources safely.
- Apply the selected policy for application-owned data.

Application data should not necessarily be deleted automatically in every case.

Users may want to retain data for backup, migration, or later reinstallation.

The removal interface should make data retention and deletion behaviour clear.

Shared runtimes and dependencies must not be removed while still required by other applications.

## 13. Application Discovery and Launching

QueueOS should provide a consistent mechanism for discovering installed applications and making them available to the user.

Discovery may use application metadata, including:

- Application identity.
- Display name.
- Icon.
- Features.
- Version.
- Availability.
- Execution entry point.

The desktop environment may use this information to present an application launcher, organize applications, or expose application-specific actions.

Discovery is separate from authorization.

An application being visible or discoverable does not imply that it has permission to access a particular resource.

The exact application discovery and launching interfaces remain open.

## 14. Application-to-Application Interaction

Applications may need to exchange information or request functionality from other applications.

QueueOS should provide a controlled mechanism for this interaction.

The queue model is the natural foundation for such communication.

Applications should be able to expose defined interfaces, and other applications should be able to request access to those interfaces subject to authorization.

Potential examples include:

- Opening a document in another application.
- Sending information to a mail client.
- Requesting a calendar operation.
- Passing an object to an image editor.
- Invoking a service provided by another application.

The architecture should not require applications to share internal memory or private storage simply to exchange information.

The exact discovery, invocation, and authorization mechanisms remain to be defined.

## 15. Application and OS Component Boundaries

Applications and OS components have different roles.

OS components provide foundational functionality required by the operating system.

Applications provide independently installed functionality to users.

The OS Database contains the system components needed to reach an operational OS. Installed applications are managed separately.

Applications should consume OS services through supported interfaces rather than depending on the physical location or internal representation of system components.

This separation allows the OS to evolve its internal implementation without requiring every application to be rewritten.

## 16. Compatibility

Application compatibility depends on several independent factors:

- QueueOS ABI compatibility.
- Executable format compatibility.
- Required runtime availability.
- OS service availability.
- Dependency versions.
- Required application Rights.

The application metadata should identify relevant compatibility requirements.

The OS should be able to determine whether an application can run before attempting execution.

Architecture-specific executable code may require different package content for different CPU architectures, but the application-facing ABI should remain consistent.

Compatibility should be treated as an explicit contract rather than an assumption based on executable format alone.

## 17. Open Design Questions

The following questions remain unresolved.

### Identity and metadata

- What is the canonical application identifier format?
- What is the final application manifest schema?
- How are publisher identities represented?
- How are application Features standardized or categorized?

### Package format

- What container format will application packages use?
- Will packages support compression and deduplication?
- How will package integrity be verified?
- What signing and trust model will be used?
- How will architecture-specific and architecture-independent resources coexist?

### Execution

- What executable format will be supported?
- What is the precise application startup contract?
- How are runtime dependencies managed?
- How are multiple instances represented?
- What lifecycle controls are exposed to applications?

### Installation and updates

- How are dependencies resolved?
- How are application updates staged and committed?
- What rollback guarantees are provided?
- How are data migrations handled?
- How are additional Rights requested during updates?

### Storage and removal

- What is the canonical application storage namespace?
- How are application-owned databases managed?
- What data is retained after uninstallation?
- How are shared dependencies tracked?

### Discovery and interaction

- How are applications registered and discovered?
- How are application entry points represented?
- How do applications publish interfaces?
- How is application-to-application access authorized?

These questions should be resolved as the package, execution, and security implementations mature.

## 18. Current Direction and Commitments

### Established direction

- Applications are independently installable and removable.
- Installed applications are stored outside the OS Database.
- Applications use the stable QueueOS application ABI.
- Queue-based interaction is the primary application-to-OS interface.
- Applications declare their Features and requested Rights.
- Features and Rights remain separate concepts.
- The OS controls and enforces granted Rights.
- Installation includes an authorization review.
- Updates must not silently expand application permissions.
- Application data and executable content have managed lifecycles.

### Not yet established

- Application identity format.
- Manifest schema.
- Package container format.
- Executable format.
- Package signing and publisher trust.
- Dependency resolution.
- Runtime packaging and compatibility.
- Application storage layout.
- Application lifecycle implementation.
- Multiple-instance behaviour.
- Application discovery and inter-application invocation mechanisms.
- Update staging, rollback, and data migration mechanisms.

Implementation decisions should preserve the established direction while treating unresolved mechanisms as design work.

---

**Status:** Application architecture and lifecycle direction established; package and execution mechanisms under design.

**Related documents:**
- `ARCHITECTURE.md`
- `DATABASE.md`
- `EXECUTION.md`
- `SECURITY.md`
- `STORAGE.md`
- `AGENTS.md`
