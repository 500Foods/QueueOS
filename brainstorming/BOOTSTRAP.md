# BOOTSTRAP.md

## 1. Purpose

This document describes the early startup mechanism used by QueueOS to locate, validate, and load an operational OS Database generation.

The bootstrap is the small, architecture-specific component responsible for bridging the gap between platform firmware and the full QueueOS environment.

Its primary responsibility is to locate a usable system generation and load the next stage. It should not attempt to implement the complete OS Database engine or reproduce functionality that belongs to the fully initialized operating system.

The bootstrap must be:

- Small.
- Predictable.
- Independent of the host operating system.
- Capable of validating essential metadata.
- Able to select a usable OS generation.
- Compatible with the architecture-independent storage format.
- Simple enough to audit and test thoroughly.

The broader boot process is described in `BOOT.md`. The persistent structures and generation model are described in `DATABASE.md`.

## 2. Design principles

The bootstrap design follows several principles.

### 2.1 Minimize complexity

The bootstrap should perform only the operations necessary to locate and load the next stage.

It should not contain a full database engine, general-purpose filesystem, application runtime, or unnecessary user-interface functionality.

### 2.2 Architecture-independent storage

The physical storage layout and logical metadata structures should be architecture-independent.

Architecture-specific code is responsible for interpreting the hardware environment and loading the appropriate executable components.

The same OS Database should be usable by different supported CPU architectures, provided the required executable components are available.

### 2.3 Preserve previous working generations

Updating the OS must not require destroying the currently usable generation before its replacement has been committed.

The bootstrap should be able to identify a previously committed generation when a newer candidate is incomplete or invalid.

### 2.4 Keep the bootstrap replaceable

The bootstrap should not become a permanent dependency for normal OS operation.

Once the full OS Database engine and native OS environment are running, they should take over the responsibilities that require more sophisticated storage, validation, recovery, and management.

## 3. Physical storage foundation

QueueOS uses fixed-size physical storage blocks as the foundation of its persistent format.

The physical block size must be established by the storage format and must not depend on the CPU architecture.

The bootstrap should operate using this fundamental block interface rather than assuming a conventional partition layout or filesystem.

### 3.1 Metadata blocks

The storage format includes expandable metadata blocks identified by magic values.

This allows metadata structures to grow without requiring the physical block size to change.

The bootstrap should recognize the metadata structures it requires and safely ignore structures that are outside its responsibilities.

Unknown or unsupported metadata must not be interpreted as if it were understood.

### 3.2 Initial layout

The initial storage layout is expected to contain:

1. A volume or database identification structure.
2. Bootstrap metadata describing the available OS generations.
3. Generation-specific metadata describing the components required to load the system.

The exact physical arrangement and boundaries remain under development.

The initial bootstrap is expected to use two foundational blocks, with a possible third block depending on the final division of responsibilities.

The number of blocks is not yet a finalized commitment.

## 4. Boot directory

The bootstrap needs a compact structure that tells it where to find the available OS generations.

The current proposal is an initial boot directory of approximately 1 KB.

JSON is the preferred candidate representation for this directory because it is simple to inspect, easy to generate, and useful during early development.

The final representation has not yet been established.

### 4.1 Directory responsibilities

The boot directory should provide enough information to locate and evaluate candidate generations without requiring the full database engine.

It is expected to include information such as:

- Directory format version.
- Generation identifier.
- Generation start location.
- Generation timestamp.
- Generation size.
- Relevant metadata references.

The precise field names, types, encoding, and validation rules remain open.

### 4.2 Directory expansion

The initial directory must remain small enough for the bootstrap to read and process easily.

If future requirements exceed the initial capacity, the metadata design should allow additional information to be stored in expandable metadata blocks.

The bootstrap should not need to understand every possible future metadata extension.

### 4.3 Directory validation

The bootstrap must validate the directory before using its contents.

Validation should include:

- Recognized format identification.
- Supported format version.
- Valid field types and values.
- Valid generation references.
- Bounds checking.
- Detection of incomplete or corrupted metadata.

A malformed directory must not cause arbitrary storage reads or uncontrolled memory access.

The bootstrap should reject information it cannot safely interpret.

## 5. Generation model

QueueOS uses generations to represent committed versions of the OS Database.

A generation is a logically consistent collection of system state and components that can be selected for startup.

The bootstrap uses generation metadata to identify which system version it should attempt to load.

### 5.1 Generation identity

Each generation must have an identity that distinguishes it from other generations.

Generation identifiers should not depend on the CPU architecture.

The bootstrap should use generation identity and commit metadata rather than relying exclusively on timestamps to determine validity or preference.

### 5.2 Committed generations

A generation must not be considered bootable merely because its metadata exists.

It must have reached a recognized committed state.

