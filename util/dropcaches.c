/** for linux,

    echo 3 | sudo tee /proc/sys/vm/drop_caches

    might work but might prompt for password

    you can't +suid a shell script, but you could +suid this.

*/
#include <stdio.h>

char const* const dropname = "/proc/sys/vm/drop_caches";

int main() {
  FILE *f = fopen(dropname, "w");
  if (f) {
    fprintf(f, "3\n");
    fclose(f);
  } else {
    fprintf(stderr, "Couldn't write to %s - you must run as (setuid) root on a Linux system?\n", dropname);
    return 1;
  }
  return 0;
}
