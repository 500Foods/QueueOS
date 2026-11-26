/* Names of the Lua sources linked into Boot. The bytes live in a
 * generated file. Nothing is read from the ESP at runtime.
 */
#ifndef QUEUEOS_GUEST_SCRIPTS_H
#define QUEUEOS_GUEST_SCRIPTS_H

typedef struct QueueosLuaScript {
    const char *name;
    const unsigned char *bytes;
    unsigned length;
} QueueosLuaScript;

const QueueosLuaScript *queueos_lua_script(const char *name);

#endif
