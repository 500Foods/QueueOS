# Vendored code

Third-party source that QueueOS builds itself lives here. Each upstream project has its own folder, with that project's license and a `VERSION` file naming the imported release.

A folder appears when the code is actually imported. Empty slots for possible future dependencies are not created.

QueueOS code does not go here. Host tests stay in `tests/`. Operating-system sources stay in `src/`. Firmware installed on the development machine, and Lua modules installed with luarocks, stay outside the tree.

Do not edit files inside an upstream folder. A local change belongs in our code, or in a note next to the import that says why the upstream file could not be used as released.

Lua 5.5.1 is `vendored/lua/`. The license text is `doc/readme.html`, which the upstream README names. `lua.c` and `luac.c` are the host programs and are not compiled. Jansson 2.15.1 is `vendored/jansson/`. `dtoa.c` is not compiled.
