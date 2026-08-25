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
  utf16_surrogate_min = 0x0000'D800,
  utf16_surrogate_max = 0x0000'DFFF,
};

enum : uint32_t {
  packed_character_mask  = 0x1FFFFF,
  packed_remaining_shift = 21,
  packed_remaining_mask  = 0x3,
  packed_octet_len_shift = 23,
  packed_octet_len_mask  = 0x7,
  packed_illegal_shift   = 26,
};

static_assert((uint32_t)utf8_4_character_max <= packed_character_mask);
static_assert((uint32_t)utf8_4_following_cnt <= packed_remaining_mask);
static_assert((uint32_t)utf8_4_octet_len <= packed_octet_len_mask);

struct checker {
  uint32_t character;
  uint8_t remaining_octet_cnt;
  uint8_t octet_len;
  bool illegal;
};

static struct checker unpack(struct utf8c object) {
  return (struct checker){
      .character = object.bits & packed_character_mask,
      .remaining_octet_cnt =
          (uint8_t)((object.bits >> packed_remaining_shift) & packed_remaining_mask),
      .octet_len = (uint8_t)((object.bits >> packed_octet_len_shift) & packed_octet_len_mask),
      .illegal = (object.bits >> packed_illegal_shift) != 0,
  };
}

static struct utf8c pack(struct checker checker) {
  return (struct utf8c){
      .bits = (checker.character & packed_character_mask) |
              ((uint32_t)checker.remaining_octet_cnt << packed_remaining_shift) |
              ((uint32_t)checker.octet_len << packed_octet_len_shift) |
              ((uint32_t)checker.illegal << packed_illegal_shift),
  };
}

static bool character_ok(uint32_t const *character, size_t octet_len) {
  switch (octet_len) {
  case utf8_1_octet_len:
    return *character <= utf8_1_character_max;
  case utf8_2_octet_len:
    return *character >= utf8_2_character_min && *character <= utf8_2_character_max;
  case utf8_3_octet_len:
    return *character >= utf8_3_character_min && *character <= utf8_3_character_max &&
           (*character < utf16_surrogate_min || *character > utf16_surrogate_max);
  case utf8_4_octet_len:
    return *character >= utf8_4_character_min && *character <= utf8_4_character_max;
  default:
    abort();
  }
}

static bool feed_following(struct checker *checker, uint8_t octet) {
  if ((octet & utf8_following_high_order_bit_mask) != utf8_following_high_order_bits) {
    checker->illegal = true;
    return false;
  }

  checker->character <<= utf8_following_bit_cnt;
  checker->character |= (uint32_t)octet & utf8_following_payload_mask;
  checker->remaining_octet_cnt--;
  if (checker->remaining_octet_cnt != 0) return true;

  if (!character_ok(&checker->character, checker->octet_len)) {
    checker->illegal = true;
    return false;
  }

  checker->character = 0;
  checker->octet_len = 0;
  return true;
}

static bool feed_initial(struct checker *checker, uint8_t octet) {
  switch (stdc_leading_ones(octet)) {
  case utf8_1_initial_octet_leading_ones:
    return true;
  case utf8_2_initial_octet_leading_ones:
    checker->character = (uint32_t)octet & utf8_2_initial_payload_mask;
    checker->octet_len = utf8_2_octet_len;
    checker->remaining_octet_cnt = utf8_2_following_cnt;
    return true;
  case utf8_3_initial_octet_leading_ones:
    checker->character = (uint32_t)octet & utf8_3_initial_payload_mask;
    checker->octet_len = utf8_3_octet_len;
    checker->remaining_octet_cnt = utf8_3_following_cnt;
    return true;
  case utf8_4_initial_octet_leading_ones:
    checker->character = (uint32_t)octet & utf8_4_initial_payload_mask;
    checker->octet_len = utf8_4_octet_len;
    checker->remaining_octet_cnt = utf8_4_following_cnt;
    return true;
  default:
    checker->illegal = true;
    return false;
  }
}

bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length) {
  if (state == nullptr) abort();
  if (length != 0 && src == nullptr) abort();

  struct checker checker = unpack(*state);
  if (checker.illegal) return false;
  if (length == 0) return true;

  for (const uint8_t *octet = src; octet < src + length; octet++) {
    if (checker.remaining_octet_cnt == 0) {
      if (!feed_initial(&checker, *octet)) {
        *state = pack(checker);
        return false;
      }
    } else {
      if (!feed_following(&checker, *octet)) {
        *state = pack(checker);
        return false;
      }
    }
  }
  *state = pack(checker);
  return true;
}

struct utf8c utf8c_init() {
  return (struct utf8c){};
}

bool utf8c_finish(const struct utf8c *state) {
  if (state == nullptr) abort();

  struct checker checker = unpack(*state);
  return !checker.illegal && checker.remaining_octet_cnt == 0;
}
