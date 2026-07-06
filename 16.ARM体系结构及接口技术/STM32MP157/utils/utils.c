#include "utils.h"

/* ---------- char ---------- */
void ts_char(char *buf, size_t size, char val)
{
    snprintf(buf, size, "%c", val);
}

/* ---------- short ---------- */
void ts_short(char *buf, size_t size, short val)
{
    snprintf(buf, size, "%hd", val);
}

/* ---------- int ---------- */
void ts_int(char *buf, size_t size, int val)
{
    snprintf(buf, size, "%d", val);
}

/* ---------- unsigned int ---------- */
void ts_uint(char *buf, size_t size, unsigned int val)
{
    snprintf(buf, size, "%u", val);
}

/* ---------- long ---------- */
void ts_long(char *buf, size_t size, long val)
{
    snprintf(buf, size, "%ld", val);
}

/* ---------- unsigned long ---------- */
void ts_ulong(char *buf, size_t size, unsigned long val)
{
    snprintf(buf, size, "%lu", val);
}

/* ---------- long long ---------- */
void ts_llong(char *buf, size_t size, long long val)
{
    snprintf(buf, size, "%lld", val);
}

/* ---------- unsigned long long ---------- */
void ts_ullong(char *buf, size_t size, unsigned long long val)
{
    snprintf(buf, size, "%llu", val);
}

/* ---------- float ---------- */
void ts_float(char *buf, size_t size, float val)
{
    snprintf(buf, size, "%g", (double)val);
}

/* ---------- double ---------- */
void ts_double(char *buf, size_t size, double val)
{
    snprintf(buf, size, "%g", val);
}

/* ---------- 字符串 (char* / const char*) ---------- */
void ts_str(char *buf, size_t size, const char *val)
{
    if (val == NULL)
        snprintf(buf, size, "(null)");
    else
        snprintf(buf, size, "%s", val);
}

/* ---------- 指针 (void*) ---------- */
void ts_ptr(char *buf, size_t size, const void *val)
{
    snprintf(buf, size, "%p", val);
}

/* ---------- _Bool ---------- */
void ts_bool(char *buf, size_t size, _Bool val)
{
    snprintf(buf, size, "%s", val ? "true" : "false");
}
