#include <utf8c.h>

#include <stdint.h>
#include <stdio.h>

int main() {
  struct utf8c state = utf8c_init();
  uint8_t octets[BUFSIZ];
  for (;;) {
    size_t octet_cnt = fread(octets, 1, sizeof octets, stdin);
    if (octet_cnt == 0) break;
    if (!utf8c_feed(&state, octets, octet_cnt)) return 1;
  }
  if (ferror(stdin) != 0) return 1;
  return utf8c_finish(&state) ? 0 : 1;
}
