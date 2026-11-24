# HARDWARE.md

## 1. Purpose

This document describes how QueueOS interacts with physical and virtual hardware.

It establishes the intended boundary between hardware-specific code and the architecture-independent operating system.

QueueOS should be capable of supporting multiple CPU architectures and hardware platforms without requiring the core OS architecture, database format, queue model, or application-facing ABI to be redesigned for each platform.

The objective is not to hide every hardware difference. Some differences are fundamental and must be handled explicitly. The objective is to isolate those differences behind well-defined interfaces.

## 2. Hardware architecture principles

The hardware layer should follow these principles:

- Hardware-specific implementation details should be isolated from architecture-independent OS functionality.
- CPU architecture support should not dictate the OS Database format.
- Applications should not need to understand physical device details.
- Hardware resources should be exposed through controlled OS interfaces.
- The OS should be able to operate in virtualized environments as well as on physical hardware.
- Hardware support should be modular wherever practical.
- The initial hardware target should remain small enough to make early development and testing manageable.

Hardware abstraction must not introduce unnecessary overhead. Direct hardware access should remain possible within trusted low-level components when required.

## 3. Supported CPU architectures

QueueOS is intended to support multiple CPU architectures.

The initial architecture targets should be selected based on:

- Availability of development hardware.
- Availability and maturity of virtualization.
- Firmware and boot support.
- Compiler and toolchain availability.
- Device driver availability.
- Debugging and testing capabilities.
- Expected deployment environments.

Potential architecture targets include:

- x86-64.
- ARM64 (AArch64).
- RISC-V 64-bit.

This is a candidate list, not a finalized support commitment.

### 3.1 Architecture-specific code

Some OS components will necessarily be architecture-specific.

Examples include:

- Early boot and CPU initialization.
- Exception and interrupt entry.
- Page-table setup.
- Context switching, if required by the execution model.
- Atomic operations and memory barriers.
- CPU feature detection.
- Low-level timer access.
- Cache maintenance.
- Architecture-specific device access.

These components should be isolated behind narrow interfaces wherever practical.

### 3.2 Architecture-independent components

The following should remain independent of CPU architecture:

- OS Database logical structures.
- Persistent storage format.
- Generation and metadata concepts.
- Queue semantics.
- Service interfaces.
- Application-facing ABI semantics.
- Application manifests and metadata.
- Security and Rights models.

The same logical OS Database should be usable across supported architectures, subject to the availability of compatible executable components.

## 4. Boot and firmware interfaces

QueueOS must interact with platform firmware to initialize the machine and locate its boot environment.

Relevant interfaces may include:

- UEFI.
- ACPI.
- Device Tree.
- Platform-specific firmware interfaces.
- Virtual machine firmware.

The initial implementation should prioritize a well-supported firmware environment rather than attempting to support every legacy boot mechanism.

UEFI is a strong candidate for the initial x86-64 and ARM64 targets.

The exact firmware requirements for each architecture must be documented separately.

### 4.1 Firmware responsibilities

Firmware may provide information about:

- Available physical memory.
- Memory regions reserved by firmware.
- CPU topology.
- Device configuration.
- Interrupt routing.
- Timers.
- Boot devices.
- Hardware descriptions.
- Platform capabilities.

QueueOS should capture the information it needs before relinquishing firmware services or modifying the environment in which those services operate.

### 4.2 Bootloader boundary

The bootloader and early bootstrap code are responsible for getting the OS into a state where its native initialization code can take over.

The boot process is described separately in `BOOT.md` and `DATABASE.md`.

Architecture-specific boot code should be kept as small as practical.

The initial bootstrap target is approximately 200 KB, although this is a design target rather than a measured or finalized size limit.

## 5. Memory architecture

Memory management is a foundational hardware responsibility.

The OS must establish a consistent representation of physical and virtual memory across supported architectures.

### 5.1 Physical memory

The hardware layer must identify usable and reserved physical memory regions.

Memory should be classified according to its purpose and accessibility.

Relevant categories may include:

