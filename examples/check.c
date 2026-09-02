#include <utf8c.h>

#include <stdio.h>

int main() {
  struct utf8c state = utf8c_create();
  unsigned char octets[BUFSIZ];
  for (;;) {
    size_t octet_count = fread(octets, 1, sizeof(octets), stdin);
    if (octet_count == 0) break;
    for (size_t index = 0; index < octet_count; index++) {
      if (!utf8c_feed_octet(&state, octets[index])) return 1;
    }
  }
  if (ferror(stdin) != 0) return 1;
  return utf8c_is_finished(&state) ? 0 : 1;
}
