#include <stdio.h>
#include <stdlib.h>

#include "rvm.h"

int main(int argc, char **argv) {
  
  rvm_t rvm;
  trans_t trans;

  char *seg[1];

  rvm = rvm_init("rvm_segments");
  rvm_destroy(rvm, "testseg");
  printf("Directory: %s\n", rvm.directory);
  
  seg[0] = (char*) rvm_map(rvm, "testseg", 10000);

  trans = rvm_begin_trans(rvm, 1, (void **) seg);

  rvm_about_to_modify(trans, seg[0], 0, 100);
  sprintf(seg[0], "test string");

  rvm_about_to_modify(trans, seg[0], 1000, 100);
  sprintf(seg[0]+1000, "test2 string");

  rvm_commit_trans(trans);

  return 0;
}
