# QueueOS — Storage Architecture

**Document status:** Architectural specification / evolving design  
**Revision:** 0.1  
**Scope:** Physical storage, database-managed persistence, allocation, caching and durability

## 1. Introduction

QueueOS takes a database-centric approach to storage.

Rather than implementing a conventional filesystem and subsequently introducing a database to manage selected system data, QueueOS intends to provide a purpose-built database and storage engine as a fundamental operating system facility.

The database owns the underlying storage resources and provides the mechanisms through which persistent data is organized, accessed, modified and recovered.

This is a deliberate architectural choice.

The objective is to eliminate unnecessary layers between applications, system components and persistent storage while providing transactional consistency, efficient resource allocation and predictable performance.

Storage is therefore not simply a service provided by QueueOS. It is part of the operating system's foundational architecture.

## 2. Design Principles

The storage architecture follows these principles:

1. **Database-managed storage:** The database directly manages persistent storage rather than relying on a conventional filesystem as its primary organizational mechanism.
2. **Raw storage ownership:** The storage engine should be capable of managing underlying block devices directly.
3. **Unified persistence model:** System state, application data and other persistent objects should use a coherent database-managed storage model.
4. **Performance:** Storage operations should minimize unnecessary copying, serialization, allocation and I/O.
5. **Transactional integrity:** Persistent modifications should support well-defined consistency and recovery guarantees.
6. **Architecture independence:** The on-disk representation must not depend on native processor structures or byte ordering.
7. **Scalability:** The architecture should accommodate small embedded environments through larger systems with substantial storage resources.
8. **Recoverability:** Interrupted operations and unexpected power loss must not leave the storage engine in an indeterminate state.
9. **Hardware abstraction:** Hardware-specific storage and RAID functionality should remain below the database-managed storage layer.
10. **Efficiency over simplicity:** Additional implementation complexity is acceptable when it produces measurable improvements in performance, reliability or resource utilization.

## 3. Storage Architecture

The conceptual architecture consists of three principal layers.

### 3.1 Physical Storage Layer

The physical storage layer provides access to underlying storage hardware.

Potential devices include:

- NVMe SSDs.
- SATA SSDs.
- Conventional hard disks.
- USB storage.
- Virtual block devices.
- RAM-backed storage.
- Hardware or software RAID devices.

The database should interact with a consistent logical-block interface rather than depending on individual device implementations.

The physical layer is responsible for translating logical operations into hardware-specific operations.

### 3.2 Storage Engine

The storage engine manages physical allocation and persistent representation.

Its responsibilities include:

- Block management.
- Page allocation.
- Extent management.
- Tablespace management.
- Buffer pools.
- Caching.
- Journaling and recovery.
- Persistent object storage.
- Integrity verification.

The storage engine should own the allocation and persistence mechanisms rather than delegating those responsibilities to a conventional filesystem.

### 3.3 Database Layer

The database provides the logical representation of persistent QueueOS data.

Its responsibilities include:

- Structured records.
- Object references.
- Indexes.
- Metadata.
- Transactions.
- Component storage.
- Persistent system state.
- Queue-related persistence.
- User data.

The distinction between the storage engine and database is primarily architectural. They are intended to operate as integrated parts of the same OS facility.

## 4. Physical Storage Abstraction

The database should operate on a consistent block-device abstraction.

The initial interface should support:

| Operation | Purpose |
|---|---|
| Read | Retrieve a range of logical blocks |
| Write | Write a range of logical blocks |
| Flush | Ensure previously submitted writes reach the required durability boundary |
| Discard | Release unused storage ranges where supported |
| Information | Retrieve device geometry and capabilities |

Additional capabilities may include:

- Alignment requirements.
- Atomic write limits.
- Device identifiers.
- Logical block size.
- Physical block size.
- Flush and FUA support.
- Maximum transfer sizes.
- Device health information.

The interface should distinguish between capabilities guaranteed by the device and those provided by the storage implementation.

### 4.1 RAID and Device Composition

RAID functionality belongs below the database storage engine.

The database should not need to implement separate allocation or persistence policies for individual RAID configurations.

It should operate against the logical storage device presented by the underlying storage layer.

This maintains a consistent storage model regardless of the physical device arrangement.

## 5. Database Storage Organization

