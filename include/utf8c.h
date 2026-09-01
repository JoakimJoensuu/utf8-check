#ifndef UTF8C_H
#define UTF8C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static constexpr size_t utf8c_opaque_len = sizeof(uint32_t) + sizeof(size_t) + sizeof(size_t) +
                                           sizeof(bool) + alignof(uint32_t) + alignof(size_t) +
                                           alignof(size_t) + alignof(bool);

/**
 * Incremental well-formed UTF-8 (RFC 3629 §3) checker. Contents are private.
 */
struct utf8c {
  alignas(uint32_t) alignas(size_t) alignas(bool) unsigned char opaque[utf8c_opaque_len];
};

struct utf8c utf8c_init();

/**
 * @p octets may be null when @p length is 0.
 * @return true if still valid, including an incomplete sequence.
 */
bool utf8c_feed(struct utf8c *utf8c, const uint8_t *octets, size_t length);

/**
 * @return true if valid and idle.
 */
bool utf8c_finish(const struct utf8c *utf8c);

#endif
