#include <utf8.h>

#include <cgreen/assertions.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/runner.h>
#include <cgreen/suite.h>
#include <cgreen/text_reporter.h>
#include <cgreen/unit.h>
#include <stddef.h>
#include <stdint.h>

static int well_formed(const uint8_t *src, size_t length) {
  struct utf8 state;
  utf8_init(&state);
  if (utf8_feed(&state, src, length) != 0) return 1;
  return utf8_finish(&state);
}

Ensure(empty) {
  struct utf8 state;
  utf8_init(&state);
  assert_that(utf8_feed(&state, nullptr, 0), is_equal_to(0));
  assert_that(utf8_finish(&state), is_equal_to(0));
  assert_that(well_formed(nullptr, 0), is_equal_to(0));
}

Ensure(ascii) {
  static const uint8_t bytes[] = {0x00, 0x01, 'A', 'z', 0x7F};
  assert_that(well_formed(bytes, sizeof bytes), is_equal_to(0));
}

Ensure(two_byte) {
  static const uint8_t u0080[] = {0xC2, 0x80};
  static const uint8_t u07ff[] = {0xDF, 0xBF};
  assert_that(well_formed(u0080, sizeof u0080), is_equal_to(0));
  assert_that(well_formed(u07ff, sizeof u07ff), is_equal_to(0));
}

Ensure(three_byte) {
  static const uint8_t u0800[] = {0xE0, 0xA0, 0x80};
  static const uint8_t ud7ff[] = {0xED, 0x9F, 0xBF};
  static const uint8_t ue000[] = {0xEE, 0x80, 0x80};
  static const uint8_t uffff[] = {0xEF, 0xBF, 0xBF};
  assert_that(well_formed(u0800, sizeof u0800), is_equal_to(0));
  assert_that(well_formed(ud7ff, sizeof ud7ff), is_equal_to(0));
  assert_that(well_formed(ue000, sizeof ue000), is_equal_to(0));
  assert_that(well_formed(uffff, sizeof uffff), is_equal_to(0));
}

Ensure(four_byte) {
  static const uint8_t u10000[] = {0xF0, 0x90, 0x80, 0x80};
  static const uint8_t u10ffff[] = {0xF4, 0x8F, 0xBF, 0xBF};
  assert_that(well_formed(u10000, sizeof u10000), is_equal_to(0));
  assert_that(well_formed(u10ffff, sizeof u10ffff), is_equal_to(0));
}

Ensure(split) {
  static const uint8_t seq[] = {0xF0, 0x9F, 0x92, 0xA9};
  struct utf8 state;
  utf8_init(&state);
  assert_that(utf8_feed(&state, seq, 1), is_equal_to(0));
  assert_that(utf8_finish(&state), is_not_equal_to(0));
  assert_that(utf8_feed(&state, seq + 1, 2), is_equal_to(0));
  assert_that(utf8_finish(&state), is_not_equal_to(0));
  assert_that(utf8_feed(&state, seq + 3, 1), is_equal_to(0));
  assert_that(utf8_finish(&state), is_equal_to(0));
}

Ensure(unfinished) {
  static const uint8_t lead[] = {0xC2};
  struct utf8 state;
  utf8_init(&state);
  assert_that(utf8_feed(&state, lead, sizeof lead), is_equal_to(0));
  assert_that(utf8_finish(&state), is_not_equal_to(0));
}

Ensure(overlong) {
  static const uint8_t over_c0[] = {0xC0, 0x80};
  static const uint8_t over_c1[] = {0xC1, 0xBF};
  static const uint8_t over_e0[] = {0xE0, 0x9F, 0xBF};
  static const uint8_t over_f0[] = {0xF0, 0x8F, 0xBF, 0xBF};
  assert_that(well_formed(over_c0, sizeof over_c0), is_not_equal_to(0));
  assert_that(well_formed(over_c1, sizeof over_c1), is_not_equal_to(0));
  assert_that(well_formed(over_e0, sizeof over_e0), is_not_equal_to(0));
  assert_that(well_formed(over_f0, sizeof over_f0), is_not_equal_to(0));
}

Ensure(surrogate) {
  static const uint8_t ud800[] = {0xED, 0xA0, 0x80};
  static const uint8_t udfff[] = {0xED, 0xBF, 0xBF};
  assert_that(well_formed(ud800, sizeof ud800), is_not_equal_to(0));
  assert_that(well_formed(udfff, sizeof udfff), is_not_equal_to(0));
}

Ensure(too_big) {
  static const uint8_t u110000[] = {0xF4, 0x90, 0x80, 0x80};
  static const uint8_t lead_f5[] = {0xF5, 0x80, 0x80, 0x80};
  assert_that(well_formed(u110000, sizeof u110000), is_not_equal_to(0));
  assert_that(well_formed(lead_f5, sizeof lead_f5), is_not_equal_to(0));
}

Ensure(truncated) {
  static const uint8_t two[] = {0xC2};
  static const uint8_t three[] = {0xE2, 0x82};
  static const uint8_t four[] = {0xF0, 0x90, 0x80};
  assert_that(well_formed(two, sizeof two), is_not_equal_to(0));
  assert_that(well_formed(three, sizeof three), is_not_equal_to(0));
  assert_that(well_formed(four, sizeof four), is_not_equal_to(0));
}

Ensure(stray) {
  static const uint8_t cont[] = {0x80};
  static const uint8_t after[] = {'A', 0xBF};
  assert_that(well_formed(cont, sizeof cont), is_not_equal_to(0));
  assert_that(well_formed(after, sizeof after), is_not_equal_to(0));
}

Ensure(reject_sticks) {
  static const uint8_t stray[] = {0x80};
  static const uint8_t letter[] = {'A'};
  struct utf8 state;
  utf8_init(&state);
  assert_that(utf8_feed(&state, stray, sizeof stray), is_not_equal_to(0));
  assert_that(utf8_feed(&state, letter, sizeof letter), is_not_equal_to(0));
}

int main() {
  auto suite = create_test_suite();
  add_test(suite, empty);
  add_test(suite, ascii);
  add_test(suite, two_byte);
  add_test(suite, three_byte);
  add_test(suite, four_byte);
  add_test(suite, split);
  add_test(suite, unfinished);
  add_test(suite, overlong);
  add_test(suite, surrogate);
  add_test(suite, too_big);
  add_test(suite, truncated);
  add_test(suite, stray);
  add_test(suite, reject_sticks);
  return run_test_suite(suite, create_text_reporter());
}