- Usable system RAM.
- Firmware-reserved regions.
- Memory-mapped device regions.
- Bootloader and bootstrap allocations.
- Reserved kernel allocations.
- Hardware-specific reserved areas.

The exact memory map representation remains to be defined.

### 5.2 Virtual memory

QueueOS is expected to use virtual addressing where supported by the architecture.

The hardware layer must provide the mechanisms required to establish and maintain address spaces.

The design must account for:

- Address-space creation.
- Mapping and unmapping.
- Access permissions.
- Memory protection.
- Shared memory.
- Memory-mapped devices.
- Page faults.
- Address-space isolation.

The relationship between address spaces and the queue execution model remains an important design decision.

### 5.3 Memory protection

Memory protection is a fundamental security requirement.

Applications and services should not be able to access arbitrary memory belonging to other components.

The OS must be able to enforce appropriate read, write, and execute permissions.

Hardware-assisted protection should be used wherever available.

### 5.4 Shared memory

QueueOS should avoid unnecessary data copying.

Shared memory may provide an efficient way for clients and services to exchange larger payloads.

However, shared memory must have explicit ownership, access rights, lifetime, and synchronization semantics.

The exact shared-memory model should be designed together with the ABI and queue architecture.

### 5.5 RAM-backed storage

The storage architecture should support memory-backed storage facilities, including the proposed `RAM:` namespace.

This is a logical storage feature rather than a requirement that all physical memory be exposed as a conventional block device.

The distinction between ordinary memory allocation, shared memory, and RAM-backed files or objects must remain clear.

## 6. CPU capabilities

The OS must identify and use CPU capabilities appropriately.

Potential capabilities include:

- Atomic instructions.
- Memory-ordering primitives.
- Hardware virtualization.
- Cryptographic acceleration.
- SIMD and vector instructions.
- Hardware random-number generation.
- Performance counters.
- Memory protection extensions.

Capabilities must be detected rather than assumed.

An optional CPU feature should not become a requirement for the entire OS unless the supported platform specification explicitly requires it.

### 6.1 Architecture-specific optimization

The OS may use architecture-specific optimizations where they provide measurable benefits.

However, optimized implementations must preserve the same logical behavior and data formats.

For example, an accelerated hashing implementation should produce the same result as its portable implementation.

The system should retain a portable implementation where doing so is practical.

Architecture-specific optimization should be driven by benchmarks rather than assumptions.

## 7. Interrupts and exceptions

The OS must handle hardware interrupts and CPU exceptions.

These are distinct mechanisms:

- Interrupts report asynchronous hardware or platform events.
- Exceptions report conditions arising during CPU execution.

The hardware layer must provide a consistent internal mechanism for handling both.

### 7.1 Interrupt handling

Potential interrupt sources include:

- Timers.
- Storage devices.
- Network interfaces.
- Input devices.
- USB controllers.
- Other peripheral hardware.

Interrupt handling should be efficient.

Time-consuming work should generally be deferred to an appropriate execution context rather than performed entirely in the interrupt handler.

This is particularly relevant to the queue architecture: hardware events may result in work being submitted to queues for later processing.

The exact relationship between interrupt handling and queue submission remains unresolved.

### 7.2 Interrupt controllers

Different architectures and platforms use different interrupt-controller implementations.

Examples include:

- x86 APIC and IOAPIC.
- ARM Generic Interrupt Controller (GIC).
- Platform-specific interrupt controllers.

The hardware layer should isolate these differences behind a common internal interface where possible.

### 7.3 Exceptions

Exceptions requiring OS attention include:

- Page faults.
- Invalid instructions.
- Protection violations.
- Arithmetic exceptions.
- General protection faults.
- Hardware errors.

The OS must distinguish between recoverable conditions, application faults, and failures that threaten system integrity.

The exact fault-handling and reporting model remains to be defined.

## 8. Timers and clocks

QueueOS requires reliable time facilities.

These are necessary for:

- Scheduling.
- Timeouts.
- Request expiration.
- Periodic operations.
- Service maintenance.
- Logging.
- User-visible time.
- Date and calendar functionality.