The exact commit representation and validation protocol will be defined by the database design.

The bootstrap must be able to distinguish a committed generation from an incomplete update.

### 5.3 Generation selection

The bootstrap should evaluate available generations and select an appropriate candidate.

The intended behavior is to prefer the newest valid committed generation.

However, a newer generation that fails validation or cannot be loaded must not prevent the system from attempting an older usable generation.

Generation selection should therefore distinguish:

- Generation preference.
- Generation validity.
- Generation availability.
- Last-known-good status.

These concepts should not be collapsed into a single timestamp or flag.

### 5.4 Last-known-good generation

The system should maintain a distinction between the preferred generation and the last-known-good generation.

The preferred generation is the one the system intends to boot under normal conditions.

The last-known-good generation provides a fallback when the preferred generation cannot be loaded or does not complete startup successfully.

The exact mechanism for confirming successful startup and updating last-known-good state remains to be defined.

## 6. Bootstrap stages

The bootstrap is expected to operate in a small number of stages.

The precise boundaries remain open, but the intended responsibilities are as follows.

### 6.1 Stage 0: Platform entry

The platform firmware loads the initial architecture-specific bootstrap code and transfers execution to it.

At this point, the bootstrap has only the facilities provided by the firmware and its own minimal implementation.

Its responsibilities include:

- Establishing a safe initial execution environment.
- Identifying the boot device or storage source.
- Determining the available hardware information needed for the next stage.
- Locating the QueueOS storage structures.

The exact firmware interface and entry requirements depend on the target architecture.

### 6.2 Stage 1: Metadata discovery

The initial bootstrap reads the foundational storage structures.

It identifies the QueueOS database and locates the boot directory.

It then validates the directory sufficiently to determine which generation metadata can be examined.

This stage should avoid loading the full database engine.

### 6.3 Stage 2: Generation selection

The bootstrap evaluates the available generation information.

It identifies valid committed candidates and selects the preferred generation according to the selection rules.

If the preferred generation is invalid or unavailable, it should attempt an appropriate fallback.

The selection process must be deterministic and must not depend on undefined metadata behavior.

### 6.4 Stage 3: Component loading

Once a generation has been selected, the bootstrap locates and loads the components required for the next stage.

The generation metadata must describe the relevant component locations and loading requirements.

The bootstrap should validate the information it uses and avoid loading arbitrary or out-of-bounds data.

The precise component dependency and loading model remains to be defined.

### 6.5 Stage 4: Handoff

The bootstrap transfers control to the next-stage component.

It must provide the information required for the next stage to continue initialization.

This may include:

- Selected generation identity.
- Storage device information.
- Memory map.
- Loaded component locations.
- Relevant firmware and hardware information.
- Bootstrap diagnostics.

The final handoff structure and calling convention will be defined by the architecture-specific implementation and ABI work.

## 7. Bootstrap size

The current target for the architecture-specific bootstrap is approximately 200 KB.

This is a design target intended to encourage a small, focused implementation.

The bootstrap should not grow into a general-purpose OS environment.

The size target should be evaluated through actual builds as functionality is added.

If the bootstrap becomes significantly larger, its responsibilities should be reviewed before simply increasing the target.

The target is not yet a strict binary size limit.

## 8. Architecture-specific implementation

The bootstrap necessarily contains architecture-specific code.

Different CPU architectures have different requirements for:

- Firmware entry.
- Initial CPU state.
- Memory initialization.
- Address translation.
- Executable loading.
- Calling conventions.
- Handoff to the next stage.

These differences should be isolated from the architecture-independent storage format.

A separate bootstrap implementation may exist for each supported architecture while operating on the same logical database structures.

### 8.1 Shared metadata interpretation

Where practical, the interpretation and validation of storage metadata should be shared across architectures.

This reduces the risk of different bootstrap implementations interpreting the same database differently.

However, shared code must not introduce dependencies that make the bootstrap unnecessarily large or difficult to build.

### 8.2 Executable format

The executable format used by QueueOS remains a separate design decision.

ELF has been considered as a possible executable container, but no commitment has been made to Linux executable compatibility or the Linux ABI.

The bootstrap must support whatever executable format is ultimately selected for the relevant architecture.

## 9. Error handling and recovery

The bootstrap operates before the full OS environment is available, so its error-handling facilities must be deliberately limited.

It should distinguish between failures such as:

- Missing or unrecognized QueueOS metadata.
- Invalid boot directory.
- Unsupported metadata version.
- No committed generations.
- Invalid generation metadata.
- Missing components.
- Corrupted component data.
- Unsupported executable format.
- Insufficient memory.
- Storage read failures.

Where possible, the bootstrap should attempt a valid fallback generation.

If no usable generation exists, it should stop safely and provide a meaningful diagnostic.

