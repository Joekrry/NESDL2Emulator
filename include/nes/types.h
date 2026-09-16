#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define KiB(n) ((size_t)(n) * 1024u)
#define MiB(n) ((size_t)(n) * 1024u * 1024u)

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UNUSED(x) ((void)(x))
