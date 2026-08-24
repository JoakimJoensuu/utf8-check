#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>

// Incremental well-formed UTF-8 (RFC 3629 §3) only. Null pointers abort, except src
// may be null when length is 0.

struct utf8 {
  uint8_t rest_cnt;
  uint8_t next_min;
  uint8_t next_max;
  uint8_t illegal;
};

void utf8_init(struct utf8 *state);

// 0 if still valid, including an incomplete sequence. Nonzero on reject.
int utf8_feed(struct utf8 *state, const uint8_t *src, size_t length);

// 0 if idle. Nonzero if a sequence is still open.
int utf8_finish(const struct utf8 *state);

// init, feed, and finish of one buffer.
int utf8_check(const uint8_t *src, size_t length);

#endif