The hardware layer must distinguish between monotonic time and wall-clock time.

### 8.1 Monotonic time

Monotonic time is suitable for measuring elapsed time and implementing timeouts.

It should not move backward due to changes in the system clock.

### 8.2 Wall-clock time

Wall-clock time represents calendar time.

It may be initialized from firmware, an RTC, network time synchronization, or another source.

Wall-clock adjustments should not invalidate monotonic timing operations.

### 8.3 Timer hardware

Potential timer sources include:

- x86 TSC and platform timer facilities.
- HPET.
- Local APIC timers.
- ARM Generic Timer.
- Virtual machine timers.

The OS should select appropriate timer sources based on platform capabilities and reliability.

## 9. Device architecture

Devices should be represented through a consistent OS-managed interface.

Applications should not need to know the hardware register layout, bus protocol, or physical connection details of a device.

The OS should expose device functionality through appropriate services.

### 9.1 Device discovery

The OS must be able to identify available devices and determine how they are connected.

Discovery may use:

- PCI and PCI Express enumeration.
- USB enumeration.
- ACPI tables.
- Device Tree.
- Firmware-provided information.
- Platform-specific discovery mechanisms.

### 9.2 Device drivers

Drivers provide the implementation-specific interaction between the OS and hardware.

A driver may be responsible for:

- Device initialization.
- Configuration.
- Request submission.
- Interrupt handling.
- DMA management.
- Error reporting.
- Device shutdown.

The architecture should permit drivers to be updated or replaced without requiring unrelated OS components to change.

Whether drivers execute in a dedicated isolated environment or within a more privileged native OS context remains undecided.

### 9.3 Device access and security

Applications should access devices through authorized OS interfaces rather than receiving unrestricted hardware access.

Device access must be governed by the Rights model.

Direct device access may be appropriate for selected trusted components, but it should not be the default application interface.

## 10. Storage hardware

Storage devices are an important hardware category, but their physical characteristics should be separated from the OS Database format and logical storage model.

Potential devices include:

- SATA drives.
- NVMe drives.
- USB storage.
- Virtual disks.
- Other block-addressable storage devices.

The hardware layer should provide the operations necessary for the storage engine to access physical storage.

These operations may include:

- Device discovery.
- Capacity reporting.
- Block reads and writes.
- Flush and durability operations.
- Error reporting.
- Device removal and availability changes.

The storage engine and OS Database are described in `STORAGE.md` and `DATABASE.md`.

The physical storage interface must not make the persistent OS Database format architecture-dependent.

## 11. Networking hardware

Networking devices should be managed by the OS and exposed through network services.

The hardware layer may need to support:

- Ethernet controllers.
- Wi-Fi adapters.
- Virtual network interfaces.
- Other supported network devices.

Drivers handle device-specific behavior, while higher-level networking services provide common functionality to applications.

Applications should not need to understand the underlying network controller.

The network stack and its service interfaces remain separate design topics.

## 12. Input and output devices

QueueOS should support a consistent model for input and output hardware.

Potential device categories include:

- Keyboards.
- Pointing devices.
- Touchscreens.
- Displays.
- Audio devices.
- Cameras.
- Printers.
- Game controllers.

Hardware-specific details should remain within the relevant driver and service layers.

For example, an application should request input events or display operations through an OS interface rather than directly manipulating controller registers.

## 13. DMA and hardware access

Direct Memory Access (DMA) can improve device performance by allowing hardware to transfer data without requiring the CPU to copy every byte.

However, DMA introduces important security and memory-management considerations.

The OS must account for:

- DMA-capable devices.
- Buffer ownership.
- Memory pinning or equivalent guarantees.
- IOMMU availability.
- Device isolation.
- Cache coherency.
- Transfer completion and error handling.

Where supported, an IOMMU should be used to constrain device access to authorized memory regions.

The exact DMA buffer model should be coordinated with the memory and queue architectures.

## 14. Multiprocessor systems

QueueOS should support systems with multiple CPU cores.

The hardware layer must provide the facilities needed to identify and initialize available processors.

Relevant concerns include:

