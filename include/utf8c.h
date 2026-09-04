#ifndef UTF8C_H
#define UTF8C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static constexpr size_t utf8c_opaque_size =
    sizeof(uint_least32_t) + (alignof(uint_least32_t) - 1) + sizeof(uint8_t) +
    (alignof(uint8_t) - 1) + sizeof(uint8_t) + (alignof(uint8_t) - 1) + sizeof(bool) +
    (alignof(bool) - 1) + (alignof(uint_least32_t) - 1) + (alignof(uint8_t) - 1) +
    (alignof(bool) - 1);

/**
 * Incremental well-formed UTF-8 (RFC 3629 §3) checker. Contents are private.
 */
struct utf8c {
  alignas(uint_least32_t) alignas(uint8_t) alignas(bool) unsigned char opaque[utf8c_opaque_size];
};

struct utf8c utf8c_create();

/**
 * Feed one RFC 3629 octet.
 * @return true if still valid, including an incomplete code point.
 */
bool utf8c_feed_octet(struct utf8c *utf8c, uint8_t octet);

/**
 * @return true if input is valid and no code point remains incomplete from prior feeds.
 */
bool utf8c_is_finished(const struct utf8c *utf8c);

#endif
