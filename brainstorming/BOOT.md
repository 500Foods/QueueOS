# QueueOS — Boot Architecture

**Document status:** Initial architectural specification  
**Revision:** 0.1  
**Scope:** Bootstrapping, system initialization, OS image selection and startup

## 1. Introduction

The QueueOS boot architecture is responsible for transitioning a powered-on computer into an operational QueueOS environment.

The boot process must establish the minimum execution environment necessary to access the OS database and initialize the operating system.

A fundamental design objective is to minimize the amount of functionality required outside the OS database.

Unlike conventional operating systems that depend on a collection of independently installed system files, QueueOS is designed around the principle that its OS database contains everything necessary to reach an operational system.

The boot architecture therefore provides the initial execution environment and the mechanisms necessary to locate, validate and activate the OS database.

## 2. Design Principles

The boot architecture should follow these principles:

1. **Minimal bootstrap:** Only the functionality required to establish the initial execution environment should reside outside the OS database.
2. **Database independence:** The OS database must contain sufficient material to reach an operational system without depending on an independently installed operating system.
3. **Architecture awareness:** Hardware initialization and processor-specific operations must be isolated from architecture-independent boot functionality.
4. **Recoverability:** An unsuccessful boot must not unnecessarily compromise the ability to start another valid OS image.
5. **User control:** Users should be able to select which OS image to start and establish persistent boot preferences.
6. **Simplicity:** The boot interface should remain lightweight and should not introduce unnecessary dependencies.
7. **Extensibility:** The architecture should accommodate additional boot options, recovery environments and future hardware platforms.

## 3. Boot Architecture Overview

The conceptual boot sequence consists of the following stages:

```text
+---------------------------+
|       Hardware            |
|       Initialization      |
+-------------+-------------+
              |
              v
+---------------------------+
|       Initial Boot        |
|       Environment         |
+-------------+-------------+
              |
              v
+---------------------------+
|       Boot Blocks         |
|       1, 2 and 3          |
+-------------+-------------+
              |
              v
+---------------------------+
|       OS Image            |
|       Discovery           |
+-------------+-------------+
              |
              v
+---------------------------+
|       OS Image            |
|       Selection           |
+-------------+-------------+
              |
              v
+---------------------------+
|       Database            |
|       Validation          |
+-------------+-------------+
              |
              v
+---------------------------+
|       OS Database         |
|       Activation          |
+-------------+-------------+
              |
              v
+---------------------------+
|       OS Component        |
|       Initialization     |
+-------------+-------------+
              |
              v
+---------------------------+
|       Operational         |
|       QueueOS             |
+---------------------------+
```

This represents the logical sequence rather than a finalized implementation or an exact number of executable stages.

## 4. Foundational Boot Blocks

An important part of the current design is the use of a small number of foundational boot blocks.

The initial implementation is expected to establish three such blocks.

Their purpose is to provide the minimum functionality necessary to transition from the machine's initial boot environment to execution of the OS database.

The blocks should be independently understandable and have clearly defined responsibilities.

### 4.1 Block 1

The first block establishes the initial execution environment.

It must provide the foundation upon which subsequent boot functionality can operate.

The exact responsibilities, implementation language, binary format and hardware dependencies remain to be specified.

### 4.2 Block 2

The second block extends the initial environment sufficiently to support the next stage of initialization.

Its precise responsibilities and relationship to the first block remain to be specified.

### 4.3 Block 3

The third block establishes the remaining functionality necessary to proceed toward OS database discovery, validation and activation.

The objective is to reach a point where the operating system can begin loading its own components from the database.

Its precise implementation remains an open design question.

### 4.4 Block independence

The foundational blocks should remain as small and predictable as practical.

The architecture should avoid duplicating functionality that belongs in the OS database.

The exact boundaries between blocks, including whether they are separate binaries, structured data or other executable representations, require further specification.

## 5. OS Image Discovery and Selection

QueueOS should support the discovery and selection of available OS images.

An OS image represents a bootable instance of QueueOS, including its associated OS database.

The boot environment should be able to identify available images and present them to the user.

Potential selection criteria include:

- Available OS images.
- Image version.
- Image identity.
- Image validity.
- Previous boot status.
- User-defined boot preference.

The discovery mechanism must not assume that every available image is necessarily bootable.

Image validation must occur before transferring execution to the selected image.

The precise image identification and storage mechanisms remain to be specified.

## 6. Boot Selection Interface

QueueOS should provide its own boot selection interface.

The intention is to provide a native QueueOS experience rather than simply reproducing the conventional GRUB interface.

The interface should be lightweight, reliable and usable before the full operating system has initialized.

### 6.1 Image selection

The user should be able to select an available OS image.

The interface should clearly identify the image that will be started.

Where multiple images exist, the user should be able to distinguish between them.

### 6.2 Timed default selection

The boot interface should support a configurable timeout.

If the user does not make a selection before the timeout expires, the configured default image should be started automatically.

This provides unattended startup while retaining the ability to select another image when necessary.

### 6.3 Default image

The default image should be identifiable independently of the order in which images are discovered.

The initial proposed behavior is to use the latest suitable image unless the user has explicitly established another preference.

The definition of latest, suitable and valid must be established in the image-management specification.

### 6.4 Persistent boot preference

Users should be able to designate an image as their permanent default.

