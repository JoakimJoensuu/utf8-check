#include "utf8c.h"

#include <stdbit.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * RFC 3629 §3:
 *
 *   Char. number range  |        UTF-8 octet sequence
 *      (hexadecimal)    |              (binary)
 *   --------------------+---------------------------------------------
 *   0000 0000-0000 007F | 0xxxxxxx
 *   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
 *   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
 *   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *                            ^     +------------------------+
 *                         initial       following octets
 *                          octet
 */

enum utf8_initial_octet_leading_ones : uint8_t {
  utf8_1_initial_octet_leading_ones = 0,
  utf8_2_initial_octet_leading_ones = 2,
  utf8_3_initial_octet_leading_ones = 3,
  utf8_4_initial_octet_leading_ones = 4,
};

enum utf8_initial_octet_mask : uint8_t {
  utf8_1_initial_payload_mask = 0b01111111,
  utf8_2_initial_payload_mask = 0b00011111,
  utf8_3_initial_payload_mask = 0b00001111,
  utf8_4_initial_payload_mask = 0b00000111,
};

enum utf8_octet_len : uint8_t {
  utf8_1_octet_len = 1,
  utf8_2_octet_len = 2,
  utf8_3_octet_len = 3,
  utf8_4_octet_len = 4,
};

enum : uint8_t {
  utf8_following_high_order_bit_mask = 0b11000000,
  utf8_following_high_order_bits     = 0b10000000,
  utf8_following_payload_mask        = 0b00111111,
  utf8_following_bit_cnt             = 6,
};

enum utf8_following_cnt : uint8_t {
  utf8_1_following_cnt = 0,
  utf8_2_following_cnt = 1,
  utf8_3_following_cnt = 2,
  utf8_4_following_cnt = 3,
};

enum : uint32_t {
  utf8_1_min = 0b00000000'00000000'00000000'00000000,
  utf8_1_max = 0b00000000'00000000'00000000'01111111,
  utf8_2_min = 0b00000000'00000000'00000000'10000000,
  utf8_2_max = 0b00000000'00000000'00000111'11111111,
  utf8_3_min = 0b00000000'00000000'00001000'00000000,
  utf8_3_max = 0b00000000'00000000'11111111'11111111,
  utf8_4_min = 0b00000000'00000001'00000000'00000000,
  utf8_4_max = 0b00000000'00010000'11111111'11111111,
};

enum : uint32_t {
  utf16_surrogate_min = 0b00000000'00000000'11011000'00000000,
  utf16_surrogate_max = 0b00000000'00000000'11011111'11111111,
};

static bool character_ok(uint32_t const *character, size_t octet_len) {
  switch (octet_len) {
  case utf8_1_octet_len:
    return *character <= utf8_1_max;
  case utf8_2_octet_len:
    return *character >= utf8_2_min && *character <= utf8_2_max;
  case utf8_3_octet_len:
    return *character >= utf8_3_min && *character <= utf8_3_max &&
           (*character < utf16_surrogate_min || *character > utf16_surrogate_max);
  case utf8_4_octet_len:
    return *character >= utf8_4_min && *character <= utf8_4_max;
  default:
    abort();
  }
}

static bool feed_following(struct utf8c *state, uint8_t octet) {
  if ((octet & utf8_following_high_order_bit_mask) != utf8_following_high_order_bits) {
    state->illegal = true;
    return false;
  }

  state->character <<= utf8_following_bit_cnt;
  state->character |= (uint32_t)octet & utf8_following_payload_mask;
  state->remaining_octet_cnt--;
  if (state->remaining_octet_cnt != 0) return true;

  if (!character_ok(&state->character, state->octet_len)) {
    state->illegal = true;
    return false;
  }

  state->character = 0;
  state->octet_len = 0;
  return true;
}

static bool feed_initial(struct utf8c *state, uint8_t octet) {
  switch (stdc_leading_ones(octet)) {
  case utf8_1_initial_octet_leading_ones:
    return true;
  case utf8_2_initial_octet_leading_ones:
    state->character = (uint32_t)octet & utf8_2_initial_payload_mask;
    state->octet_len = utf8_2_octet_len;
    state->remaining_octet_cnt = utf8_2_following_cnt;
    return true;
  case utf8_3_initial_octet_leading_ones:
    state->character = (uint32_t)octet & utf8_3_initial_payload_mask;
    state->octet_len = utf8_3_octet_len;
    state->remaining_octet_cnt = utf8_3_following_cnt;
    return true;
  case utf8_4_initial_octet_leading_ones:
    state->character = (uint32_t)octet & utf8_4_initial_payload_mask;
    state->octet_len = utf8_4_octet_len;
    state->remaining_octet_cnt = utf8_4_following_cnt;
    return true;
  default:
    state->illegal = true;
    return false;
  }
}

bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length) {
  if (state == nullptr) abort();
  if (length != 0 && src == nullptr) abort();

  if (state->illegal) return false;
  if (length == 0) return true;

  for (const uint8_t *octet = src; octet < src + length; octet++) {
    if (state->remaining_octet_cnt == 0) {
      if (!feed_initial(state, *octet)) return false;
    } else {
      if (!feed_following(state, *octet)) return false;
    }
  }
  return true;
}

struct utf8c utf8c_init() {
  return (struct utf8c){};
}

bool utf8c_finish(const struct utf8c *state) {
  if (state == nullptr) abort();

  return !state->illegal && state->remaining_octet_cnt == 0;
}
