#include <utf8c.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  struct utf8c state = utf8c_init();
  uint8_t buf[BUFSIZ];
  for (;;) {
    size_t byte_cnt = fread(buf, 1, sizeof buf, stdin);
    if (byte_cnt == 0) break;
    if (!utf8c_feed(&state, buf, byte_cnt)) return 1;
  }
  if (ferror(stdin) != 0) abort();
  return utf8c_finish(&state) ? 0 : 1;
}
