#include "utf8c.h"

#include <limits.h>
#include <stdbit.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

enum utf8_initial_octet_leading_ones : unsigned char {
  utf8_1_initial_octet_leading_ones = 0,
  utf8_2_initial_octet_leading_ones = 2,
  utf8_3_initial_octet_leading_ones = 3,
  utf8_4_initial_octet_leading_ones = 4,
};

enum utf8_octet_sequence_length : unsigned char {
  utf8_1_octet_sequence_length = 1,
  utf8_2_octet_sequence_length = 2,
  utf8_3_octet_sequence_length = 3,
  utf8_4_octet_sequence_length = 4,
};

enum utf8_initial_octet_mask : unsigned char {
  utf8_1_initial_octet_payload_mask = 0b01111111,
  utf8_2_initial_octet_payload_mask = 0b00011111,
  utf8_3_initial_octet_payload_mask = 0b00001111,
  utf8_4_initial_octet_payload_mask = 0b00000111,
};

enum utf8_following_octet_count : unsigned char {
  utf8_1_following_octet_count = 0,
  utf8_2_following_octet_count = 1,
  utf8_3_following_octet_count = 2,
  utf8_4_following_octet_count = 3,
};

enum : unsigned char {
  utf8_following_octet_high_order_bits_mask = 0b11000000,
  utf8_following_octet_high_order_bits      = 0b10000000,
  utf8_following_octet_payload_mask         = 0b00111111,
  utf8_following_octet_payload_bit_count    = 6,
  utf8_octet_bit_count                      = 8,
  utf8_octet_mask                           = 0b11111111,
  utf8_storage_unit_bit_count               = CHAR_BIT,
};

enum : unsigned {
  utf8_unsigned_bit_count      = UINT_WIDTH,
  utf8_unsigned_long_bit_count = ULONG_WIDTH,
};

enum : uint_least32_t {
  utf8_1_character_min            = 0x0000'0000,
  utf8_1_character_max            = 0x0000'007F,
  utf8_2_character_min            = 0x0000'0080,
  utf8_2_character_max            = 0x0000'07FF,
  utf8_3_character_min            = 0x0000'0800,
  utf8_3_character_max            = 0x0000'FFFF,
  utf8_3_prohibited_character_min = 0x0000'D800,
  utf8_3_prohibited_character_max = 0x0000'DFFF,
  utf8_4_character_min            = 0x0001'0000,
  utf8_4_character_max            = 0x0010'FFFF,
};

static unsigned leading_ones_in_octet(unsigned char octet) {
  unsigned value = octet;
  unsigned const aligned = value << (utf8_unsigned_bit_count - utf8_octet_bit_count);
  return stdc_leading_ones(aligned);
}

struct state {
  uint_least32_t character;
  size_t remaining_octet_count;
  size_t octet_sequence_length;
  unsigned partial_octet;
  unsigned char partial_bit_count;
  bool illegal;
};

static_assert(sizeof(struct state) <= sizeof(struct utf8c));
static_assert(alignof(struct state) <= alignof(struct utf8c));

static struct state state_of(const struct utf8c *utf8c) {
  struct state state;
  memcpy(&state, utf8c->opaque, sizeof(state));
  return state;
}

static void store_state(struct utf8c *utf8c, const struct state *state) {
  memcpy(utf8c->opaque, state, sizeof(*state));
}

static bool is_character_valid(uint_least32_t const *character, size_t octet_sequence_length) {
  switch (octet_sequence_length) {
  case utf8_1_octet_sequence_length:
    return *character <= utf8_1_character_max;
  case utf8_2_octet_sequence_length:
    return *character >= utf8_2_character_min && *character <= utf8_2_character_max;
  case utf8_3_octet_sequence_length:
    return *character >= utf8_3_character_min && *character <= utf8_3_character_max &&
           (*character < utf8_3_prohibited_character_min ||
            *character > utf8_3_prohibited_character_max);
  case utf8_4_octet_sequence_length:
    return *character >= utf8_4_character_min && *character <= utf8_4_character_max;
  default:
    unreachable();
  }
}

static void append_following_octet_payload(struct state *state, unsigned char octet) {
  state->character <<= utf8_following_octet_payload_bit_count;
  unsigned payload = octet;
  payload &= utf8_following_octet_payload_mask;
  state->character |= payload;
}