The storage engine should support multiple logical storage areas within a managed volume.

Tablespaces are a candidate mechanism for organizing these areas.

Potential initial tablespaces include:

| Tablespace | Intended contents |
|---|---|
| SYSTEM | Core OS metadata and configuration |
| EXECUTABLES | Executable components and associated metadata |
| SERVICES | System service definitions and state |
| QUEUES | Persistent queue data and related metadata |
| USERDATA | Persistent user objects |
| INDEXES | Database indexes |
| LOGS | Journals and recovery information |
| TEMP | Temporary database allocations |

These are proposed logical categories, not a finalized mandatory tablespace layout.

The purpose is to allow different storage requirements to be handled efficiently without introducing unrelated storage mechanisms.

For example, executable objects, transactional records and append-oriented queue data may benefit from different physical allocation strategies.

## 6. Pages and Extents

Pages and extents are candidate fundamental allocation units.

### 6.1 Pages

Pages provide a fixed-size unit of database storage and memory management.

Candidate initial page sizes include:

- 4 KB.
- 8 KB.
- 16 KB.

Different page sizes may provide advantages for different workloads.

Smaller pages can reduce unnecessary I/O for small records, while larger pages may improve sequential access and reduce metadata overhead.

The final page-size strategy should be determined through benchmarking.

Multiple page sizes or page classes may be appropriate if they provide measurable benefits without creating excessive implementation overhead.

### 6.2 Extents

Extents provide a mechanism for allocating groups of pages.

They should reduce allocation overhead and improve locality for larger objects and sequential workloads.

Extent allocation should support:

- Contiguous page allocation where practical.
- Efficient extension of existing objects.
- Reclamation of unused pages.
- Tracking of allocation ownership.
- Fragmentation management.

The exact extent geometry and allocation strategy remain open.

### 6.3 Allocation Strategies

Different data types may benefit from different physical allocation strategies.

Potential approaches include:

- Compact inline storage.
- Page-based allocation.
- Extent-based allocation.
- Append-oriented allocation.
- Immutable content-addressed objects.

These strategies should operate within a unified storage and transaction architecture.

They should not evolve into independent storage systems with incompatible recovery and consistency semantics.

## 7. Persistent Objects and Large Data

Not all data should necessarily be stored in the same physical representation.

Small structured objects may be stored inline within database pages.

Larger objects may benefit from dedicated object or blob storage.

The database should maintain the authoritative references and metadata for these objects.

Potential examples include:

- Executable binaries.
- Images and other media.
- Large application resources.
- Database-managed documents.
- User-generated content.
- Large serialized objects.

### 7.1 Object References

Large objects should be referenced through database-managed identifiers rather than exposing physical storage addresses to applications.

This allows the storage engine to relocate objects without invalidating application references.

### 7.2 Content Addressing

Content-addressed storage is a candidate optimization.

An object may be identified by a cryptographic digest of its contents.

This could provide:

- Duplicate object detection.
- Immutable executable identification.
- Efficient sharing of identical objects.
- Integrity verification.
- Reduced storage consumption.

SHA-256 is one candidate digest algorithm.

However, hashing every object on the synchronous write path introduces additional CPU overhead.

The performance implications must be benchmarked, particularly for smaller objects and systems with different hardware acceleration capabilities.

Content addressing should not become a mandatory overhead where it offers no meaningful advantage.

### 7.3 Immutable Objects

Executable components are candidates for immutable content-addressed storage.

An updated executable would be represented as a new object rather than modifying the existing object in place.

This could simplify integrity verification, rollback and sharing.

The exact relationship between immutable objects and transactional database updates requires further specification.

## 8. Transactions and Consistency

The database should provide transactional persistence.

ACID semantics are a proposed foundation:

- **Atomicity:** A transaction either commits completely or has no visible effect.
- **Consistency:** Transactions preserve defined database invariants.
- **Isolation:** Concurrent operations follow defined isolation semantics.
- **Durability:** Committed transactions survive failures according to their configured durability guarantees.

MVCC is a candidate concurrency-control mechanism.

It could allow readers to access consistent snapshots while writers make changes without unnecessarily blocking read operations.

The exact transaction and isolation model remains to be finalized.

### 8.1 Write-Ahead Logging

A write-ahead log (WAL) is the proposed primary mechanism for transactional recovery.