It must not silently select an incomplete generation merely to continue booting.

## 10. Integrity and trust

The bootstrap must establish sufficient confidence in the metadata and components it loads.

At minimum, it must perform structural validation and bounds checking.

Additional integrity mechanisms may include:

- Checksums.
- Cryptographic hashes.
- Signed generation metadata.
- Signed executable components.
- Trust anchors.
- Rollback protection.

Cryptographic verification is an important future security consideration, but the exact trust model has not yet been established.

The bootstrap must not assume that a structurally valid generation is necessarily trusted.

The distinction between integrity, authenticity, and successful startup should remain explicit.

## 11. Updating the bootstrap and OS

Updating QueueOS should preserve the ability to boot a previously working generation.

A replacement generation should be prepared and validated before it becomes the preferred generation.

The publication mechanism must ensure that an interrupted update does not leave the database pointing exclusively to an incomplete generation.

The exact publication protocol belongs to `DATABASE.md`.

Updating the bootstrap itself is a separate concern because it may be required before the full OS Database can be accessed.

The design should eventually address:

- Bootstrap version compatibility.
- Metadata format compatibility.
- Recovery from interrupted bootstrap updates.
- Architecture-specific bootstrap replacement.
- Compatibility with older committed generations.

The initial implementation should avoid introducing a complicated self-updating bootstrap mechanism before the basic generation model is working.

## 12. User interaction and boot selection

The long-term boot experience should provide a QueueOS-native interface rather than exposing a conventional bootloader interface as the final user experience.

The intended direction includes:

- A default generation selected automatically.
- A timed default boot.
- A way to select an alternative generation or image.
- A persistent preference such as "always boot this one".

The selected default may eventually be represented in metadata or a small configuration structure.

This user interface is not a requirement for the initial bootstrap implementation.

The early implementation should prioritize reliable generation discovery, selection, and loading.

The final boot interface and its configuration mechanism remain open.

## 13. Testing

The bootstrap should be tested independently of the fully running OS.

Testing should include:

- Valid initial metadata.
- Missing metadata.
- Invalid magic values.
- Unsupported metadata versions.
- Corrupted boot directories.
- Multiple committed generations.
- Incomplete generations.
- Invalid generation references.
- Missing components.
- Corrupted components.
- Interrupted updates.
- Fallback to an older generation.
- Insufficient memory.
- Storage read failures.

The test suite should include deliberately malformed metadata to verify that the bootstrap rejects invalid inputs safely.

### 13.1 Virtualized testing

QEMU/KVM should be used for repeatable bootstrap testing wherever practical.

Automated tests should be able to construct storage images with known metadata and generation states.

The bootstrap should be tested against both successful and unsuccessful boot scenarios.

Physical hardware testing should follow once the basic boot process is reliable in the virtualized environment.

### 13.2 Architecture coverage

Each supported architecture must have its own bootstrap tests.

Shared metadata test cases should be reused across architectures to verify that all implementations interpret the same storage structures consistently.

## 14. Open design questions

The following decisions remain open:

1. What is the exact physical layout of the foundational bootstrap blocks?
2. Will the initial design use two blocks or three?
3. What is the final boot directory representation?
4. Which fields are mandatory in the boot directory?
5. How are expandable metadata blocks linked and validated?
6. What is the exact generation manifest format?
7. How is a committed generation identified?
8. How is last-known-good state maintained?
9. What constitutes a successful boot?
10. What is the exact boundary between bootstrap stages?
11. What executable format will be used?
12. How are component dependencies represented?
13. What is the final bootstrap-to-next-stage handoff structure?
14. What integrity and signature mechanisms are required?
15. How is the bootstrap itself updated safely?
16. What is the initial architecture and firmware target?

These decisions should be made as the storage format and first bootable implementation mature.

## 15. Current architectural commitments

The following principles are established or strongly intended:

- The bootstrap is small and architecture-specific.
- The persistent storage format is architecture-independent.
- Physical storage uses fixed-size blocks.
- Metadata blocks are expandable and identified by magic values.
- The initial boot directory is expected to be approximately 1 KB.
- JSON is the current candidate representation for the boot directory.
- The boot directory identifies available generations and their locations.
- Generations provide the basis for selecting a consistent OS state.
- The bootstrap should prefer the newest valid committed generation.
- A previous usable generation should remain available as a fallback.
- The initial bootstrap is expected to use two foundational blocks, with a possible third.
- The bootstrap size target is approximately 200 KB.
- The full OS Database engine takes over more advanced storage and recovery responsibilities.
- The final boot interface should be QueueOS-native rather than dependent on a conventional bootloader UI.

The precise block layout, metadata encoding, executable format, generation validation protocol, and stage boundaries remain open.