static bool feed_following_octet(struct state *state, unsigned char octet) {
  if ((octet & utf8_following_octet_high_order_bits_mask) != utf8_following_octet_high_order_bits) {
    state->illegal = true;
    return false;
  }

  append_following_octet_payload(state, octet);
  state->remaining_octet_count--;
  if (state->remaining_octet_count != 0) return true;

  if (!is_character_valid(&state->character, state->octet_sequence_length)) {
    state->illegal = true;
    return false;
  }

  state->character = 0;
  state->octet_sequence_length = 0;
  return true;
}

static bool feed_initial_octet(struct state *state, unsigned char octet) {
  switch (leading_ones_in_octet(octet)) {
  case utf8_1_initial_octet_leading_ones:
    return true;
  case utf8_2_initial_octet_leading_ones:
    state->character = octet & utf8_2_initial_octet_payload_mask;
    state->octet_sequence_length = utf8_2_octet_sequence_length;
    state->remaining_octet_count = utf8_2_following_octet_count;
    return true;
  case utf8_3_initial_octet_leading_ones:
    state->character = octet & utf8_3_initial_octet_payload_mask;
    state->octet_sequence_length = utf8_3_octet_sequence_length;
    state->remaining_octet_count = utf8_3_following_octet_count;
    return true;
  case utf8_4_initial_octet_leading_ones:
    state->character = octet & utf8_4_initial_octet_payload_mask;
    state->octet_sequence_length = utf8_4_octet_sequence_length;
    state->remaining_octet_count = utf8_4_following_octet_count;
    return true;
  default:
    state->illegal = true;
    return false;
  }
}

static bool feed_octet(struct state *state, unsigned char octet) {
  if (state->remaining_octet_count == 0) return feed_initial_octet(state, octet);
  return feed_following_octet(state, octet);
}

static bool feed_bit(struct state *state, unsigned char bit) {
  state->partial_octet = (state->partial_octet << 1U) | bit;
  state->partial_bit_count++;
  if (state->partial_bit_count < utf8_octet_bit_count) return true;

  unsigned char const octet = state->partial_octet & utf8_octet_mask;
  state->partial_octet = 0;
  state->partial_bit_count = 0;
  return feed_octet(state, octet);
}

static bool feed_bits(struct utf8c *utf8c, unsigned long bits, unsigned char bit_count) {
  if (utf8c == nullptr) unreachable();
  if (bit_count > utf8_unsigned_long_bit_count) unreachable();

  struct state state = state_of(utf8c);
  if (state.illegal) return false;
  if (bit_count == 0) return true;

  if (bit_count < utf8_unsigned_long_bit_count) bits &= ((1UL << bit_count) - 1UL);

  for (unsigned char index = bit_count; index > 0; index--) {
    unsigned char const bit = (bits >> (index - 1U)) & 1UL;
    if (!feed_bit(&state, bit)) {
      store_state(utf8c, &state);
      return false;
    }
  }
  store_state(utf8c, &state);
  return true;
}

bool utf8c_feed_bits(struct utf8c *utf8c, unsigned char bits) {
  return feed_bits(utf8c, bits, utf8_storage_unit_bit_count);
}

bool utf8c_feed_bit_pattern(struct utf8c *utf8c, size_t bits, unsigned char bit_count) {
  return feed_bits(utf8c, (unsigned long)bits, bit_count);
}

bool utf8c_feed_octet(struct utf8c *utf8c, unsigned char octet) {
  if (utf8c == nullptr) unreachable();

  struct state state = state_of(utf8c);
  if (state.partial_bit_count != 0) unreachable();
  if (state.illegal) return false;

  if (!feed_octet(&state, octet & utf8_octet_mask)) {
    store_state(utf8c, &state);
    return false;
  }
  store_state(utf8c, &state);
  return true;
}

struct utf8c utf8c_create() {
  return (struct utf8c){};
}

bool utf8c_is_finished(const struct utf8c *utf8c) {
  if (utf8c == nullptr) unreachable();

  struct state state = state_of(utf8c);
  return !state.illegal && state.remaining_octet_count == 0 && state.partial_bit_count == 0;
}
