#include <utf8c.h>

#include <cgreen/assertions.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/reporter.h>
#include <cgreen/runner.h>
#include <cgreen/suite.h>
#include <cgreen/text_reporter.h>
#include <cgreen/unit.h>
#include <limits.h>
#include <stddef.h>

static bool feed_octets(struct utf8c *state, const unsigned char *octets, size_t length) {
  for (size_t index = 0; index < length; index++) {
    if (!utf8c_feed_octet(state, octets[index])) return false;
  }
  return true;
}

static bool well_formed(const unsigned char *octets, size_t length) {
  struct utf8c state = utf8c_create();
  return feed_octets(&state, octets, length) && utf8c_is_finished(&state);
}

Ensure(empty) {
  struct utf8c state = utf8c_create();
  assert_that(utf8c_is_finished(&state), is_true);
  assert_that(well_formed(nullptr, 0), is_true);
}

Ensure(ascii) {
  static const unsigned char octets[] = {0x00, 0x01, 'A', 'z', 0x7F};
  assert_that(well_formed(octets, sizeof(octets)), is_true);
}

Ensure(two_byte) {
  static const unsigned char u0080[] = {0xC2, 0x80};
  static const unsigned char u07ff[] = {0xDF, 0xBF};
  assert_that(well_formed(u0080, sizeof(u0080)), is_true);
  assert_that(well_formed(u07ff, sizeof(u07ff)), is_true);
}

Ensure(three_byte) {
  static const unsigned char u0800[] = {0xE0, 0xA0, 0x80};
  static const unsigned char ud7ff[] = {0xED, 0x9F, 0xBF};
  static const unsigned char ue000[] = {0xEE, 0x80, 0x80};
  static const unsigned char uffff[] = {0xEF, 0xBF, 0xBF};
  assert_that(well_formed(u0800, sizeof(u0800)), is_true);
  assert_that(well_formed(ud7ff, sizeof(ud7ff)), is_true);
  assert_that(well_formed(ue000, sizeof(ue000)), is_true);
  assert_that(well_formed(uffff, sizeof(uffff)), is_true);
}

Ensure(four_byte) {
  static const unsigned char u10000[] = {0xF0, 0x90, 0x80, 0x80};
  static const unsigned char u10ffff[] = {0xF4, 0x8F, 0xBF, 0xBF};
  assert_that(well_formed(u10000, sizeof(u10000)), is_true);
  assert_that(well_formed(u10ffff, sizeof(u10ffff)), is_true);
}

Ensure(split) {
  static const unsigned char seq[] = {0xF0, 0x9F, 0x92, 0xA9};
  struct utf8c state = utf8c_create();
  assert_that(feed_octets(&state, seq, 1), is_true);
  assert_that(utf8c_is_finished(&state), is_false);
  assert_that(feed_octets(&state, seq + 1, 2), is_true);
  assert_that(utf8c_is_finished(&state), is_false);
  assert_that(feed_octets(&state, seq + 3, 1), is_true);
  assert_that(utf8c_is_finished(&state), is_true);
}

#if CHAR_BIT >= 16
Ensure(two_octets_per_word) {
  struct utf8c state = utf8c_create();
  assert_that(utf8c_feed_bits(&state, (unsigned char)0xC280), is_true);
  assert_that(utf8c_is_finished(&state), is_true);
}
#endif

Ensure(unfinished) {
  static const unsigned char initial[] = {0xC2};
  struct utf8c state = utf8c_create();
  assert_that(feed_octets(&state, initial, sizeof(initial)), is_true);
  assert_that(utf8c_is_finished(&state), is_false);
}

Ensure(overlong) {
  static const unsigned char over_c0[] = {0xC0, 0x80};
  static const unsigned char over_c1[] = {0xC1, 0xBF};
  static const unsigned char over_e0[] = {0xE0, 0x9F, 0xBF};
  static const unsigned char over_f0[] = {0xF0, 0x8F, 0xBF, 0xBF};
  assert_that(well_formed(over_c0, sizeof(over_c0)), is_false);
  assert_that(well_formed(over_c1, sizeof(over_c1)), is_false);
  assert_that(well_formed(over_e0, sizeof(over_e0)), is_false);
  assert_that(well_formed(over_f0, sizeof(over_f0)), is_false);
}

Ensure(surrogate) {
  static const unsigned char ud800[] = {0xED, 0xA0, 0x80};
  static const unsigned char udfff[] = {0xED, 0xBF, 0xBF};
  assert_that(well_formed(ud800, sizeof(ud800)), is_false);
  assert_that(well_formed(udfff, sizeof(udfff)), is_false);
}

