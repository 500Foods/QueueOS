# QueueOS — OS Database Architecture

**Document status:** Architectural specification / evolving design  
**Revision:** 0.1  
**Scope:** OS database structure, boot metadata, generations, component representation, activation and recovery

## 1. Introduction

The QueueOS database is the authoritative persistent representation of the operating system.

It contains the components, metadata and resources necessary to reconstruct and operate QueueOS.

The database is not simply a repository of executable files. It is a structured representation of the operating system, including the relationships between components, their versions, dependencies and associated system resources.

The central architectural objective is to make the OS database independently sufficient to provide a bootable, operational operating system.

This has several important consequences:

- The operating system must be able to identify and access its database without relying on an independently installed OS.
- The initial bootstrap must understand only a small, stable subset of the database format.
- The full database engine can assume responsibility for normal operation once loaded.
- OS updates should produce new generations without destroying the previous operational generation.
- Database integrity must be established before an image is treated as bootable.

This document defines the conceptual structure and responsibilities of the OS database.

## 2. Design Principles

The OS database should follow these principles:

1. **Self-containment:** The database contains the material required to reconstruct and operate QueueOS.
2. **Bootstrap accessibility:** A small initial reader can locate and load the next initialization stage without implementing the full database engine.
3. **Architecture independence:** The persistent database format is independent of the processor architecture.
4. **Versioning:** OS database generations are identifiable and independently recoverable.
5. **Immutability:** Previously committed generations should not be modified in place during normal OS updates.
6. **Atomic publication:** A new generation becomes bootable only after its required contents have been written and validated.
7. **Recoverability:** An interrupted update must not unnecessarily compromise an existing operational generation.
8. **Minimal bootstrap complexity:** The initial reader should understand only the structures necessary to locate and load the next stage.
9. **Separation of concerns:** The bootstrap reader, full database engine and OS component manager have distinct responsibilities.

## 3. Physical Representation

The OS database occupies a defined storage region.

Its physical representation must be independent of conventional filesystem structures.

The database should be recognizable through a defined header and metadata structures.

### 3.1 Fixed-Size Physical Blocks

The initial database structures should use a fixed physical block size.

A consistent block size simplifies bootstrap access and provides predictable addressing.

The precise block size remains to be finalized.

The format must distinguish between:

- Physical storage blocks.
- Database metadata blocks.
- Database pages.
- Logical objects.
- OS component records.

These are different concepts and should not be conflated.

### 3.2 Metadata Blocks

Metadata blocks provide the structures necessary to identify and interpret the database.

The architecture should permit metadata structures to expand without requiring the initial physical layout to change.

A proposed approach is to identify metadata blocks using recognizable magic values.

This allows a reader to identify the type and format of a metadata structure without relying exclusively on a fixed positional layout.

Potential metadata structures include:

- Volume identity.
- Storage geometry.
- Database format information.
- Boot directory.
- Generation manifests.
- Component descriptors.
- Recovery information.

The exact metadata organization remains subject to implementation.

### 3.3 Expandable Metadata

Metadata structures should include sufficient information to identify their own format and size.

This allows future versions to extend metadata without necessarily invalidating earlier readers.

Where possible, new fields should be additive.

Unknown optional fields should be safely ignorable by readers that do not require them.

Unknown mandatory features must cause an appropriate compatibility failure rather than being silently misinterpreted.

## 4. Database Header and Identity

The database requires a recognizable identity structure.

The initial bootstrap must be able to establish that it is examining a QueueOS database rather than arbitrary storage.

The identity structure should provide information such as:

- Database magic identifier.
- Database format version.
- Physical block size.
- Database identity.
- Metadata location.
- Supported structural features.

The header should be small and stable.

Changes to the header format should be approached cautiously because it is among the earliest structures interpreted during boot.

The exact binary layout and field definitions require a separate format specification.

## 5. Boot Directory

The boot directory is a particularly important structure.

It provides the information necessary to discover available database generations without requiring the full database engine.

The initial design direction is a compact directory of approximately 1 KB.

### 5.1 Directory Contents

The directory should identify available generations and provide sufficient information to locate them.

Each entry should contain at least:

- Generation identifier.
- Starting location.
- Creation or publication timestamp.
- Generation size.

Additional fields may include:

