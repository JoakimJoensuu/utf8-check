#include "utf8c.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum : uint8_t {
  cont_min = 0x80,
  cont_max = 0xBF,
  two_min = 0xC2,
  two_max = 0xDF,
  lead_e0 = 0xE0,
  three_min = 0xE1,
  three_max = 0xEC,
  lead_ed = 0xED,
  lead_ee = 0xEE,
  lead_ef = 0xEF,
  lead_f0 = 0xF0,
  four_min = 0xF1,
  four_max = 0xF3,
  lead_f4 = 0xF4,
};

struct form {
  uint8_t cont_cnt;
  uint8_t first_cont_min;
  uint8_t first_cont_max;
};

static const struct form ascii = {
    .cont_cnt = 0,
    .first_cont_min = 0x00,
    .first_cont_max = 0x00,
};

static const struct form two_byte = {
    .cont_cnt = 1,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct form three_byte_e0 = {
    .cont_cnt = 2,
    .first_cont_min = 0xA0,
    .first_cont_max = cont_max,
};

static const struct form three_byte = {
    .cont_cnt = 2,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct form three_byte_ed = {
    .cont_cnt = 2,
    .first_cont_min = cont_min,
    .first_cont_max = 0x9F,
};

static const struct form three_byte_high = {
    .cont_cnt = 2,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct form four_byte_f0 = {
    .cont_cnt = 3,
    .first_cont_min = 0x90,
    .first_cont_max = cont_max,
};

static const struct form four_byte = {
    .cont_cnt = 3,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct form four_byte_f4 = {
    .cont_cnt = 3,
    .first_cont_min = cont_min,
    .first_cont_max = 0x8F,
};

static const struct form *form_for(uint8_t lead_octet) {
  const struct form *form = nullptr;
  if (lead_octet < cont_min)
    form = &ascii;
  else if (lead_octet >= two_min && lead_octet <= two_max)
    form = &two_byte;
  else if (lead_octet == lead_e0)
    form = &three_byte_e0;
  else if (lead_octet >= three_min && lead_octet <= three_max)
    form = &three_byte;
  else if (lead_octet == lead_ed)
    form = &three_byte_ed;
  else if (lead_octet >= lead_ee && lead_octet <= lead_ef)
    form = &three_byte_high;
  else if (lead_octet == lead_f0)
    form = &four_byte_f0;
  else if (lead_octet >= four_min && lead_octet <= four_max)
    form = &four_byte;
  else if (lead_octet == lead_f4)
    form = &four_byte_f4;
  return form;
}

struct utf8c utf8c_init() {
  return (struct utf8c){};
}

bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length) {
  if (state == nullptr) abort();
  if (length != 0 && src == nullptr) abort();
  if (state->illegal) return false;
  if (length == 0) return true;

  for (const uint8_t *octet = src; octet < src + length; octet++) {
    if (state->remaining_octet_cnt == 0) {
      const struct form *form = form_for(*octet);
      if (form == nullptr) {
        state->illegal = true;
        return false;
      }
      state->remaining_octet_cnt = form->cont_cnt;
      state->next_octet_min = form->first_cont_min;
      state->next_octet_max = form->first_cont_max;
      continue;
    }

    if (*octet < state->next_octet_min || *octet > state->next_octet_max) {
      state->illegal = true;
      return false;
    }
    state->remaining_octet_cnt--;
    if (state->remaining_octet_cnt == 0) continue;
    state->next_octet_min = cont_min;
    state->next_octet_max = cont_max;
  }
  return true;
}

bool utf8c_finish(const struct utf8c *state) {
  if (state == nullptr) abort();
  return !state->illegal && state->remaining_octet_cnt == 0;
}
