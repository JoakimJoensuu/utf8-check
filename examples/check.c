#include <utf8c.h>

#include <stdio.h>

int main() {
  struct utf8c state = utf8c_create();
  unsigned char units[BUFSIZ];
  for (;;) {
    size_t unit_count = fread(units, 1, sizeof(units), stdin);
    if (unit_count == 0) break;
    for (size_t index = 0; index < unit_count; index++) {
      if (!utf8c_feed_bits(&state, units[index])) return 1;
    }
  }
  if (ferror(stdin) != 0) return 1;
  return utf8c_is_finished(&state) ? 0 : 1;
}