- Generation status.
- Manifest location.
- Integrity information.
- Compatibility information.

### 5.2 JSON Representation

JSON is the proposed initial representation for the directory.

An illustrative example:

```json
{
  "format": 1,
  "generations": [
    {
      "id": 1,
      "start": 1048576,
      "timestamp": 1795300000,
      "size": 536870912
    },
    {
      "id": 2,
      "start": 537919488,
      "timestamp": 1795386400,
      "size": 603979776
    }
  ]
}
```

The values are illustrative and do not define the final on-disk format.

JSON offers several advantages:

- Human readability.
- Straightforward inspection.
- Expandability.
- Familiar parsing.
- Ease of generating and validating directory entries.

However, the bootstrap reader must not depend on a full general-purpose JSON runtime.

The representation should remain deliberately constrained.

The exact encoding, character set, integer representation, field ordering and validation rules must be defined before the format is finalized.

### 5.3 Directory Expansion

The initial directory should be small, but the architecture must accommodate additional entries and metadata.

A fixed initial allocation with expandable metadata structures is preferable to requiring a fundamental database-layout change whenever the directory grows.

The mechanism for expanding the directory remains to be determined.

## 6. Database Generations

OS database generations provide the foundation for versioning and recoverability.

A generation represents a complete, identifiable version of the operating system's persistent state.

A generation should not be confused with an individual executable version or an incremental database transaction.

### 6.1 Generation Identity

Each generation should have a unique, monotonically increasing identifier within its database.

Generation metadata should identify:

- Generation number.
- Creation timestamp.
- Physical location.
- Size.
- Manifest.
- Integrity status.
- Publication status.

Generation identifiers should remain stable throughout the lifetime of the generation.

### 6.2 Immutable Generations

Committed generations should be treated as immutable.

An OS update should create a new generation rather than modifying the currently active generation in place.

This has several advantages:

- Existing generations remain available.
- Failed updates do not destroy the previous system.
- Validation can occur before publication.
- Rollback becomes substantially simpler.
- Generation integrity can be evaluated independently.

Storage reclamation can subsequently remove generations that are no longer required.

### 6.3 Generation Selection

The boot environment must be able to identify suitable generations.

The newest generation is a candidate default, but generation number alone should not establish bootability.

A generation must first satisfy the required structural and integrity checks.

The boot manager should distinguish between:

- Preferred generation.
- Latest valid generation.
- Last-known-good generation.
- Other available generations.

These values represent different concepts.

A user's preferred generation should not automatically become the last-known-good generation merely because it was selected.

## 7. Generation Manifests

Each generation should have an associated manifest.

The manifest describes the contents and structure of that generation.

It provides the information necessary to determine whether the generation contains the components required for startup.

Potential manifest information includes:

- Generation identity.
- Database format compatibility.
- Required OS components.
- Component identifiers.
- Component locations.
- Component sizes.
- Component dependencies.
- Integrity information.
- Entry points.
- Required initialization order.

The manifest should be sufficient for the bootstrap reader to locate the next stage without understanding the complete database structure.

The full database engine can subsequently interpret the remaining metadata.

### 7.1 Component Descriptors

Components should have explicit descriptors.

A descriptor identifies a component and provides the information required to locate and interpret it.

Potential fields include:

- Component identifier.
- Component type.
- Version.
- Object location.
- Object size.
- Architecture requirements.
- Dependencies.
- Integrity information.
- Loading requirements.

Not every component requires every field.

The descriptor format should support multiple component types without assuming that everything stored in the database is an executable binary.

## 8. Bootstrapping and the Database Reader

The initial bootstrap reader is deliberately constrained.

It does not need to implement the complete QueueOS database engine.

Its purpose is to interpret the stable structures required to locate and load the next initialization stage.

### 8.1 Bootstrap Responsibilities

The reader should be able to:

1. Locate and identify the QueueOS database.
2. Read the database header.
3. Validate the initial metadata structures.
4. Locate the boot directory.
5. Identify available generations.
6. Evaluate the relevant generation metadata.
7. Locate the generation manifest.
8. Validate the required bootstrap objects.
9. Load the next initialization stage.
10. Transfer execution to that stage.

The reader should avoid unnecessary general-purpose functionality.

In particular, it should not implement:

