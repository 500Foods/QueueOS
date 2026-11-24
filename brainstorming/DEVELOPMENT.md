# DEVELOPMENT.md

## 1. Purpose

This document describes the development environment, build process, testing strategy, and contribution practices for QueueOS.

QueueOS is a new operating system architecture. Development must therefore account for both conventional software engineering concerns and the additional requirements of low-level system development.

The initial priority is to establish a reproducible development environment capable of building, booting, testing, and debugging QueueOS in a virtual machine.

Physical hardware support should follow once the fundamental architecture and boot process are sufficiently stable.

## 2. Development principles

Development should follow these principles:

- Keep the build process reproducible.
- Prefer small, independently testable components.
- Keep architecture-specific code isolated.
- Avoid unnecessary dependencies.
- Make foundational code easy to inspect and debug.
- Test failure conditions, not just successful execution.
- Preserve architectural consistency across implementations.
- Measure performance rather than relying on assumptions.
- Document decisions and unresolved questions.
- Avoid introducing abstractions that do not provide a measurable or architectural benefit.

C is the preferred language for foundational native components.

Other languages may be used where they provide a clear advantage, provided they do not introduce unnecessary dependencies into the core OS.

## 3. Development environment

The development environment should support:

- Native compilation.
- Cross-compilation.
- Binary inspection.
- Storage image creation and inspection.
- Virtual machine execution.
- Automated testing.
- Debugging.
- Static analysis.
- Reproducible builds.

A Linux development environment is the initial target because of its mature compiler, debugging, and virtualization toolchains.

The exact supported development distributions remain open.

### 3.1 Recommended tools

The following tools are candidates for the initial development environment:

| Purpose | Candidate tools |
|---|---|
| Native compiler | GCC, Clang |
| Cross-compilation | Architecture-specific GCC/Clang toolchains |
| Assembler | GNU assembler, LLVM integrated assembler |
| Linker | GNU ld, LLD |
| Build system | Make |
| Debugging | GDB |
| Binary inspection | readelf, objdump, nm |
| Static analysis | Clang Static Analyzer, GCC warnings, sanitizers |
| Virtualization | QEMU/KVM |
| Firmware emulation | OVMF and architecture-specific UEFI firmware |
| Version control | Git |
| Continuous integration | GitHub Actions |

These are suggested tools, not mandatory dependencies for every component.

The build system should remain straightforward enough that a developer can understand what happens during a build without needing to understand a large framework.

## 4. Repository organization

The repository should make architectural boundaries visible.

The exact directory structure may evolve, but it should distinguish between:

- Architecture-independent OS code.
- Architecture-specific code.
- Bootstrap implementations.
- OS Database and storage code.
- Queue and execution infrastructure.
- Service implementations.
- Runtime support.
- Applications and utilities.
- Build tooling.
- Test code.
- Documentation.

A possible starting structure is:

```text
QueueOS/
├── README.md
├── AGENTS.md
├── docs/
│   ├── ARCHITECTURE.md
│   ├── BOOT.md
│   ├── BOOTSTRAP.md
│   ├── STORAGE.md
│   ├── DATABASE.md
│   ├── EXECUTION.md
│   ├── ABI.md
│   ├── SECURITY.md
│   ├── APPLICATIONS.md
│   ├── RUNTIMES.md
│   ├── SERVICES.md
│   ├── HARDWARE.md
│   └── DEVELOPMENT.md
├── src/
│   ├── common/
│   ├── arch/
│   │   ├── x86_64/
│   │   ├── aarch64/
│   │   └── riscv64/
│   ├── bootstrap/
│   ├── storage/
│   ├── database/
│   ├── execution/
│   ├── services/
│   └── runtimes/
├── include/
├── tools/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── storage/
│   ├── boot/
│   └── fixtures/
├── build/
└── Makefile
```

This is an illustrative structure, not a requirement to create every directory immediately.

Directories should be introduced when they contain actual implementation or test material.

## 5. Build system

The build system should support a small number of predictable commands.

A developer should be able to:

- Build the default target.
- Build a specific architecture.
- Build the bootstrap independently.
- Build the OS Database tooling.
- Run unit tests.
- Run integration tests.
- Create a bootable storage image.
- Launch the result in a virtual machine.
- Clean generated artifacts.