The fundamental requirement is that sufficient recovery information be made durable before modified database pages are considered committed.

The WAL should support:

- Transaction identification.
- Commit records.
- Sequence numbers.
- Integrity checks.
- Recovery after interruption.
- Checkpointing.
- Reclamation of obsolete log segments.

Checksums and sequence numbers should help detect incomplete or corrupted log records.

### 8.2 Group Commit

Group commit is a candidate optimization.

Multiple transactions can share a durability operation rather than independently forcing storage synchronization.

This can substantially improve throughput where storage flush operations are expensive.

### 8.3 Durability Options

Different workloads may have different durability requirements.

The database should eventually support explicitly defined durability policies.

For example, some operations may require synchronous durability before acknowledging success, while others may tolerate deferred persistence.

These guarantees must be explicit and must not imply stronger durability than the underlying hardware can provide.

## 9. Recovery and Integrity

Recovery is a fundamental storage requirement.

The storage engine must be able to reconstruct a consistent database state following:

- Unexpected power loss.
- Interrupted writes.
- Operating system crashes.
- Device errors.
- Incomplete transactions.
- Interrupted checkpoints.

The WAL provides one proposed mechanism for transaction-level recovery.

Additional mechanisms should include:

- Checksummed metadata.
- Generation numbers.
- Validated allocation structures.
- Redundant critical metadata.
- Detection of incomplete updates.

### 9.1 Checkpointing

Checkpointing should periodically establish a recoverable database state and reduce the amount of WAL that must be replayed.

The checkpoint process must preserve the ability to recover from interruption.

### 9.2 Metadata Redundancy

Critical metadata should be stored redundantly where practical.

Generation numbers and checksums can help identify the newest valid copy.

The exact superblock and metadata redundancy scheme will be specified in the OS database specification.

### 9.3 Power-Loss Testing

Power-loss and interrupted-write testing should be part of storage-engine validation.

Particular attention should be given to:

- Flush ordering.
- FUA semantics.
- Device write caches.
- Atomic write guarantees.
- Partial writes.
- Interrupted metadata updates.

Hardware capabilities must be treated as explicit inputs to the durability model.

## 10. Memory and Buffer Management

Memory management is particularly important in a database-centric operating system.

A high-performance storage engine should avoid unnecessary repeated reads and writes between persistent storage and memory.

### 10.1 Buffer Pools

The database should maintain managed buffer pools for frequently accessed pages.

These pools should support:

- Page caching.
- Dirty-page tracking.
- Replacement policies.
- Background writeback.
- Pinning and unpinning.
- Workload-aware allocation.

The buffer pool should be considered a fundamental performance component rather than an optional optimization.

### 10.2 Multiple Buffer Classes

Different workloads may benefit from separate buffer pools or cache classes.

For example:

- Frequently accessed system metadata.
- Executable pages.
- Structured records.
- Large object data.
- Queue and journal data.

The purpose is to prevent one workload from unnecessarily displacing another's useful cached data.

The final cache architecture should be guided by measurement.

### 10.3 RAM-backed Storage

A RAM-backed storage facility, provisionally called `RAM:`, is desirable.

It would provide a high-performance storage space for temporary or volatile data.

Potential uses include:

- Temporary objects.
- Scratch data.
- Transient queues.
- Intermediate processing results.
- Cached content.

A further possibility is lazy persistent backing.

Under this model, RAM-backed objects would initially consume memory and acquire persistent backing only when required.

This could reduce unnecessary storage activity.

The semantics of volatile versus persistent-backed RAM objects remain to be specified.

## 11. Queue and Journal Storage

Queue-oriented execution introduces storage workloads that differ from conventional document and record access.

Queue data may benefit from append-oriented storage.

Potential optimizations include:

- Sequential allocation.
- Batched writes.
- Reduced random I/O.
- Efficient reclamation.
- Journal-oriented persistence.
- Transactional publication of events.

Queue persistence must remain consistent with the overall database transaction model.

A transactional outbox mechanism is a candidate approach for ensuring that state changes and associated event publication cannot become inconsistent.

The exact persistence requirements will be defined in the execution and queue specifications.

## 12. Performance Considerations

QueueOS deliberately accepts storage-engine complexity where it produces meaningful performance improvements.