- General SQL or query processing.
- Full transaction management.
- General-purpose object modification.
- Complete WAL recovery.
- Application-facing database APIs.
- General storage administration.

### 8.2 Bootstrap Size

The architecture-specific bootstrap implementation has a target size of approximately 200 KB.

This is a design target rather than an absolute limit.

The objective is to keep the bootstrap small enough to minimize complexity, reduce the amount of code requiring architecture-specific maintenance and simplify verification.

The target should be evaluated independently for each supported architecture.

### 8.3 Database Format Stability

The bootstrap reader should depend on a small, stable database contract.

Changes to the full database engine should not routinely require changes to the bootstrap format.

This separation is important for long-term compatibility.

A new database generation should remain readable by an appropriate bootstrap implementation even as the full database engine evolves.

## 9. Transition to the Full Database Engine

Once the next initialization stage has been loaded, responsibility can transfer from the bootstrap reader to the full QueueOS database implementation.

The full engine is responsible for establishing normal database operation.

Its responsibilities include:

- Complete metadata interpretation.
- Transaction recovery.
- WAL processing.
- Index initialization.
- Allocation management.
- Buffer-pool initialization.
- Database consistency validation.
- Persistent object access.
- Normal database operations.

The transition should be explicit.

The bootstrap reader should not attempt to become a partial implementation of the full database engine.

### 9.1 Recovery

The full database engine performs the recovery operations necessary to establish a consistent operational database state.

This may include replaying committed transactions, resolving incomplete transactions and validating relevant metadata.

The precise recovery sequence depends on the finalized storage-engine design.

### 9.2 Operational Handoff

After the database engine has established normal operation, subsequent OS initialization should access system components through the normal database interfaces.

This completes the transition from bootstrap access to operational QueueOS.

## 10. Atomic Generation Publication

Generation publication is one of the most important reliability mechanisms in the OS database.

A new generation must not become the selected boot target until its required contents have been completely written and validated.

The proposed approach is based on immutable generations and replaceable directory metadata.

### 10.1 Update Sequence

A conceptual update sequence:

1. Preserve the existing committed generation.
2. Allocate storage for the new generation.
3. Write the new generation's contents.
4. Write the generation manifest.
5. Validate the required objects and metadata.
6. Ensure the new generation's contents are durable.
7. Publish a new directory generation referencing the new image.
8. Validate the published directory.
9. Make the new generation eligible for boot selection.

The precise ordering of writes and flushes must be established by the storage-engine specification.

### 10.2 Directory Publication

The directory should support recovery from interrupted updates.

One proposed implementation uses multiple fixed directory slots.

For example, three slots of approximately 1 KB each could allow a new directory version to be written without immediately overwriting the previous valid version.

Each slot would contain a generation identifier and integrity information.

The bootstrap reader could examine the available slots and select the highest valid committed directory generation.

This is a proposed implementation, not a finalized requirement.

### 10.3 Interrupted Publication

If power is lost while publishing a new directory, the reader should be able to identify the previous valid directory.

A partially written directory must not be treated as committed merely because its generation number is higher.

Checksums, structural validation and publication ordering must establish validity.

The exact commit-record mechanism remains to be specified.

## 11. Preferred and Last-Known-Good Generations

QueueOS should distinguish user preference from operational success.

### 11.1 Preferred Generation

The preferred generation represents the user's desired default.

It may be selected explicitly through the boot interface.

The preference should persist across restarts.

### 11.2 Last-Known-Good Generation

The last-known-good generation represents a generation that has successfully completed the required startup process.

It provides a recovery reference when a newer generation fails.

The system should not designate a generation as last-known-good simply because the bootstrap reader successfully loaded its initial components.

Successful startup must be established through an explicit mechanism.

### 11.3 Boot Failure

If the preferred generation cannot be validated or fails during startup, QueueOS should provide a mechanism to select another suitable generation.

The previous last-known-good generation should remain available whenever practical.

The precise failure detection and confirmation process requires further specification.

## 12. OS Components and Database Ownership

The OS database contains the components necessary to operate QueueOS.

These components may include:

- Core OS functionality.
- Database engine.
- Lua runtime.
- Desktop engine.
- Zsh shell.
- System services.
- Configuration.
- System resources.
- Supporting metadata.

Components should be identifiable independently.

The database should maintain their relationships, versions and dependencies.