GNU Make is a reasonable initial choice.

The build should not require a developer to manually reproduce a long sequence of compiler and linker commands.

### 5.1 Build targets

Potential targets include:

| Target | Purpose |
|---|---|
| `all` | Build the default development target |
| `bootstrap` | Build the architecture-specific bootstrap |
| `core` | Build foundational OS components |
| `database` | Build database and storage components |
| `image` | Construct a bootable OS image |
| `test` | Run the standard test suite |
| `test-unit` | Run unit tests |
| `test-integration` | Run integration tests |
| `run` | Launch the default virtual machine |
| `debug` | Launch with debugging support |
| `clean` | Remove generated build artifacts |

The final target names should be selected during implementation.

### 5.2 Build configuration

Architecture, compiler, optimization level, and build profile should be explicit.

The build system should avoid silently using the host architecture when a cross-architecture target was requested.

Build output should identify:

- Target architecture.
- Compiler and toolchain.
- Build configuration.
- Relevant feature selections.
- Version or revision information.

### 5.3 Reproducibility

Builds should be reproducible wherever practical.

The same source revision and build configuration should produce equivalent artifacts.

Sources of nondeterminism should be identified and controlled where possible.

Examples include:

- Timestamps.
- Unstable file ordering.
- Random identifiers.
- Environment-dependent paths.
- Compiler and linker version differences.

Perfect bit-for-bit reproducibility may not be practical initially, but the build process should not introduce unnecessary variation.

## 6. Cross-compilation

Cross-compilation is essential for supporting multiple CPU architectures.

A developer should be able to build an architecture-specific QueueOS image without running the build environment on that architecture.

The build system should explicitly identify:

- Target architecture.
- Target ABI.
- Compiler.
- Assembler.
- Linker.
- Required firmware and boot artifacts.

Architecture-independent code should be compiled using the same source wherever possible.

Architecture-specific implementations should be selected explicitly by the build configuration.

### 6.1 Architecture targets

The initial architecture targets under consideration are:

- x86-64.
- ARM64.
- RISC-V 64-bit.

The first implementation target should be selected based on available virtualization and development tooling.

Support for additional architectures should follow once the architecture-independent interfaces are sufficiently stable.

### 6.2 Cross-architecture validation

The same architecture-independent test cases should be used across supported targets.

This is particularly important for:

- Storage structures.
- Database metadata.
- Generation selection.
- Queue semantics.
- ABI structures.
- Application manifests.

The goal is to prevent architecture-specific implementations from interpreting shared data differently.

## 7. Storage image tooling

QueueOS requires tooling to create, inspect, validate, and modify test storage images.

These tools should be usable independently of a running QueueOS instance.

Potential capabilities include:

- Creating an initial database image.
- Writing foundational metadata blocks.
- Generating boot directories.
- Creating test generations.
- Inspecting generation metadata.
- Validating block boundaries.
- Simulating incomplete updates.
- Corrupting selected metadata for testing.
- Comparing generations.

These tools are particularly important during bootstrap development.

The format used by the tools must match the actual QueueOS storage specification.

The tools should not become an alternative implementation of the database engine with subtly different behavior.

Where possible, shared format definitions and validation logic should be reused.

## 8. Virtualized development

QEMU/KVM should be the primary initial environment for running QueueOS.

Virtualization makes it possible to test boot behavior, storage recovery, and architecture-specific code without requiring physical hardware for every iteration.

### 8.1 Initial virtual machine

The first virtual machine should be kept deliberately simple.

It should provide:

- A supported CPU architecture.
- Appropriate virtual firmware.
- Sufficient RAM.
- A virtual storage device.
- A serial console or equivalent diagnostic output.
- Optional debugging support.

The initial device set should be small enough to avoid unnecessary driver work.

### 8.2 Machine configurations

Virtual machine configurations should be version-controlled where practical.

A developer should not need to manually recreate a complicated QEMU command line.

The repository may eventually provide:

```bash
make run
make run ARCH=x86_64
make debug
```

These commands are illustrative. Their exact behavior will depend on the selected build system and architecture.

### 8.3 Debugging

The development environment should support debugging at multiple stages:

- Initial firmware handoff.
- Bootstrap execution.
- Metadata discovery.
- Generation selection.
- Component loading.
- OS initialization.
- Queue execution.
- Service startup.