A simple editor or selection mechanism should allow the user to choose an image and select an option equivalent to:

**Always boot this image.**

This preference should survive subsequent restarts.

The mechanism for storing and updating the preference remains to be determined.

## 7. Boot Configuration

Boot preferences may be represented using JSON.

This would provide a straightforward, human-readable representation of boot configuration.

Potential configuration properties include:

- Default OS image.
- Boot timeout.
- Automatic boot behavior.
- Preferred boot entry.
- Recovery preferences.

An illustrative configuration:

```json
{
  "default_image": "queueos-main",
  "timeout": 5,
  "automatic_boot": true
}
```

This is an example of the proposed configuration model, not a finalized schema.

The location, ownership, validation and persistence of boot configuration remain open questions.

In particular, the boot environment must not depend on a fully operational OS database merely to determine which database it should activate.

## 8. OS Database Validation

Before activating an OS image, QueueOS must establish that its associated database is suitable for startup.

Validation should consider:

- Image identification.
- Database integrity.
- Required foundational components.
- Compatibility with the current hardware architecture.
- Compatibility with the boot environment.
- Availability of required resources.

The validation mechanism should fail safely.

A database that cannot be established as valid should not automatically be treated as a bootable system.

Detailed integrity verification and recovery procedures belong in the storage specification.

## 9. OS Database Activation

Once an image has been selected and validated, the boot environment must establish access to its OS database.

This is a critical transition.

The database contains the operating system components required to complete initialization.

The activation process must establish the appropriate execution and storage interfaces without introducing a dependency on an external conventional OS installation.

The exact transition mechanism, memory layout, database mounting or activation semantics and ownership boundaries require further design.

## 10. System Initialization

After database activation, QueueOS begins initializing its own components.

This includes the components necessary to establish the operational environment.

Potential initialization responsibilities include:

- Core OS functionality.
- Database services and interfaces.
- Queue infrastructure.
- Scheduling and execution.
- System configuration.
- Lua runtime.
- Desktop engine.
- Zsh shell.
- Application interfaces.

The order of initialization and the dependencies between components must be defined in the system initialization specification.

The boot process should distinguish between essential functionality required for an operational system and optional functionality that can be initialized later.

## 11. Failed Boot and Recovery

QueueOS must account for unsuccessful startup attempts.

Potential failure scenarios include:

- Invalid OS image.
- Database corruption.
- Missing required components.
- Incompatible hardware.
- Failed initialization.
- Interrupted updates.

A failed boot should not prevent the user from selecting another available OS image.

Where possible, the boot environment should preserve access to known-good images.

Recovery mechanisms should be designed to minimize the risk of rendering all available OS images unusable.

The architecture should eventually support a dedicated recovery environment or equivalent recovery mechanism.

## 12. Updates and Multiple OS Images

The boot architecture should accommodate multiple OS images.

This provides the foundation for:

- Testing new OS versions.
- Retaining previous versions.
- Recovery after unsuccessful upgrades.
- Selecting alternative system configurations.
- Development and experimental environments.

Updates should not unnecessarily destroy the currently operational image before the replacement has been validated.

The exact image lifecycle, update process and rollback mechanisms will be specified separately.

## 13. Architecture Portability

The boot architecture must support multiple processor architectures.

Hardware-specific initialization should be isolated from the architecture-independent portions of the boot process.

QEMU/KVM will provide the initial virtualization environment for testing.

The architecture should eventually accommodate physical hardware platforms with differing firmware, boot protocols, processor capabilities and memory layouts.

Support for individual hardware platforms must be validated independently.

Apple hardware, including Apple Silicon, is an area of interest, but its boot requirements and compatibility must be investigated rather than assumed.

## 14. Outstanding Questions

The following questions require further design and implementation work.

| Area | Outstanding question |
|---|---|
| Boot blocks | What are the precise responsibilities of blocks 1, 2 and 3? |
| Block representation | Are blocks separate executables, structured records or another representation? |
| Firmware | What initial firmware interfaces will be supported? |
| Hardware | Which processor architectures and platforms are targeted initially? |
| Image discovery | How are available OS images identified? |
| Image identity | What uniquely identifies an OS image? |
| Default selection | How is the latest suitable image determined? |
| Configuration | Where is boot configuration stored? |
| Boot UI | What rendering and input facilities are available before database activation? |
| Validation | How is database integrity established during startup? |
| Activation | How is execution transferred into the OS database environment? |
| Recovery | What functionality must remain available after a failed boot? |
| Updates | How are multiple images maintained and rolled back? |

## 15. Current Architectural Commitments

The following represent the current agreed design direction:

1. QueueOS will have its own boot experience rather than simply adopting GRUB's user interface.
2. The initial bootstrap will establish three foundational boot blocks.
3. The bootstrap must provide the minimum functionality necessary to activate the OS database.
4. The OS database contains the components necessary to reach an operational operating system.
5. Available OS images should be discoverable and selectable.
6. Boot selection should support a configurable timeout and automatic default selection.
7. Users should be able to establish a persistent default OS image.
8. JSON is a candidate representation for boot preferences.
9. Multiple OS images should be accommodated.
10. Recovery and rollback must be considered fundamental boot architecture concerns.

The detailed implementation of these commitments remains subject to the specifications and experiments described above.

