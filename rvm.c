#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

rvm_t rvm_init(const char *directory) {
  rvm_t *rvm = malloc(sizeof(rvm_t));

  /* create a string with directory name */
  const size_t directory_len = snprintf(NULL, 0,
                                       "%s", directory) + 1;
  rvm->directory = malloc(directory_len);
  snprintf(rvm->directory, directory_len, "%s", directory);

  return *rvm;
}