QEMU's GDB integration is a useful initial debugging mechanism.

Early boot diagnostics should be available even when the normal OS logging infrastructure is not yet running.

A serial console is a practical initial solution.

### 8.4 Automated boot testing

Virtualized boot tests should be repeatable and capable of running unattended.

Tests should verify both successful startup and expected failure behavior.

Examples include:

- Booting an empty or invalid image.
- Booting a valid generation.
- Selecting between multiple generations.
- Falling back to an older generation.
- Recovering from incomplete updates.
- Handling missing components.
- Rejecting corrupted metadata.

A test should be able to determine whether the expected boot stage was reached.

## 9. Testing strategy

Testing should begin before the OS is capable of booting fully.

Foundational components should have independent tests wherever practical.

The testing strategy should include several levels.

### 9.1 Unit tests

Unit tests verify individual functions and data structures.

Examples include:

- Metadata parsing.
- Block address calculations.
- Bounds checking.
- Generation comparison.
- Hash calculations.
- Queue state transitions.
- ABI structure validation.
- Rights checks.

These tests should not require a complete OS environment.

### 9.2 Integration tests

Integration tests verify that multiple components work together.

Examples include:

- Storage engine and database metadata.
- Queue submission and completion.
- Service request handling.
- Runtime-to-ABI interaction.
- Application discovery.
- Component loading.

### 9.3 Boot tests

Boot tests validate the complete startup path or selected portions of it.

They should use constructed storage images with known contents and expected outcomes.

### 9.4 Failure tests

Failure testing is especially important for foundational OS functionality.

Tests should cover:

- Invalid input.
- Corrupted storage.
- Incomplete writes.
- Interrupted updates.
- Missing dependencies.
- Resource exhaustion.
- Invalid memory references.
- Unexpected hardware responses.

The OS should fail predictably rather than continuing with corrupted or inconsistent state.

### 9.5 Regression tests

A bug fix should ideally include a test that reproduces the original failure.

Regression tests should be retained as the architecture evolves.

## 10. Code quality and diagnostics

Foundational OS code should be compiled with strong warnings enabled.

The project should aim to maintain:

- Consistent formatting.
- Explicit types.
- Careful memory ownership.
- Well-defined error handling.
- Minimal global mutable state.
- Documented public interfaces.
- No unexplained compiler warnings.

Static analysis and sanitizers should be used where supported by the target environment.

The build should make it easy to identify which component introduced a warning or failure.

### 10.1 Memory safety

C code requires particular attention to:

- Buffer boundaries.
- Integer overflow.
- Pointer validity.
- Allocation and release.
- Alignment.
- Lifetime.
- Concurrent access.
- Untrusted input.

The bootstrap and storage metadata parsers deserve especially strict validation.

### 10.2 Error handling

Errors should be explicit and meaningful.

A component should not silently ignore an error that affects correctness or security.

Error handling conventions should eventually be standardized through the ABI and native OS interfaces.

## 11. Performance testing

Performance should be measured at the component and system levels.

The project should avoid optimizing based on assumptions about conventional OS architecture.

Relevant benchmarks may include:

- Bootstrap size and startup time.
- Database initialization.
- Metadata lookup.
- Storage read and write throughput.
- Queue submission and completion latency.
- Memory allocation.
- Shared-memory transfer.
- Context switching or equivalent execution transitions.
- Service request latency.
- Application startup.

Benchmark results should include enough environmental information to be meaningful.

For example:

- CPU architecture.
- Virtual or physical hardware.
- Available RAM.
- Storage device type.
- Compiler and optimization level.
- Build revision.

Optimization should preserve correctness and compatibility.

## 12. Continuous integration

Continuous integration should validate the project as changes are introduced.

The initial CI system can use GitHub Actions.

Early CI should prioritize fast, deterministic checks.

Potential stages include:

1. Source formatting and static checks.
2. Native compilation.
3. Unit tests.
4. Storage-format tests.
5. Bootstrap build.
6. Image construction.
7. Virtualized boot tests.

Not every architecture needs to be included in every CI run immediately.

However, architecture-independent code should be tested against all supported targets as the infrastructure matures.

### 12.1 Build artifacts

CI should retain useful artifacts where appropriate.

