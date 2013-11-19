#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

rvm_t rvm_init(const char *directory){
  rvm_t *rvm = malloc(sizeof(rvm_t));

  /* create a string with directory name */
  const size_t directory_len = snprintf(NULL, 0, "%s", directory) + 1;
  rvm->directory = malloc(directory_len);
  snprintf(rvm->directory, directory_len, "%s", directory);

  rvm->segments = calloc(5, sizeof(segment_t));
  return *rvm;
}

segment_t rvm_map(rvm_t rvm, const char *seg_name, int size_to_create){
  segment_t *segment = malloc(sizeof(segment_t));

  /* create a string with segment name */
  const size_t segment_len = snprintf(NULL, 0, "%s", seg_name) + 1;
  segment->name = malloc(segment_len);
  snprintf(segment->name, segment_len, "%s", seg_name);

  segment->size = size_to_create;

  return *segment;
}
