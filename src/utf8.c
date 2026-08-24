#include "utf8.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { tail_min = 0x80, tail_max = 0xBF };

struct rule {
  uint8_t lead_min;
  uint8_t lead_max;
  uint8_t rest_cnt;
  uint8_t first_min;
  uint8_t first_max;
};

// clang-format off
static const struct rule rules[] = {
    {0x00, 0x7F, 0, 0x00, 0x00},
    {0xC2, 0xDF, 1, tail_min, tail_max},
    {0xE0, 0xE0, 2, 0xA0, tail_max},
    {0xE1, 0xEC, 2, tail_min, tail_max},
    {0xED, 0xED, 2, tail_min, 0x9F},
    {0xEE, 0xEF, 2, tail_min, tail_max},
    {0xF0, 0xF0, 3, 0x90, tail_max},
    {0xF1, 0xF3, 3, tail_min, tail_max},
    {0xF4, 0xF4, 3, tail_min, 0x8F},
};
// clang-format on

enum { rule_cnt = sizeof rules / sizeof rules[0] };

static const struct rule *rule_for(uint8_t octet) {
  for (size_t i = 0; i < rule_cnt; i++) {
    const struct rule *rule = &rules[i];
    if (octet >= rule->lead_min && octet <= rule->lead_max) return rule;
  }
  return nullptr;
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
  if (state->illegal != 0) return 1;

  for (size_t i = 0; i < length; i++) {
    uint8_t octet = src[i];
    if (state->rest_cnt != 0) {
      if (octet < state->next_min || octet > state->next_max) {
        state->illegal = 1;
        return 1;
      }
      state->rest_cnt--;
      if (state->rest_cnt != 0) {
        state->next_min = tail_min;
        state->next_max = tail_max;
      }
      continue;
    }

    const struct rule *rule = rule_for(octet);
    if (rule == nullptr) {
      state->illegal = 1;
      return 1;
    }
    state->rest_cnt = rule->rest_cnt;
    state->next_min = rule->first_min;
    state->next_max = rule->first_max;
  }
  return 0;
}

int utf8_finish(const struct utf8 *state) {
  require_state(state);
  return state->rest_cnt != 0;
}

int utf8_check(const uint8_t *src, size_t length) {
  struct utf8 state;
  utf8_init(&state);
  if (utf8_feed(&state, src, length) != 0) return 1;
  return utf8_finish(&state);
}
