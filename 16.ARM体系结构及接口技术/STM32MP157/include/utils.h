#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/* ======================================================================== *
 *  Public API —— 用户只需要用这一个宏                                      *
 * ======================================================================== */

/**
 * to_string(buf, size, x) —— 任意基本类型 → 字符串
 *
 * 功能：将任意基本类型的值 x 格式化为字符串，存入 buf 中。
 *
 * 支持的类型：
 *   int / unsigned int / long / unsigned long /
 *   long long / unsigned long long / char / short /
 *   float / double / long double /
 *   char* / const char* / void* / const void* / _Bool
 *
 * 用法示例：
 *   char buf[64];
 *   to_string(buf, sizeof(buf), 42);          // → "42"
 *   to_string(buf, sizeof(buf), -123);        // → "-123"
 *   to_string(buf, sizeof(buf), 3.14f);       // → "3.14"
 *   to_string(buf, sizeof(buf), "hello");     // → "hello"
 *   to_string(buf, sizeof(buf), &buf);        // → "0x7fff..."
 *   to_string(buf, sizeof(buf), (_Bool)1);    // → "true"
 */
#define to_string(buf, size, x)                                     \
    _Generic((x),                                                   \
        _Bool:              ts_bool,                                \
        char:               ts_char,                                \
        signed char:        ts_char,                                \
        unsigned char:      ts_char,                                \
        short:              ts_short,                               \
        unsigned short:     ts_ushort,                              \
        int:                ts_int,                                 \
        unsigned int:       ts_uint,                                \
        long:               ts_long,                                \
        unsigned long:      ts_ulong,                               \
        long long:          ts_llong,                               \
        unsigned long long: ts_ullong,                              \
        float:              ts_float,                               \
        double:             ts_double,                              \
        long double:        ts_double,                              \
        char *:             ts_str,                                 \
        const char *:       ts_str,                                 \
        void *:             ts_ptr,                                 \
        const void *:       ts_ptr,                                 \
        default:            ts_ptr                                  \
    )(buf, size, x)

/* ======================================================================== *
 *  Internal —— 由 to_string 宏自动分派，不建议直接调用                      *
 * ======================================================================== */

void ts_char    (char *buf, size_t size, char               val);
void ts_short   (char *buf, size_t size, short              val);
void ts_int     (char *buf, size_t size, int                val);
void ts_uint    (char *buf, size_t size, unsigned int       val);
void ts_long    (char *buf, size_t size, long               val);
void ts_ulong   (char *buf, size_t size, unsigned long      val);
void ts_llong   (char *buf, size_t size, long long          val);
void ts_ullong  (char *buf, size_t size, unsigned long long val);
void ts_float   (char *buf, size_t size, float              val);
void ts_double  (char *buf, size_t size, double             val);
void ts_str     (char *buf, size_t size, const char        *val);
void ts_ptr     (char *buf, size_t size, const void        *val);
void ts_bool    (char *buf, size_t size, _Bool              val);

/* 辅助：unsigned short 复用 ts_uint 的实现 */
static inline void ts_ushort(char *buf, size_t size, unsigned short val)
{
    ts_uint(buf, size, val);
}

#endif /* __UTILS_H__ */