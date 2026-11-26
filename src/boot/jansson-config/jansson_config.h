/* Hand substitution for vendored/jansson/src/jansson_config.h.in.
 * The upstream header says to fill these in when configure is not run.
 * Boot is freestanding, so this build does not run that script.
 * Dtoa stays off: the private config is not included, and DTOA_ENABLED
 * is therefore unset. Integer JSON uses strtoll.
 */
#ifndef JANSSON_CONFIG_H
#define JANSSON_CONFIG_H

#ifdef __cplusplus
#define JSON_INLINE inline
#else
#define JSON_INLINE inline
#endif

#define JSON_INTEGER_IS_LONG_LONG 1
#define JSON_HAVE_ATOMIC_BUILTINS 1
#define JSON_HAVE_SYNC_BUILTINS 1
#define JSON_PARSER_MAX_DEPTH 2048

#endif
