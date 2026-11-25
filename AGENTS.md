# QueueOS — Agent Development Instructions

## 1. Project Overview

QueueOS is an experimental operating system exploring a different approach to operating system architecture, execution, storage and resource management.

The project emphasizes queue-centric execution, database-managed system state, a consistent application interface, modularity, portability and security.

The objective is not to reproduce an existing operating system with a different appearance. QueueOS is an opportunity to reconsider fundamental operating system design decisions.

All development agents must understand this distinction.

## 2. Architectural Principles

The following principles should guide all development.

### 2.1 Queue-centric architecture

Queues are a fundamental architectural concept in QueueOS, not merely an implementation detail.

System services, application interfaces, scheduling and resource management should be considered in the context of the overall queue architecture.

Do not introduce conventional request/response, process-management or messaging patterns without considering their implications for the QueueOS architecture.

### 2.2 Database-oriented system architecture

QueueOS explores database-managed system state and storage.

Database concepts should be considered first-class architectural concerns, rather than simply adding a conventional database to an otherwise conventional operating system.

Preserve consistency, transactional integrity, recoverability and portability.

Do not assume that conventional filesystem semantics are necessarily appropriate for every storage requirement.

### 2.3 Architecture independence

Applications should interact with QueueOS through a consistent ABI rather than depending directly on processor-specific implementation details.

Processor-specific optimizations must not compromise portability or the consistency of the application interface.

Where architecture-specific implementations are necessary, maintain clearly defined boundaries.

### 2.4 Native implementation

The primary native implementation language is C.

Keep the implementation efficient, portable and conscious of memory and processor overhead.

Avoid introducing heavyweight runtime dependencies without a compelling architectural justification.

Lua is also part of the project's development and modularity exploration, including its potential use for reference implementations and module definitions.

Script that ships in the operating system is Lua. Host checks and the test harness are Bash. Section 11 is the rule for languages, file size, and filenames.

### 2.5 Modularity

Operating system components should have well-defined responsibilities and interfaces.

Avoid unnecessary coupling between components.

New functionality should be designed so that it can be independently developed, tested, replaced or extended wherever practical.

### 2.6 Security and authorization

Security must be considered throughout the architecture rather than added after functionality is implemented.

QueueOS distinguishes application features from the rights granted to applications.

Application installation should include an authorization presentation identifying the rights being requested or granted.

Do not conflate application functionality with authorization to access system resources.

## 3. Architectural Decision Management

This section is particularly important.

### 3.1 Preserve established decisions

Do not silently change established architectural decisions.

If implementation reveals that an existing decision is impractical, document the problem and present alternatives before changing the architecture.

Explain the consequences of proposed changes, including compatibility, performance, portability and security implications.

### 3.2 Distinguish decisions from proposals

Not every idea discussed during development is an approved architectural decision.

Maintain these distinctions:

- **Accepted:** An agreed architectural direction.
- **Proposed:** An idea under consideration.
- **Experimental:** An approach being evaluated through implementation or benchmarking.
- **Deferred:** A decision intentionally postponed.
- **Rejected:** An approach considered and explicitly abandoned.

Do not promote a proposal or experiment to an accepted decision without agreement.

### 3.3 Challenge assumptions

Agents are expected to exercise independent technical judgment.

If an assumption appears incorrect, explain why.

If an alternative offers meaningful improvements, present it with supporting reasoning.

Agreement is less important than technical correctness and architectural consistency.

### 3.4 Document significant changes

Record a change in the place that already holds that kind of information.

- `brainstorming/` is exploration. A note there does not accept a decision.
- `plans/interview-log.md` holds the design interview. Each note carries a status from section 3.2.
- `plans/` holds implementation plans. A plan states the slice to build, what is deferred, the order of the work, and how the result is validated. Plans are a diary. When a slice is finished, correct that plan's record and move unfinished work into the next plan. Leave the earlier mistakes in the record.
- `plans/README.md` names the plan that is ready to build. That plan is the active slice. One plan is ready at a time. Interview notes for the following plan go in the interview log while the active plan is open. They do not change the active plan's scope, and they do not start the following plan's code. Mark the following plan ready only after the active plan is closed.
- `docs/` is reserved for an accepted architecture manual. The directory has no manual yet. Create a page there when an accepted design needs one. Copying `brainstorming/` or the interview log into `docs/` does not accept those notes.

Changes to established interfaces, storage formats, execution models, or security boundaries are recorded in the active plan and, once accepted, in that manual.

## 4. Development Practices

### 4.1 Understand before modifying

Before implementing a feature:

1. Read `plans/README.md` and the active plan. Read `plans/interview-log.md` for the status of decisions that plan depends on. Then read the code and interfaces the plan names. Treat `brainstorming/` as exploration. `docs/` has no manual yet.
2. Examine existing implementations and interfaces.
3. Identify dependencies and affected components.
4. Understand existing conventions.
5. Identify unresolved architectural questions.

Do not invent interfaces or assumptions when the existing architecture does not yet define them.

### 4.2 Keep changes focused

Prefer small, coherent changes over large speculative rewrites.

Avoid unrelated refactoring while implementing a feature.

Do not replace working implementations simply because another approach is more familiar.

### 4.3 Preserve existing functionality

Existing functionality should remain operational unless its replacement is explicitly part of the task.

Do not remove compatibility mechanisms, tests or diagnostic facilities without understanding their purpose.

### 4.4 Dependencies

Prefer small, well-understood dependencies.

Before introducing a dependency, evaluate:

- Binary size.
- Runtime memory requirements.
- CPU overhead.
- Portability across target architectures.
- Licensing compatibility.
- Maintenance requirements.
- Whether equivalent functionality can reasonably be implemented internally.

