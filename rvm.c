#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 0
#define FALSE 1

#define NO_OF_RVMS 3
#define NO_OF_SEGMENTS 5

rvm_list_t *rvm_list;
int rvm_list_initialised = FALSE;

void rvm_list_init(){
  /* allocate memory for 1 rvm_list*/
  rvm_list = calloc(1, sizeof(rvm_list_t));

  /* allocate memeory for 3 rvms */
  rvm_list->rvms = calloc(NO_OF_RVMS, sizeof(rvm_t));

  rvm_list->no_of_rvms = 0;

  rvm_list_initialised = TRUE;
}

rvm_t *find_rvm(rvm_t rvm) {
  rvm_t *local_rvm;

  /* find our rvm from the list of rvms */
  for(int i = 0; i < rvm_list->no_of_rvms; i++) {
    if (strcmp(rvm.directory, rvm_list->rvms[i]->directory) == 0) {
      local_rvm = rvm_list->rvms[i];
    }
  }

  return local_rvm;
}

rvm_t rvm_init(const char *directory){
  /* initialise list of rvms */
  if (rvm_list_initialised == FALSE){
    rvm_list_init();
  }

  /* allocate memory for new rvm */
  rvm_t *rvm = malloc(sizeof(rvm_t));

  /* create a string with directory name */
  const size_t directory_len = snprintf(NULL, 0, "%s", directory) + 1;
  rvm->directory = malloc(directory_len);
  snprintf(rvm->directory, directory_len, "%s", directory);

  /* currently we can have only 5 segments -- we should dynammically allocate */
  /* in the future or use a linked list */
  rvm->segments = calloc(NO_OF_SEGMENTS, sizeof(segment_t));

  /* we don't have any segments now */
  rvm->no_of_segments = 0;

  /* add this rvm to our list of rvms */
  rvm_list->rvms[rvm_list->no_of_rvms] = rvm;
  rvm_list->no_of_rvms += 1;

  return *rvm;
}

void *rvm_map(rvm_t rvm, const char *seg_name, int size_to_create){
  segment_t *segment = malloc(sizeof(segment_t));

  /* find our rvm from list of rvms */
  rvm_t *local_rvm = find_rvm(rvm);

  /* iterate through all segments in the rvm */
  for (int i = 0; i < local_rvm->no_of_segments; i++) {
    /* find the segment we need */
    if(strcmp(local_rvm->segments[i]->name, seg_name) == 0) {
      /* check if we need increase memory */
      if (local_rvm->segments[i]->size < size_to_create) {
        local_rvm->segments[i]->size = size_to_create;

        local_rvm->segments[i]->client_ptr = realloc(local_rvm->segments[i]->client_ptr,
                                                     size_to_create);
        local_rvm->segments[i]->log_ptr = realloc(local_rvm->segments[i]->log_ptr,
                                                  size_to_create);

        return local_rvm->segments[i]->client_ptr;
      }
    }
  }

  /* create a string with segment name */
  const size_t segment_len = snprintf(NULL, 0, "%s", seg_name) + 1;
  segment->name = malloc(segment_len);
  snprintf(segment->name, segment_len, "%s", seg_name);

  segment->size = size_to_create;

  /* this is the memory we're going to give to the client */
  segment->client_ptr = (void *)malloc(segment->size);

  /* this is the duplicate memory which we're going to log to */
  segment->log_ptr = (void *)malloc(segment->size);

  local_rvm->segments[rvm.no_of_segments] = segment;
  local_rvm->no_of_segments += 1;
  return segment->client_ptr;
}


/* Since rvm->segments is a list, this is going to mess up when you delete segments. */
/* You'll have to reorder it once you finish, or maybe use a linked list? */
void rvm_unmap(rvm_t rvm, void *seg_base) {
  rvm_t *local_rvm = find_rvm(rvm);

  /* search for our segment by comparing client_ptr with seg_base */
  for(int i = 0; i < local_rvm->no_of_segments; i++) {
    if(seg_base == local_rvm->segments[i]->client_ptr){
      free(local_rvm->segments[i]->name);
      free(local_rvm->segments[i]->client_ptr);
      free(local_rvm->segments[i]->log_ptr);
      free(local_rvm->segments[i]);

      local_rvm->no_of_segments--;

      return;
    }
  }
}
