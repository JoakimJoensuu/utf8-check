#include "utf8c.h"

#include <stdbit.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * RFC 3629 §3:
 *
 *     | Char. number range  |        UTF-8 octet sequence
 *     |    (hexadecimal)    |              (binary)
 *   --+---------------------+---------------------------------------------
 *   1 | 0000 0000-0000 007F | 0xxxxxxx
 *   2 | 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
 *   3 | 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
 *   4 | 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *                                ^     +------------------------+
 *                             initial       following octets
 *                              octet
 */

enum utf8_initial_octet_leading_ones : uint8_t {
  utf8_1_initial_octet_leading_ones = 0,
  utf8_2_initial_octet_leading_ones = 2,
  utf8_3_initial_octet_leading_ones = 3,
  utf8_4_initial_octet_leading_ones = 4,
};

enum utf8_octet_sequence_len : uint8_t {
  utf8_1_octet_sequence_len = 1,
  utf8_2_octet_sequence_len = 2,
  utf8_3_octet_sequence_len = 3,
  utf8_4_octet_sequence_len = 4,
};

enum utf8_initial_octet_mask : uint8_t {
  utf8_1_initial_octet_payload_mask = 0b01111111,
  utf8_2_initial_octet_payload_mask = 0b00011111,
  utf8_3_initial_octet_payload_mask = 0b00001111,
  utf8_4_initial_octet_payload_mask = 0b00000111,
};

enum utf8_following_octet_cnt : uint8_t {
  utf8_1_following_octet_cnt = 0,
  utf8_2_following_octet_cnt = 1,
  utf8_3_following_octet_cnt = 2,
  utf8_4_following_octet_cnt = 3,
};

enum : uint8_t {
  utf8_following_octet_high_order_bits_mask = 0b11000000,
  utf8_following_octet_high_order_bits      = 0b10000000,
  utf8_following_octet_payload_mask         = 0b00111111,
  utf8_following_octet_payload_bit_cnt      = 6,
};

enum : uint32_t {
  utf8_1_character_min = 0x0000'0000,
  utf8_1_character_max = 0x0000'007F,
  utf8_2_character_min = 0x0000'0080,
  utf8_2_character_max = 0x0000'07FF,
  utf8_3_character_min = 0x0000'0800,
  utf8_3_character_max = 0x0000'FFFF,
  utf8_4_character_min = 0x0001'0000,
  utf8_4_character_max = 0x0010'FFFF,
};

enum : uint32_t {
  surrogate_character_min = 0x0000'D800,
  surrogate_character_max = 0x0000'DFFF,
};

struct state {
  uint32_t character;
  size_t remaining_octet_cnt;
  size_t octet_sequence_len;
  bool illegal;
};

static_assert(sizeof(struct state) <= sizeof(struct utf8c));
static_assert(alignof(struct state) <= alignof(struct utf8c));

static bool is_character_valid(uint32_t const *character, size_t octet_sequence_len) {
  switch (octet_sequence_len) {
  case utf8_1_octet_sequence_len:
    return *character <= utf8_1_character_max;
  case utf8_2_octet_sequence_len:
    return *character >= utf8_2_character_min && *character <= utf8_2_character_max;
  case utf8_3_octet_sequence_len:
    return *character >= utf8_3_character_min && *character <= utf8_3_character_max &&
           (*character < surrogate_character_min || *character > surrogate_character_max);
  case utf8_4_octet_sequence_len:
    return *character >= utf8_4_character_min && *character <= utf8_4_character_max;
  default:
    abort();
  }
}

static void append_following_payload(struct state *state, uint8_t octet) {
  state->character <<= utf8_following_octet_payload_bit_cnt;
  state->character |= (uint32_t)octet & utf8_following_octet_payload_mask;
}

static bool feed_following_octet(struct state *state, uint8_t octet) {
  if ((octet & utf8_following_octet_high_order_bits_mask) != utf8_following_octet_high_order_bits) {
    state->illegal = true;
    return false;
  }

  append_following_payload(state, octet);
  state->remaining_octet_cnt--;
  if (state->remaining_octet_cnt != 0) return true;

  if (!is_character_valid(&state->character, state->octet_sequence_len)) {
    state->illegal = true;
    return false;
  }

  state->character = 0;
  state->octet_sequence_len = 0;
  return true;
}

static bool feed_initial_octet(struct state *state, uint8_t octet) {
  switch (stdc_leading_ones(octet)) {
  case utf8_1_initial_octet_leading_ones:
    return true;
  case utf8_2_initial_octet_leading_ones:
    state->character = (uint32_t)octet & utf8_2_initial_octet_payload_mask;
    state->octet_sequence_len = utf8_2_octet_sequence_len;
    state->remaining_octet_cnt = utf8_2_following_octet_cnt;
    return true;
  case utf8_3_initial_octet_leading_ones:
    state->character = (uint32_t)octet & utf8_3_initial_octet_payload_mask;
    state->octet_sequence_len = utf8_3_octet_sequence_len;
    state->remaining_octet_cnt = utf8_3_following_octet_cnt;
    return true;
  case utf8_4_initial_octet_leading_ones:
    state->character = (uint32_t)octet & utf8_4_initial_octet_payload_mask;
    state->octet_sequence_len = utf8_4_octet_sequence_len;
    state->remaining_octet_cnt = utf8_4_following_octet_cnt;
    return true;
  default:
    state->illegal = true;
    return false;
  }
}

bool utf8c_feed(struct utf8c *utf8c, const uint8_t *src, size_t length) {
  if (utf8c == nullptr) abort();
  if (length != 0 && src == nullptr) abort();

  struct state *state = (struct state *)utf8c;
  if (state->illegal) return false;
  if (length == 0) return true;

  for (const uint8_t *octet = src; octet < src + length; octet++) {
    if (state->remaining_octet_cnt == 0) {
      if (!feed_initial_octet(state, *octet)) return false;
    } else {
      if (!feed_following_octet(state, *octet)) return false;
    }
  }
  return true;
}

struct utf8c utf8c_init() {
  return (struct utf8c){};
}

bool utf8c_finish(const struct utf8c *utf8c) {
  if (utf8c == nullptr) abort();

  const struct state *state = (const struct state *)utf8c;
  return !state->illegal && state->remaining_octet_cnt == 0;
}