Examples include:

- Bootstrap binaries.
- OS images.
- Test logs.
- Build metadata.
- Debug symbols.
- Test reports.

Artifacts should be clearly associated with the source revision and build configuration.

### 12.2 Release process

Release builds should be distinguishable from development builds.

The release process should eventually include:

- Version identification.
- Build configuration.
- Architecture identification.
- Integrity verification.
- Release notes.
- Reproducible artifact generation where practical.

The final release and update mechanism remains a separate design concern.

## 13. Documentation and decision management

Architectural decisions should be documented as they are made.

The project should distinguish between:

- Accepted decisions.
- Proposed designs.
- Experimental implementations.
- Deferred questions.
- Rejected approaches.

A working implementation does not automatically establish a permanent architectural commitment.

Likewise, an architectural proposal should not be presented as implemented functionality.

When a decision changes, affected documentation should be updated rather than allowing contradictory descriptions to accumulate.

`AGENTS.md` provides additional guidance for human contributors and coding agents.

## 14. Development workflow

A typical development task should follow this sequence:

1. Identify the affected architectural component.
2. Review the relevant documentation and existing implementation.
3. Confirm whether the proposed change conflicts with an existing decision.
4. Implement the smallest coherent change.
5. Build the affected targets.
6. Run the relevant tests.
7. Add regression coverage where appropriate.
8. Update documentation if behavior or architecture changed.
9. Review generated artifacts and diagnostics.
10. Commit the change with a meaningful description.

Foundational architectural changes should receive particular scrutiny.

A change to the queue model, storage format, ABI, or generation protocol may affect many otherwise independent components.

Such changes should be considered at the architectural level before being implemented locally.

## 15. Development milestones

The following is a suggested progression rather than a fixed release schedule.

### Milestone 1: Build foundation

- Establish the repository structure.
- Create the initial build system.
- Compile a minimal architecture-specific executable.
- Establish the QEMU/KVM environment.
- Produce useful diagnostic output.

### Milestone 2: Bootstrap foundation

- Implement initial storage block access.
- Define foundational metadata structures.
- Generate and inspect boot directories.
- Locate and validate a generation.
- Load a minimal next-stage component.

### Milestone 3: Database foundation

- Implement the early storage and database structures.
- Validate generation metadata.
- Establish the full database initialization path.
- Test interrupted and incomplete updates.

### Milestone 4: Execution foundation

- Implement the initial queue infrastructure.
- Establish request and completion semantics.
- Implement a minimal service interaction.
- Validate the application-facing ABI.

### Milestone 5: Operational OS

- Establish core service functionality.
- Introduce memory protection and isolation.
- Implement the initial runtime environment.
- Support basic application installation and execution.

### Milestone 6: Platform expansion

- Add additional device support.
- Validate on physical hardware.
- Expand CPU architecture support.
- Improve performance and diagnostics.

The exact milestone boundaries should evolve as implementation reveals the real dependencies.

## 16. Open design questions

The following decisions remain open:

1. What is the final repository directory structure?
2. Which compiler and linker are mandatory?
3. Will Make remain the primary build system?
4. What is the first supported architecture?
5. Which cross-compilation toolchains are required?
6. What is the canonical bootable image format?
7. Which QEMU machine configurations are initially supported?
8. What is the minimum automated boot test environment?
9. Which static analysis and sanitizer tools are required?
10. How will test coverage and performance benchmarks be tracked?
11. What is the release artifact and versioning policy?
12. What level of reproducibility is required for release builds?

These decisions should be made as the first working implementation takes shape.

## 17. Current development commitments

The following principles are established or strongly intended:

- Linux is the initial development environment.
- C is the preferred language for foundational native components.
- The build system should remain simple and reproducible.
- Cross-compilation is required for multi-architecture support.
- QEMU/KVM is the primary candidate for virtualized development and testing.
- Bootstrap and storage tooling should be testable independently.
- Unit, integration, boot, failure, and regression testing are all important.
- CI should validate builds and tests automatically.
- Architectural decisions must be documented and distinguished from proposals.
- Performance optimizations should be benchmark-driven.
- Physical hardware validation follows the establishment of a reliable virtualized environment.

The exact toolchain, repository layout, build targets, and CI implementation remain open.
