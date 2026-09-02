#ifndef UTF8C_TEST_H
#define UTF8C_TEST_H

#include <utf8c.h>

/**
 * Test-only feed with an arbitrary storage-unit width. Not part of the library API.
 */
bool utf8c_test_feed_bits(struct utf8c *utf8c, unsigned long bits, unsigned char bit_count);

#endif
