#include <stdio.h>
#include <stdlib.h>

#include "rvm.h"

int main(int argc, char **argv) {
  rvm_t rvm;

  rvm = rvm_init("rvm_segments");
  printf("%s\n", rvm.directory);
}
