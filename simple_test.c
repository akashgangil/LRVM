#include <stdio.h>
#include <stdlib.h>

#include "rvm.h"

int main(int argc, char **argv) {
  rvm_t rvm;
  char *segs[1];
  segment_t segment;

  rvm = rvm_init("rvm_segments");
  printf("%s\n", rvm.directory);

  rvm_destroy(rvm, "testseg");

  segs[0] = (char *) rvm_map(rvm, "testseg", 1000);

  sprintf(segs[0], "hello, world");

  return 0;
}
