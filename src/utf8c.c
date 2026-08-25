#include "utf8c.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum : uint32_t {
  utf8_1_mask = 0b10000000,
  utf8_1_tag = 0b00000000,
  utf8_2_mask = 0b11100000,
  utf8_2_tag = 0b11000000,
  utf8_2_payload = 0b00011111,
  utf8_3_mask = 0b11110000,
  utf8_3_tag = 0b11100000,
  utf8_3_payload = 0b00001111,
  utf8_4_mask = 0b11111000,
  utf8_4_tag = 0b11110000,
  utf8_4_payload = 0b00000111,
  utf8_tail_mask = 0b11000000,
  utf8_tail_tag = 0b10000000,
  utf8_tail_payload = 0b00111111,
};

enum : uint32_t {
  utf8_2_min = 0x80,
  utf8_3_min = 0x800,
  surrogate_min = 0xD800,
  surrogate_max = 0xDFFF,
  utf8_4_min = 0x10000,
  utf8_4_max = 0x10FFFF,
};

enum : uint8_t {
  utf8_2_octet_len = 2,
  utf8_3_octet_len = 3,
  utf8_4_octet_len = 4,
  utf8_2_tail_cnt = 1,
  utf8_3_tail_cnt = 2,
  utf8_4_tail_cnt = 3,
  utf8_tail_bit_cnt = 6,
};

static bool utf8_1(uint32_t octet) {
  return (octet & utf8_1_mask) == utf8_1_tag;
}

static bool utf8_2(uint32_t octet) {
  return (octet & utf8_2_mask) == utf8_2_tag;
}

static bool utf8_3(uint32_t octet) {
  return (octet & utf8_3_mask) == utf8_3_tag;
}

static bool utf8_4(uint32_t octet) {
  return (octet & utf8_4_mask) == utf8_4_tag;
}

static bool utf8_tail(uint32_t octet) {
  return (octet & utf8_tail_mask) == utf8_tail_tag;
}

static bool char_number_ok(const struct utf8c *state) {
  switch (state->octet_len) {
  case utf8_2_octet_len:
    return state->char_number >= utf8_2_min;
  case utf8_3_octet_len:
    return state->char_number >= utf8_3_min &&
           (state->char_number < surrogate_min || state->char_number > surrogate_max);
  case utf8_4_octet_len:
    return state->char_number >= utf8_4_min && state->char_number <= utf8_4_max;
  default:
    abort();
  }
}

static bool reject(struct utf8c *state) {
  state->illegal = true;
  return false;
}

static void utf8_2_start(struct utf8c *state, uint32_t octet) {
  state->char_number = octet & utf8_2_payload;
  state->octet_len = utf8_2_octet_len;
  state->remaining_octet_cnt = utf8_2_tail_cnt;
}

static void utf8_3_start(struct utf8c *state, uint32_t octet) {
  state->char_number = octet & utf8_3_payload;
  state->octet_len = utf8_3_octet_len;
  state->remaining_octet_cnt = utf8_3_tail_cnt;
}

static void utf8_4_start(struct utf8c *state, uint32_t octet) {
  state->char_number = octet & utf8_4_payload;
  state->octet_len = utf8_4_octet_len;
  state->remaining_octet_cnt = utf8_4_tail_cnt;
}

static bool feed_lead(struct utf8c *state, uint32_t octet) {
  if (utf8_1(octet)) return true;
  if (utf8_2(octet)) {
    utf8_2_start(state, octet);
    return true;
  }
  if (utf8_3(octet)) {
    utf8_3_start(state, octet);
    return true;
  }
  if (utf8_4(octet)) {
    utf8_4_start(state, octet);
    return true;
  }
  return reject(state);
}

static bool feed_tail(struct utf8c *state, uint32_t octet) {
  if (!utf8_tail(octet)) return reject(state);
  state->char_number <<= utf8_tail_bit_cnt;
  state->char_number |= octet & utf8_tail_payload;
  state->remaining_octet_cnt--;
  if (state->remaining_octet_cnt != 0) return true;
  if (!char_number_ok(state)) return reject(state);
  state->char_number = 0;
  state->octet_len = 0;
  return true;
}

struct utf8c utf8c_init() {
  return (struct utf8c){};
}

bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length) {
  if (state == nullptr) abort();
  if (length != 0 && src == nullptr) abort();
  if (state->illegal) return false;
  if (length == 0) return true;

  for (const uint8_t *cursor = src; cursor < src + length; cursor++) {
    uint32_t const octet = *cursor;
    if (state->remaining_octet_cnt == 0) {
      if (!feed_lead(state, octet)) return false;
    } else {
      if (!feed_tail(state, octet)) return false;
    }
  }
  return true;
}

bool utf8c_finish(const struct utf8c *state) {
  if (state == nullptr) abort();
  return !state->illegal && state->remaining_octet_cnt == 0;
}