### 12.1 Component Updates

Component updates should be incorporated into a new OS generation.

The update process should not leave a generation containing an inconsistent mixture of incompatible component versions.

Where components are shared between generations, immutable object references may reduce unnecessary duplication.

### 12.2 Component Loading

The component manager should retrieve components through the normal database interface.

The physical storage representation should remain hidden from component consumers.

The loader should be responsible for interpreting component descriptors and applying the appropriate loading procedure.

## 13. Architecture Independence

The OS database format must remain independent of the processor architecture.

The same logical database structures should be interpretable by implementations targeting different supported architectures.

Architecture-specific executable objects may coexist within the database.

A generation may therefore contain architecture-specific components while retaining an architecture-independent database structure.

The generation manifest should identify any required architecture compatibility.

A bootstrap implementation should reject an incompatible component rather than attempting to execute it.

This separation allows the OS database format to remain stable even when the underlying processor architecture changes.

## 14. Integrity and Trust

Database integrity is fundamental to bootability.

The architecture should support validation of:

- Database identity.
- Metadata structures.
- Directory entries.
- Generation manifests.
- Component descriptors.
- Executable objects.
- Critical system resources.

Checksums provide a means of detecting accidental corruption.

Cryptographic hashes may additionally provide content identification and integrity verification.

Digital signatures are a potential future mechanism for establishing the authenticity of OS generations.

Integrity and authenticity are separate concerns.

A valid checksum does not establish that an image came from a trusted source.

The final trust and signing model belongs in the security specification.

## 15. Relationship to the Storage Engine

The OS database depends on the underlying QueueOS storage engine.

The distinction is:

**Storage engine:**

- Physical block management.
- Page and extent allocation.
- Transactions.
- WAL.
- Buffer pools.
- Object persistence.
- Recovery.

**OS database:**

- OS generation management.
- Boot directories.
- Generation manifests.
- Component descriptors.
- System resources.
- OS component ownership.
- Bootability and generation selection.

The OS database uses the storage engine to implement its persistence requirements.

The bootstrap reader accesses a limited subset of the underlying storage representation before the full engine is operational.

## 16. Outstanding Questions

| Area | Outstanding question |
|---|---|
| Block geometry | What is the final fixed physical block size? |
| Database identity | What are the exact magic identifiers and header layout? |
| Metadata | How are expandable metadata structures located and validated? |
| Boot directory | Is the initial JSON directory retained as the permanent representation? |
| Directory slots | Should three fixed 1 KB directory slots be used? |
| Encoding | What canonical encoding and validation rules will bootstrap use? |
| Generations | What is the exact generation numbering and lifecycle model? |
| Manifests | What fields are mandatory for a bootable generation? |
| Component descriptors | What is the common descriptor format? |
| Bootstrap | What is the final architecture-independent bootstrap contract? |
| Bootstrap size | Can the approximately 200 KB target be achieved across architectures? |
| Handoff | What is the precise transition contract to the full database engine? |
| Publication | What exact write ordering guarantees atomic generation publication? |
| Integrity | Which structures require checksums and which require cryptographic hashes? |
| Last-known-good | How is successful startup confirmed? |
| Reclamation | When can obsolete generations and shared objects be reclaimed? |
| Trust | How are signed or otherwise trusted generations established? |

## 17. Current Architectural Commitments

The following represent the current agreed design direction:

1. The OS database is the authoritative persistent representation of QueueOS.
2. The database must contain sufficient material to reconstruct and operate the OS.
3. The physical storage format uses a fixed block size.
4. Metadata structures should be expandable and identifiable through magic values.
5. A compact initial directory of approximately 1 KB is the proposed starting representation.
6. The directory should identify generations and provide their starting location, timestamp and size.
7. OS database generations are versioned.
8. Committed generations should be treated as immutable.
9. The bootstrap reader should understand only the structures necessary to locate and load the next initialization stage.
10. The architecture-specific bootstrap implementation has an approximate 200 KB target.
11. The full database engine assumes responsibility for normal database operation and recovery.
12. New OS generations should be published without destroying the previous valid generation.
13. Preferred and last-known-good generations are separate concepts.
14. The database format must remain independent of processor architecture.

The three-slot directory, JSON representation, exact metadata layout, manifest format and publication protocol remain proposed implementation details pending validation.

