#ifndef UTF8C_H
#define UTF8C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static constexpr size_t utf8c_opaque_size =
    sizeof(uint_least32_t) + sizeof(size_t) + sizeof(size_t) + sizeof(unsigned char) +
    sizeof(unsigned char) + sizeof(bool) + alignof(uint_least32_t) + alignof(size_t) +
    alignof(size_t) + alignof(unsigned char) + alignof(unsigned char) + alignof(bool);

/**
 * Incremental well-formed UTF-8 (RFC 3629 §3) checker. Contents are private.
 */
struct utf8c {
  alignas(uint_least32_t) alignas(size_t) alignas(unsigned char) alignas(
      bool) unsigned char opaque[utf8c_opaque_size];
};

struct utf8c utf8c_create();

/**
 * Feed @c CHAR_BIT bits from one storage unit, MSB-first.
 * @return true if still valid, including an incomplete octet or code point.
 */
bool utf8c_feed_bits(struct utf8c *utf8c, unsigned char bits);

/**
 * Feed one RFC 3629 octet.
 * @param octet only the least significant 8 bits are used.
 * @return true if still valid, including an incomplete code point.
 */
bool utf8c_feed_octet(struct utf8c *utf8c, unsigned octet);

/**
 * @return true if input is valid and no octet or code point remains incomplete from prior
 * feeds.
 */
bool utf8c_is_finished(const struct utf8c *utf8c);

#endif
