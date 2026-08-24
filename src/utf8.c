#include "utf8.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum : uint8_t { tail_min = 0x80, tail_max = 0xBF };

struct core {
  uint8_t rest_cnt;
  uint8_t next_min;
  uint8_t next_max;
  uint8_t illegal;
};

static_assert(sizeof(struct utf8) >= sizeof(struct core));
static_assert(alignof(struct utf8) >= alignof(struct core));

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

static struct core load(const struct utf8 *state) {
  struct core core;
  memcpy(&core, state->opaque, sizeof core);
  return core;
}

static void save(struct utf8 *state, const struct core *core) {
  memcpy(state->opaque, core, sizeof *core);
}

static void require_state(const struct utf8 *state) {
  if (state == nullptr) abort();
}

static void require_src(const uint8_t *src, size_t length) {
  if (length != 0 && src == nullptr) abort();
}

void utf8_init(struct utf8 *state) {
  require_state(state);
  *state = (struct utf8){};
}

int utf8_feed(struct utf8 *state, const uint8_t *src, size_t length) {
  require_state(state);
  require_src(src, length);
  struct core core = load(state);
  if (core.illegal != 0) return 1;

  for (size_t i = 0; i < length; i++) {
    uint8_t octet = src[i];
    if (core.rest_cnt != 0) {
      if (octet < core.next_min || octet > core.next_max) {
        core.illegal = 1;
        save(state, &core);
        return 1;
      }
      core.rest_cnt--;
      if (core.rest_cnt != 0) {
        core.next_min = tail_min;
        core.next_max = tail_max;
      }
      continue;
    }

    const struct lead *lead = lead_for(octet);
    if (lead == nullptr) {
      core.illegal = 1;
      save(state, &core);
      return 1;
    }
    core.rest_cnt = lead->rest_cnt;
    core.next_min = lead->cont_min;
    core.next_max = lead->cont_max;
  }
  save(state, &core);
  return 0;
}

int utf8_finish(const struct utf8 *state) {
  require_state(state);
  return load(state).rest_cnt != 0;
}
