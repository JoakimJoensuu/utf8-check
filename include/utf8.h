#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>

/**
 * Incremental well-formed UTF-8 (RFC 3629 §3) only. Contents are private.
 *
 * Null pointers abort, except @p src of utf8_feed() may be null when @p length
 * is 0.
 */
struct utf8 {
  alignas(uint32_t) uint8_t opaque[4];
};

void utf8_init(struct utf8 *state);

/**
 * @return 0 if still valid, including an incomplete sequence. Nonzero on reject.
 */
int utf8_feed(struct utf8 *state, const uint8_t *src, size_t length);

/**
 * @return 0 if idle. Nonzero if a sequence is still open.
 */
int utf8_finish(const struct utf8 *state);

#endif