- CPU discovery.
- Processor startup.
- Inter-processor communication.
- Atomic operations.
- Memory ordering.
- Cache coherency.
- Interrupt distribution.
- Processor affinity.

The execution model must eventually define how work is distributed across processors.

However, the existence of multiple CPU cores does not itself require a conventional thread-per-task model.

Queue scheduling, processor assignment, and execution-context management should be designed together.

## 15. Virtualization

Virtual machines are an important development and testing target.

QueueOS should be capable of running in a virtualized environment without requiring the physical hardware features of a particular machine.

Virtualization provides a repeatable environment for:

- Boot testing.
- Database recovery testing.
- Architecture-specific validation.
- Memory-management testing.
- Device-driver development.
- Automated installation tests.
- Failure and recovery testing.

### 15.1 Initial virtualization targets

QEMU/KVM is a strong candidate for the primary development and testing environment.

It provides a practical way to test multiple machine types and CPU architectures.

Potential configurations include:

- x86-64 with UEFI.
- ARM64 with UEFI.
- RISC-V virtual machines, as support matures.

These are targets for investigation, not a promise that every configuration will be supported immediately.

### 15.2 Virtual devices

Virtual machines provide devices that may differ from physical hardware.

QueueOS should support a small set of well-defined virtual devices initially.

The exact device selection should prioritize simplicity, reliability, and availability across the chosen virtualization environments.

### 15.3 Physical hardware validation

Virtualization is not a substitute for physical hardware testing.

Real devices may expose differences in:

- Firmware behavior.
- Interrupt routing.
- DMA.
- Memory topology.
- Device initialization.
- Power management.
- Timing.
- Hardware-specific bugs.

Physical validation should follow once the fundamental architecture and virtualized test environment are stable.

## 16. Hardware abstraction and performance

Hardware abstraction should simplify portability without creating unnecessary layers.

The design should distinguish between:

- Hardware-specific mechanisms.
- Architecture-independent OS interfaces.
- Service-level functionality.
- Application-facing operations.

An abstraction is useful when it provides a stable boundary or eliminates repeated platform-specific logic.

It is not automatically beneficial merely because it hides an implementation detail.

Performance-sensitive paths should be measured, especially for:

- Queue submission and completion.
- Memory allocation.
- Context transitions.
- Interrupt handling.
- Storage I/O.
- Networking.
- Shared-memory transfers.

Where a portable abstraction imposes measurable overhead, architecture-specific implementations may be appropriate behind the same logical interface.

## 17. Open design questions

The following decisions remain open:

1. Which CPU architecture will be the first supported target?
2. Which additional architectures are realistic near-term targets?
3. What firmware interface is mandatory for each target?
4. What is the exact physical memory map representation?
5. How are address spaces and memory protection exposed internally?
6. What is the relationship between queues and shared-memory regions?
7. Which interrupt-controller interfaces are required initially?
8. What is the standard timer abstraction?
9. Which device buses and controllers are required for the initial release?
10. What is the driver execution and isolation model?
11. How are DMA buffers allocated, owned, and protected?
12. What is the initial virtual device set?
13. How much hardware-specific code is permitted outside the architecture layer?
14. What hardware capabilities are mandatory versus optional?

These questions should be resolved as implementation targets become concrete.

## 18. Current architectural commitments

The following principles are established or strongly intended:

- QueueOS is intended to support multiple CPU architectures.
- The OS Database format should be architecture-independent.
- The application-facing ABI should remain consistent across supported architectures.
- Architecture-specific code should be isolated wherever practical.
- Hardware access should be managed by the OS.
- Applications should interact with devices through authorized OS interfaces.
- Memory protection is a fundamental security requirement.
- Interrupt handling should avoid unnecessary work in interrupt context.
- Monotonic time and wall-clock time must be treated separately.
- Virtualization is a first-class development and testing environment.
- QEMU/KVM is a strong candidate for the initial virtualized development environment.
- Hardware-specific optimizations must preserve logical compatibility and should be benchmark-driven.

The exact supported hardware list, driver architecture, memory model, and low-level interfaces remain open.
