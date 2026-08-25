#include "utf8c.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum : uint8_t { cont_min = 0x80, cont_max = 0xBF };

struct seq {
  uint8_t lead_min;
  uint8_t lead_max;
  uint8_t cont_cnt;
  uint8_t first_cont_min;
  uint8_t first_cont_max;
};

static const struct seq ascii = {
    .lead_min = 0x00,
    .lead_max = 0x7F,
    .cont_cnt = 0,
    .first_cont_min = 0x00,
    .first_cont_max = 0x00,
};

static const struct seq two_byte = {
    .lead_min = 0xC2,
    .lead_max = 0xDF,
    .cont_cnt = 1,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct seq three_byte_e0 = {
    .lead_min = 0xE0,
    .lead_max = 0xE0,
    .cont_cnt = 2,
    .first_cont_min = 0xA0,
    .first_cont_max = cont_max,
};

static const struct seq three_byte = {
    .lead_min = 0xE1,
    .lead_max = 0xEC,
    .cont_cnt = 2,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct seq three_byte_ed = {
    .lead_min = 0xED,
    .lead_max = 0xED,
    .cont_cnt = 2,
    .first_cont_min = cont_min,
    .first_cont_max = 0x9F,
};

static const struct seq three_byte_high = {
    .lead_min = 0xEE,
    .lead_max = 0xEF,
    .cont_cnt = 2,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct seq four_byte_f0 = {
    .lead_min = 0xF0,
    .lead_max = 0xF0,
    .cont_cnt = 3,
    .first_cont_min = 0x90,
    .first_cont_max = cont_max,
};

static const struct seq four_byte = {
    .lead_min = 0xF1,
    .lead_max = 0xF3,
    .cont_cnt = 3,
    .first_cont_min = cont_min,
    .first_cont_max = cont_max,
};

static const struct seq four_byte_f4 = {
    .lead_min = 0xF4,
    .lead_max = 0xF4,
    .cont_cnt = 3,
    .first_cont_min = cont_min,
    .first_cont_max = 0x8F,
};

static const struct seq *const seqs[] = {
    &ascii,           &two_byte,     &three_byte_e0, &three_byte,   &three_byte_ed,
    &three_byte_high, &four_byte_f0, &four_byte,     &four_byte_f4,
};

static const size_t seq_cnt = sizeof seqs / sizeof seqs[0];

static const struct seq *seq_for(uint8_t octet) {
  for (size_t i = 0; i < seq_cnt; i++) {
    const struct seq *seq = seqs[i];
    if (octet >= seq->lead_min && octet <= seq->lead_max) return seq;
  }
  return nullptr;
}

struct utf8c utf8c_init() { return (struct utf8c){}; }

bool utf8c_feed(struct utf8c *state, const uint8_t *src, size_t length) {
  if (state == nullptr) abort();
  if (length != 0 && src == nullptr) abort();
  if (state->illegal) return false;
  if (length == 0) return true;

  for (const uint8_t *octet = src; octet < src + length; octet++) {
    if (state->remaining_octet_cnt == 0) {
      const struct seq *seq = seq_for(*octet);
      if (seq == nullptr) {
        state->illegal = true;
        return false;
      }
      state->remaining_octet_cnt = seq->cont_cnt;
      state->next_octet_min = seq->first_cont_min;
      state->next_octet_max = seq->first_cont_max;
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
