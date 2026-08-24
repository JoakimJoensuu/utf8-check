#include <utf8.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int fail_at(const char *file, int line) {
  (void)fprintf(stderr, "%s:%d\n", file, line);
  return 1;
}

#define EXPECT(cond)                                                                               \
  if ((cond) == 0) return fail_at(__FILE__, __LINE__)

static int expect_ok(const uint8_t *src, size_t length) {
  EXPECT(utf8_check(src, length) == 0);
  return 0;
}

static int expect_bad(const uint8_t *src, size_t length) {
  EXPECT(utf8_check(src, length) != 0);
  return 0;
}

static int test_empty() {
  struct utf8 state;
  utf8_init(&state);
  EXPECT(utf8_feed(&state, nullptr, 0) == 0);
  EXPECT(utf8_finish(&state) == 0);
  EXPECT(utf8_check(nullptr, 0) == 0);
  return 0;
}

static int test_ascii() {
  static const uint8_t ascii[] = {0x00, 0x01, 'A', 'z', 0x7F};
  return expect_ok(ascii, sizeof ascii);
}

static int test_two_byte() {
  static const uint8_t u0080[] = {0xC2, 0x80};
  static const uint8_t u07ff[] = {0xDF, 0xBF};
  EXPECT(expect_ok(u0080, sizeof u0080) == 0);
  EXPECT(expect_ok(u07ff, sizeof u07ff) == 0);
  return 0;
}

static int test_three_byte() {
  static const uint8_t u0800[] = {0xE0, 0xA0, 0x80};
  static const uint8_t ud7ff[] = {0xED, 0x9F, 0xBF};
  static const uint8_t ue000[] = {0xEE, 0x80, 0x80};
  static const uint8_t uffff[] = {0xEF, 0xBF, 0xBF};
  EXPECT(expect_ok(u0800, sizeof u0800) == 0);
  EXPECT(expect_ok(ud7ff, sizeof ud7ff) == 0);
  EXPECT(expect_ok(ue000, sizeof ue000) == 0);
  EXPECT(expect_ok(uffff, sizeof uffff) == 0);
  return 0;
}

static int test_four_byte() {
  static const uint8_t u10000[] = {0xF0, 0x90, 0x80, 0x80};
  static const uint8_t u10ffff[] = {0xF4, 0x8F, 0xBF, 0xBF};
  EXPECT(expect_ok(u10000, sizeof u10000) == 0);
  EXPECT(expect_ok(u10ffff, sizeof u10ffff) == 0);
  return 0;
}

static int test_split() {
  static const uint8_t seq[] = {0xF0, 0x9F, 0x92, 0xA9};
  struct utf8 state;
  utf8_init(&state);
  EXPECT(utf8_feed(&state, seq, 1) == 0);
  EXPECT(utf8_finish(&state) != 0);
  EXPECT(utf8_feed(&state, seq + 1, 2) == 0);
  EXPECT(utf8_finish(&state) != 0);
  EXPECT(utf8_feed(&state, seq + 3, 1) == 0);
  EXPECT(utf8_finish(&state) == 0);
  return 0;
}

static int test_unfinished() {
  static const uint8_t lead[] = {0xC2};
  struct utf8 state;
  utf8_init(&state);
  EXPECT(utf8_feed(&state, lead, sizeof lead) == 0);
  EXPECT(utf8_finish(&state) != 0);
  return 0;
}

static int test_overlong() {
  static const uint8_t over_c0[] = {0xC0, 0x80};
  static const uint8_t over_c1[] = {0xC1, 0xBF};
  static const uint8_t over_e0[] = {0xE0, 0x9F, 0xBF};
  static const uint8_t over_f0[] = {0xF0, 0x8F, 0xBF, 0xBF};
  EXPECT(expect_bad(over_c0, sizeof over_c0) == 0);
  EXPECT(expect_bad(over_c1, sizeof over_c1) == 0);
  EXPECT(expect_bad(over_e0, sizeof over_e0) == 0);
  EXPECT(expect_bad(over_f0, sizeof over_f0) == 0);
  return 0;
}

static int test_surrogate() {
  static const uint8_t ud800[] = {0xED, 0xA0, 0x80};
  static const uint8_t udfff[] = {0xED, 0xBF, 0xBF};
  EXPECT(expect_bad(ud800, sizeof ud800) == 0);
  EXPECT(expect_bad(udfff, sizeof udfff) == 0);
  return 0;
}

static int test_too_big() {
  static const uint8_t u110000[] = {0xF4, 0x90, 0x80, 0x80};
  static const uint8_t lead_f5[] = {0xF5, 0x80, 0x80, 0x80};
  EXPECT(expect_bad(u110000, sizeof u110000) == 0);
  EXPECT(expect_bad(lead_f5, sizeof lead_f5) == 0);
  return 0;
}

static int test_truncated() {
  static const uint8_t two[] = {0xC2};
  static const uint8_t three[] = {0xE2, 0x82};
  static const uint8_t four[] = {0xF0, 0x90, 0x80};
  EXPECT(expect_bad(two, sizeof two) == 0);
  EXPECT(expect_bad(three, sizeof three) == 0);
  EXPECT(expect_bad(four, sizeof four) == 0);
  return 0;
}

static int test_stray() {
  static const uint8_t cont[] = {0x80};
  static const uint8_t after[] = {'A', 0xBF};
  EXPECT(expect_bad(cont, sizeof cont) == 0);
  EXPECT(expect_bad(after, sizeof after) == 0);
  return 0;
}

static int test_reject_sticks() {
  static const uint8_t stray[] = {0x80};
  static const uint8_t letter[] = {'A'};
  struct utf8 state;
  utf8_init(&state);
  EXPECT(utf8_feed(&state, stray, sizeof stray) != 0);
  EXPECT(utf8_feed(&state, letter, sizeof letter) != 0);
  return 0;
}

int main() {
  if (test_empty() != 0) return 1;
  if (test_ascii() != 0) return 1;
  if (test_two_byte() != 0) return 1;
  if (test_three_byte() != 0) return 1;
  if (test_four_byte() != 0) return 1;
  if (test_split() != 0) return 1;
  if (test_unfinished() != 0) return 1;
  if (test_overlong() != 0) return 1;
  if (test_surrogate() != 0) return 1;
  if (test_too_big() != 0) return 1;
  if (test_truncated() != 0) return 1;
  if (test_stray() != 0) return 1;
  if (test_reject_sticks() != 0) return 1;
  return 0;
}