Do not introduce dependencies merely for convenience.

### 4.5 Performance

Performance is an architectural concern.

Avoid optimizing based exclusively on assumptions or theoretical advantages.

Where meaningful, benchmark competing approaches on representative hardware.

Pay particular attention to:

- Memory consumption.
- CPU utilization.
- Storage overhead.
- I/O performance.
- Startup time.
- Cross-architecture behavior.

Do not sacrifice portability or correctness for an optimization without documenting the trade-offs.

## 5. Testing and Validation

Testing must be an integral part of development.

### 5.1 Virtualized testing

QEMU/KVM is the initial primary environment for system development and automated testing.

Use virtualization to validate fundamental functionality before relying on physical hardware.

### 5.2 Hardware portability

Do not assume that successful execution on one processor architecture establishes compatibility with another.

Architecture-specific functionality must be identified and tested independently.

### 5.3 Testing expectations

New functionality should include appropriate tests.

Where practical, test:

- Normal operation.
- Invalid input.
- Resource exhaustion.
- Interrupted operations.
- Recovery after failure.
- Boundary conditions.
- Compatibility with existing functionality.

### 5.4 Reproducibility

Builds and tests should be reproducible.

Document required tools, dependencies, configuration and test procedures.

Do not claim successful validation when tests have not actually been executed.

## 6. Code Quality

- Write clear, maintainable C.
- Use explicit interfaces and well-defined ownership.
- Avoid unnecessary dynamic allocation.
- Handle errors explicitly.
- Validate external input.
- Avoid undefined behavior.
- Keep architecture-specific code isolated.
- Prefer straightforward implementations over unnecessary abstraction.
- Comment architectural reasoning, not merely obvious operations.

Follow existing project conventions where they are established.

Do not introduce arbitrary coding standards that conflict with the existing implementation.

## 7. Documentation

Documentation is part of the implementation.

When changing functionality, update the relevant documentation. That is the active plan, the interview log when a status changes, and `docs/` once an accepted manual exists. Section 3.4 names those places.

When introducing a new component, explain its responsibilities, interfaces and dependencies.

When changing an established design, explain why the change was necessary.

Keep the following information distinct:

- What the system currently implements.
- What the architecture specifies.
- What remains experimental.
- What is planned but not implemented.

Never document proposed functionality as though it already exists.

## 8. Collaboration

QueueOS may be developed using multiple AI coding agents and human contributors.

Agents should:

- Respect existing work performed by other contributors.
- Avoid overwriting unrelated changes.
- Identify assumptions and dependencies explicitly.
- Provide concise implementation summaries.
- Report tests performed and their results.
- Identify unresolved issues and recommended next steps.

When multiple implementation approaches are possible, explain the trade-offs rather than making consequential architectural decisions unilaterally.

## 9. Scope and Priorities

The immediate development priority is establishing and validating the fundamental QueueOS architecture.

The active slice is the plan `plans/README.md` marks ready to build. Finish that slice before writing the next plan's code. Interview notes taken along the way land in `plans/interview-log.md` and wait there.

Avoid premature feature expansion.

Prioritize foundational functionality, clearly defined interfaces and repeatable testing.

Do not introduce features simply because conventional operating systems provide them. Each feature should have a justified place within the QueueOS architecture.

## 10. Final Development Rule

**Understand the architecture. Preserve its intent. Challenge it constructively. Implement carefully. Validate the result. Document what changed.**

QueueOS should evolve through deliberate engineering decisions rather than accumulating conventional operating system assumptions by default.

## 11. Language, size, and filenames

These rules apply to every QueueOS file an agent creates or edits.

### 11.1 No Python

QueueOS does not use Python. That includes scripts, generators, checkers, build steps, and tests. Do not add a `.py` file. Do not invoke `python`, `python3`, or a Python module from CMake or from a script.

### 11.2 Which language

C is the native implementation language, as section 2.4 says.

Script that ships in the operating system is Lua. Boot screens, menus, and module definitions that run on QueueOS are Lua.

The test harness and the host checks are Bash. A check that `ctest` runs, and the QEMU runner, are shell scripts.

Host programs that build artifacts are C. The image builder and the PE relocation tool are host C programs. They may execute existing system tools such as `mkfs.fat`, `mmd`, `mcopy`, and `qemu-img`.

`tests/test-queueos.lua` is the interactive catalogue. It is Lua. With no arguments it lists cases. `all` calls the Bash harness in `tests/harness/run.sh`. The Lua file does not contain a QEMU command line. It stays Lua.

### 11.3 Source file length

A QueueOS source file stays at or under 1,000 lines. Split a file before it crosses that line. Generated output under `build/` is not a source file.

### 11.4 Filenames

QueueOS filenames use dashes, not underscores. This applies to files and directories the project creates.

C identifiers keep their underscores. An identifier is not a filename.

Names that belong to an external tool stay as that tool spells them. `qemu-system-x86_64` and the objcopy target `efi-app-x86_64` are examples. An architecture token passed to a tool is not a filename this project chose.

### 11.5 Vendored trees

`vendored/<project>/` holds unmodified third-party source. Those files keep the upstream names and the upstream lengths. Do not rename, reformat, or split them to satisfy 11.3 or 11.4.

Unity 2.7.0 is the current case. `unity.c` and `unity_internals.h` are longer than 1,000 lines, and `unity_internals.h` uses an underscore. Leave that tree untouched.

A new import is recorded the same way, with its license and `VERSION`, and the upstream files stay as they arrived.

QueueOS files outside `vendored/` follow 11.1 through 11.4.

