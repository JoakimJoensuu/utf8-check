#include <utf8.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  struct utf8 state;
  uint8_t buf[BUFSIZ];
  utf8_init(&state);
  for (;;) {
    size_t byte_cnt = fread(buf, 1, sizeof buf, stdin);
    if (byte_cnt == 0) break;
    if (utf8_feed(&state, buf, byte_cnt) != 0) return 1;
  }
  if (ferror(stdin) != 0) abort();
  return utf8_finish(&state) != 0;
}