Ensure(too_big) {
  static const unsigned char u110000[] = {0xF4, 0x90, 0x80, 0x80};
  static const unsigned char initial_f5[] = {0xF5, 0x80, 0x80, 0x80};
  assert_that(well_formed(u110000, sizeof(u110000)), is_false);
  assert_that(well_formed(initial_f5, sizeof(initial_f5)), is_false);
}

Ensure(truncated) {
  static const unsigned char two[] = {0xC2};
  static const unsigned char three[] = {0xE2, 0x82};
  static const unsigned char four[] = {0xF0, 0x90, 0x80};
  assert_that(well_formed(two, sizeof(two)), is_false);
  assert_that(well_formed(three, sizeof(three)), is_false);
  assert_that(well_formed(four, sizeof(four)), is_false);
}

Ensure(stray) {
  static const unsigned char cont[] = {0x80};
  static const unsigned char after[] = {'A', 0xBF};
  assert_that(well_formed(cont, sizeof(cont)), is_false);
  assert_that(well_formed(after, sizeof(after)), is_false);
}

Ensure(reject_sticks) {
  static const unsigned char stray[] = {0x80};
  static const unsigned char letter[] = {'A'};
  struct utf8c state = utf8c_create();
  assert_that(feed_octets(&state, stray, sizeof(stray)), is_false);
  assert_that(utf8c_is_finished(&state), is_false);
  assert_that(feed_octets(&state, letter, sizeof(letter)), is_false);
  assert_that(utf8c_is_finished(&state), is_false);
}

Ensure(bad_following) {
  static const unsigned char two[] = {0xC2, 0x00};
  static const unsigned char three[] = {0xE2, 0x28, 0xA1};
  assert_that(well_formed(two, sizeof(two)), is_false);
  assert_that(well_formed(three, sizeof(three)), is_false);
}

Ensure(bad_following_sticks) {
  static const unsigned char bad[] = {0xC2, 0x00};
  static const unsigned char letter[] = {'A'};
  struct utf8c state = utf8c_create();
  assert_that(feed_octets(&state, bad, sizeof(bad)), is_false);
  assert_that(utf8c_is_finished(&state), is_false);
  assert_that(feed_octets(&state, letter, sizeof(letter)), is_false);
  assert_that(utf8c_is_finished(&state), is_false);
}

Ensure(invalid_initial) {
  static const unsigned char initial_f8[] = {0xF8, 0x80, 0x80, 0x80, 0x80};
  static const unsigned char initial_ff[] = {0xFF};
  static const unsigned char initial_fe[] = {0xFE};
  assert_that(well_formed(initial_f8, sizeof(initial_f8)), is_false);
  assert_that(well_formed(initial_ff, sizeof(initial_ff)), is_false);
  assert_that(well_formed(initial_fe, sizeof(initial_fe)), is_false);
}

Ensure(mixed) {
  static const unsigned char octets[] = {
      'A', 0xC2, 0x80, 0xE0, 0xA0, 0x80, 0xF0, 0x90, 0x80, 0x80, 'z',
  };
  assert_that(well_formed(octets, sizeof(octets)), is_true);
}

Ensure(empty_while_incomplete) {
  static const unsigned char sequence[] = {0xF0, 0x90, 0x80, 0x80};
  struct utf8c state = utf8c_create();
  assert_that(feed_octets(&state, sequence, 2), is_true);
  assert_that(utf8c_is_finished(&state), is_false);
  assert_that(feed_octets(&state, sequence + 2, 2), is_true);
  assert_that(utf8c_is_finished(&state), is_true);
}

int main() {
  auto suite = create_test_suite();
  add_test(suite, empty);
  add_test(suite, ascii);
  add_test(suite, two_byte);
  add_test(suite, three_byte);
  add_test(suite, four_byte);
  add_test(suite, split);
#if CHAR_BIT >= 16
  add_test(suite, two_octets_per_word);
#endif
  add_test(suite, unfinished);
  add_test(suite, overlong);
  add_test(suite, surrogate);
  add_test(suite, too_big);
  add_test(suite, truncated);
  add_test(suite, stray);
  add_test(suite, reject_sticks);
  add_test(suite, bad_following);
  add_test(suite, bad_following_sticks);
  add_test(suite, invalid_initial);
  add_test(suite, mixed);
  add_test(suite, empty_while_incomplete);
  auto reporter = create_text_reporter();
  int result = run_test_suite(suite, reporter);
  destroy_test_suite(suite);
  destroy_reporter(reporter);
  return result;
}
