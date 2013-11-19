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

  /* currently we can have only 5 segments -- we should dynammically allocate */
  /* in the future or use a linked list */
  rvm->segments = calloc(5, sizeof(segment_t));

  /* we don't have any segments now */
  rvm->no_of_segments = 0;

  return *rvm;
}

void *rvm_map(rvm_t rvm, const char *seg_name, int size_to_create){
  segment_t *segment = malloc(sizeof(segment_t));

  /* create a string with segment name */
  const size_t segment_len = snprintf(NULL, 0, "%s", seg_name) + 1;
  segment->name = malloc(segment_len);
  snprintf(segment->name, segment_len, "%s", seg_name);

  segment->size = size_to_create;

  segment->client_ptr = (void *)malloc(segment->size);
  segment->log_ptr = (void *)malloc(segment->size);

  rvm.segments[rvm.no_of_segments] = segment;
  rvm.no_of_segments += 1;

  return segment->client_ptr;
}
