#include <stdio.h>
#include <stdlib.h>

#include "rvm.h"

int main(int argc, char **argv) {
  rvm_t rvm;
  segment_t segment;
  
  rvm = rvm_init("rvm_segments");
  printf("%s\n", rvm.directory);

  segment = rvm_map(rvm, "testseg", 1000);
  printf("%s %d\n", segment.name, segment.size);

  return 0;
}
