#include "utf8c.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum : uint8_t { tail_min = 0x80, tail_max = 0xBF };

struct lead {
  uint8_t lead_min;
  uint8_t lead_max;
  uint8_t rest_cnt;
  uint8_t cont_min;
  uint8_t cont_max;
};

static const struct lead ascii = {
    .lead_min = 0x00,
    .lead_max = 0x7F,
    .rest_cnt = 0,
    .cont_min = 0x00,
    .cont_max = 0x00,
};

static const struct lead two_byte = {
    .lead_min = 0xC2,
    .lead_max = 0xDF,
    .rest_cnt = 1,
    .cont_min = tail_min,
    .cont_max = tail_max,
};

static const struct lead three_byte_e0 = {
    .lead_min = 0xE0,
    .lead_max = 0xE0,
    .rest_cnt = 2,
    .cont_min = 0xA0,
    .cont_max = tail_max,
};

static const struct lead three_byte = {
    .lead_min = 0xE1,
    .lead_max = 0xEC,
    .rest_cnt = 2,
    .cont_min = tail_min,
    .cont_max = tail_max,
};

static const struct lead three_byte_ed = {
    .lead_min = 0xED,
    .lead_max = 0xED,
    .rest_cnt = 2,
    .cont_min = tail_min,
    .cont_max = 0x9F,
};

static const struct lead three_byte_high = {
    .lead_min = 0xEE,
    .lead_max = 0xEF,
    .rest_cnt = 2,
    .cont_min = tail_min,
    .cont_max = tail_max,
};

static const struct lead four_byte_f0 = {
    .lead_min = 0xF0,
    .lead_max = 0xF0,
    .rest_cnt = 3,
    .cont_min = 0x90,
    .cont_max = tail_max,
};

static const struct lead four_byte = {
    .lead_min = 0xF1,
    .lead_max = 0xF3,
    .rest_cnt = 3,
    .cont_min = tail_min,
    .cont_max = tail_max,
};

static const struct lead four_byte_f4 = {
    .lead_min = 0xF4,
    .lead_max = 0xF4,
    .rest_cnt = 3,
    .cont_min = tail_min,
    .cont_max = 0x8F,
};

static const struct lead *const leads[] = {
    &ascii,           &two_byte,     &three_byte_e0, &three_byte,   &three_byte_ed,
    &three_byte_high, &four_byte_f0, &four_byte,     &four_byte_f4,
};

static const size_t lead_cnt = sizeof leads / sizeof leads[0];

static const struct lead *lead_for(uint8_t octet) {
  for (size_t i = 0; i < lead_cnt; i++) {
    const struct lead *lead = leads[i];
    if (octet >= lead->lead_min && octet <= lead->lead_max) return lead;
  }
  return nullptr;
}

struct utf8c utf8c_init() { return (struct utf8c){}; }

bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length) {
  if (state == nullptr) abort();
  if (length != 0 && src == nullptr) abort();
  if (state->illegal) return false;

  for (size_t i = 0; i < length; i++) {
    uint8_t octet = src[i];
    if (state->rest_cnt != 0) {
      if (octet < state->next_min || octet > state->next_max) {
        state->illegal = true;
        return false;
      }
      state->rest_cnt--;
      if (state->rest_cnt != 0) {
        state->next_min = tail_min;
        state->next_max = tail_max;
      }
      continue;
    }

    const struct lead *lead = lead_for(octet);
    if (lead == nullptr) {
      state->illegal = true;
      return false;
    }
    state->rest_cnt = lead->rest_cnt;
    state->next_min = lead->cont_min;
    state->next_max = lead->cont_max;
  }
  return true;
}

bool utf8c_finish(const struct utf8c *state) {
  if (state == nullptr) abort();
  return !state->illegal && state->rest_cnt == 0;
}
