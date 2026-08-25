#ifndef UTF8C_H
#define UTF8C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Incremental well-formed UTF-8 (RFC 3629 §3) checker.
 */
struct utf8c {
  uint8_t rest_cnt; /**< Remaining continuation bytes. 0 means expecting a lead. */
  uint8_t next_min; /**< Inclusive lower bound for the next byte. */
  uint8_t next_max; /**< Inclusive upper bound for the next byte. */
  bool illegal;     /**< Sticky: a previous byte was not well-formed. */
};

struct utf8c utf8c_init();

/**
 * @p src may be null when @p length is 0.
 * @return true if still valid, including an incomplete sequence.
 */
bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length);

/**
 * @return true if valid and idle.
 */
bool utf8c_finish(const struct utf8c *state);

#endif