Performance evaluation should consider the complete storage path rather than isolated operations.

Important measurements include:

- Sequential read throughput.
- Sequential write throughput.
- Random read IOPS.
- Random write IOPS.
- Transaction throughput.
- Commit latency.
- Flush latency.
- Buffer-pool hit rates.
- Allocation overhead.
- Fragmentation.
- Recovery time.
- Memory consumption.
- CPU utilization.

The storage engine should be benchmarked against appropriate filesystem-based alternatives.

This is important because database-managed storage is an architectural decision that must demonstrate practical advantages rather than relying on assumptions.

### 12.1 Minimize Overhead

Particular attention should be given to:

- Avoiding unnecessary data copies.
- Efficient page access.
- Reducing allocation overhead.
- Batching physical I/O.
- Avoiding unnecessary synchronization.
- Efficient metadata management.
- Avoiding redundant integrity calculations.

Optimization should not compromise transactional correctness.

## 13. Architecture Independence

The persistent database format must be independent of the processor architecture.

The on-disk representation must not depend on:

- Native structure alignment.
- Compiler-specific padding.
- Native pointer sizes.
- Host byte ordering.
- Architecture-specific integer representations.

Persistent structures should use:

- Explicit byte ordering.
- Fixed-width integer fields.
- Explicit format versions.
- Defined field boundaries.
- Validated offsets.
- Checksums where appropriate.

Native pointers and memory addresses must never be persisted as though they were portable object references.

The database image format and database engine version are separate concepts.

An engine implementation may change without necessarily requiring an incompatible change to the persistent image format.

## 14. Storage and Bootstrapping

The storage architecture must account for the fact that QueueOS needs to read its OS database before the full database engine is operational.

The initial boot environment therefore requires a constrained database-reading capability.

This capability should be deliberately limited.

It needs to understand enough of the storage format to locate and load the next stage of initialization.

It should not need to implement:

- The full query engine.
- General-purpose transaction processing.
- Full database recovery.
- General application-facing storage APIs.

The bootstrap reader should operate read-only.

The full database engine takes responsibility for normal database operation after initialization.

This separation is important to avoid circular dependencies between the operating system and its database.

See `BOOT.md` and `OS_DATABASE.md` for the associated architectural requirements.

## 15. Outstanding Questions

The following subjects require further investigation and specification.

| Area | Outstanding question |
|---|---|
| Device interface | Exact logical-block API and capability model |
| Page sizes | Fixed size or multiple page classes? |
| Extents | Allocation geometry and reclamation strategy |
| Tablespaces | Final logical organization |
| Objects | Inline versus external blob allocation thresholds |
| Content addressing | When hashing is performed and how deduplication is managed |
| Hash algorithms | SHA-256 or alternative architecture-aware choices |
| Transactions | Exact isolation and concurrency model |
| WAL | Record format, sequencing and persistence guarantees |
| Checkpointing | Frequency, ordering and recovery semantics |
| Buffer pools | Allocation, replacement and memory-pressure behavior |
| RAM: | Volatile and lazy persistent-backing semantics |
| Queue storage | Append-oriented storage and reclamation |
| Integrity | Checksums, metadata redundancy and corruption detection |
| Compression | Whether and where compression provides a net benefit |
| Hardware | Flush, FUA and atomic-write guarantees |
| Recovery | Full recovery and consistency validation procedures |
| Performance | Benchmarking methodology and target workloads |

## 16. Current Architectural Commitments

The following represent the current agreed direction:

1. QueueOS uses a purpose-built database and storage engine.
2. The database directly manages its underlying storage resources.
3. The database replaces the conventional filesystem as the primary persistent storage model.
4. A unified database-managed persistence architecture is preferred over multiple independent storage policies.
5. Hardware and RAID abstraction belong below the database storage engine.
6. Storage efficiency is a primary design objective, even where achieving it requires additional implementation complexity.
7. The database must support a read-only bootstrap access mechanism.
8. Persistent storage formats must be architecture-independent.
9. Buffer pools and caching are fundamental to achieving acceptable performance.
10. A RAM-backed storage facility is a desirable part of the architecture.

Page sizes, tablespace organization, MVCC, WAL implementation details, content addressing, compression and allocation strategies remain subject to detailed design and benchmarking.

