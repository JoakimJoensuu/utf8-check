#ifndef UTF8C_H
#define UTF8C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static constexpr size_t utf8c_opaque_size =
    sizeof(uint_least32_t) + sizeof(size_t) + sizeof(size_t) + sizeof(unsigned) +
    sizeof(unsigned char) + sizeof(bool) + alignof(uint_least32_t) + alignof(size_t) +
    alignof(size_t) + alignof(unsigned) + alignof(unsigned char) + alignof(bool) +
    alignof(max_align_t);

/**
 * Incremental well-formed UTF-8 (RFC 3629 §3) checker. Contents are private.
 */
struct utf8c {
  alignas(uint_least32_t) alignas(size_t) alignas(unsigned) alignas(unsigned char) alignas(
      bool) unsigned char opaque[utf8c_opaque_size];
};

struct utf8c utf8c_create();

/**
 * Feed @c CHAR_BIT bits from one storage unit. Bits are fed MSB-first; each
 * 8 bits is one RFC 3629 octet.
 * @return true if still valid, including an incomplete octet or code point.
 */
bool utf8c_feed_bits(struct utf8c *utf8c, unsigned char bits);

/**
 * Feed @p bit_count bits from @p bits, MSB-first. Each 8 bits is one RFC 3629
 * octet. @p bit_count must not exceed @c ULONG_WIDTH.
 * @return true if still valid, including an incomplete octet or code point.
 */
bool utf8c_feed_bit_pattern(struct utf8c *utf8c, unsigned long bits, unsigned long bit_count);

/**
 * Feed one RFC 3629 octet (8 bits). Do not call while @c utf8c_feed_bits() or
 * @c utf8c_feed_bit_pattern() has left a partial octet.
 * @return true if still valid, including an incomplete code point.
 */
bool utf8c_feed_octet(struct utf8c *utf8c, unsigned char octet);

/**
 * @return true if input is valid and no octet or code point remains incomplete from prior
 * feeds.
 */
bool utf8c_is_finished(const struct utf8c *utf8c);

#endif
