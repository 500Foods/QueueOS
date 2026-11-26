# QueueOS

*A new OS for a new Age.*

QueueOS is an experimental operating system project exploring a different approach to operating system architecture, resource management, storage and application execution.

The project starts with a simple premise: operating systems should be designed around the way modern computing systems actually work, rather than perpetually extending architectural decisions inherited from earlier generations.

## Design Direction

QueueOS is exploring several interconnected architectural ideas:

- **Queue-centric execution:** Queues are a fundamental operating system abstraction, providing a structured interface for application execution, scheduling and resource management.
- **Database-managed system state:** Persistent operating system state and storage are explored through database-oriented architecture, with transactional consistency and recoverability as important design considerations.
- **A consistent application interface:** A stable, architecture-independent ABI separates application interfaces from underlying processor architectures.
- **Native implementation:** A lightweight native implementation, primarily in C, with an emphasis on performance, portability and minimal dependencies.
- **Modular architecture:** Operating system functionality is organized into independently manageable components rather than being tied to a monolithic collection of permanently integrated services.
- **Security by design:** Application authorization, resource ownership and isolation are fundamental architectural concerns.

These are the project's current design directions. Detailed specifications will evolve as architectural decisions are examined and validated.

## Architecture

QueueOS is being designed as a collection of cooperating components, with particular attention to:

- Bootstrapping and system initialization
- Storage management and persistence
- Execution queues and scheduling
- Application interfaces and ABI
- Security and authorization
- Installation, upgrades and recovery

Architectural exploration is in [`brainstorming/`](brainstorming/). Implementation plans are in [`plans/`](plans/README.md).

## Development Status

QueueOS is in its initial architectural design and implementation-planning stage.

The immediate objective is to establish the fundamental architecture, validate its feasibility through prototyping, and develop the initial system components.

Virtualized environments, particularly QEMU/KVM, will provide the initial development and testing platform before expanding hardware compatibility.

## Project Principles

QueueOS aims to be:

- Open source and permissively licensed.
- Efficient in its use of processor, memory and storage resources.
- Portable across processor architectures.
- Modular and extensible.
- Recoverable and resilient.
- Understandable and maintainable.

## Contributing

The project is in its early stages. Architectural documentation and design discussions will be maintained alongside implementation as the project develops.

## License

See [LICENSE.md](LICENSE.md) for licensing information.


## Additional Notes
While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Repository Information 
[![Count Lines of Code](https://github.com/500Foods/QueueOS/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/QueueOS/actions/workflows/main.yml)
<!--CLOC-START -->
```cloc
Last updated at 2026-11-26 18:54:54 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                               82           5897           5486          38543
C/C++ Header                    58           1952           2417           5430
Markdown                        27           2938              2           5368
Lua                              9            226            196           3922
Bourne Shell                     7             79             68           1138
CMake                            8             50             66            423
HTML                             1             36              3            295
make                             1             46             12            174
reStructuredText                 1             26              6             38
YAML                             2              8             13             37
Text                             1              4              0             17
-------------------------------------------------------------------------------
SUM:                           197          11262           8269          55385
-------------------------------------------------------------------------------
46 Files were skipped (duplicate, binary, or without source code):
  conf: 29
  sha256: 4
  version: 3
  list: 2
  changes: 1
  current: 1
  gitattributes: 1
  gitignore: 1
  in: 1
  lds: 1
  license: 1
  readme: 1
```
<!--CLOC-END-->

## Sponsor / Donate / Support
If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
