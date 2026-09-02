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
  utf8_octet_mask                           = 0xFF,
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

enum : unsigned {
  utf8_bits_per_word      = (unsigned)(sizeof(unsigned long) * CHAR_BIT),
  utf8_unsigned_bit_count = (unsigned)(sizeof(unsigned) * CHAR_BIT),
};

static unsigned leading_ones_in_octet(unsigned octet) {
  return stdc_leading_ones(octet << (utf8_unsigned_bit_count - utf8_octet_bit_count));
}

struct state {
  uint_least32_t character;
  size_t remaining_octet_count;
  size_t octet_sequence_length;
  unsigned char partial_octet;
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

static void append_following_octet_payload(struct state *state, unsigned octet) {
  state->character <<= utf8_following_octet_payload_bit_count;
  state->character |= (uint_least32_t)octet & utf8_following_octet_payload_mask;
}

static bool feed_following_octet(struct state *state, unsigned octet) {
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

static bool feed_initial_octet(struct state *state, unsigned octet) {
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

static bool feed_octet(struct state *state, unsigned octet) {
  if (state->remaining_octet_count == 0) return feed_initial_octet(state, octet);
  return feed_following_octet(state, octet);
}

static bool feed_bit(struct state *state, unsigned bit) {
  state->partial_octet = (unsigned char)(((unsigned)state->partial_octet << 1U) | (bit & 1U));
  state->partial_bit_count++;
  if (state->partial_bit_count < utf8_octet_bit_count) return true;

  unsigned const octet = state->partial_octet;
  state->partial_octet = 0;
  state->partial_bit_count = 0;
  return feed_octet(state, octet);
}

static bool feed_bits(struct utf8c *utf8c, unsigned long bits, unsigned bit_count) {
  if (utf8c == nullptr) unreachable();
  if (bit_count > utf8_bits_per_word) unreachable();

  struct state state = state_of(utf8c);
  if (state.illegal) return false;
  if (bit_count == 0) return true;

  if (bit_count < utf8_bits_per_word) bits &= ((1UL << bit_count) - 1UL);

  for (unsigned index = bit_count; index > 0; index--) {
    unsigned const bit = (unsigned)((bits >> (index - 1)) & 1UL);
    if (!feed_bit(&state, bit)) {
      store_state(utf8c, &state);
      return false;
    }
  }
  store_state(utf8c, &state);
  return true;
}

bool utf8c_feed_bits(struct utf8c *utf8c, unsigned char bits) {
  return feed_bits(utf8c, bits, CHAR_BIT);
}

bool utf8c_feed_octet(struct utf8c *utf8c, unsigned octet) {
  return feed_bits(utf8c, octet & utf8_octet_mask, utf8_octet_bit_count);
}

struct utf8c utf8c_create() {
  return (struct utf8c){};
}

bool utf8c_is_finished(const struct utf8c *utf8c) {
  if (utf8c == nullptr) unreachable();

  struct state state = state_of(utf8c);
  return !state.illegal && state.remaining_octet_count == 0 && state.partial_bit_count == 0;
}
