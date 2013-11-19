#ifndef _RVM_H_
#define _RVM_H_

typedef struct {
  char *name;
  int size;
} segment_t;

typedef struct {
  char *directory;
  void *ptrs[4];
  segment_t *segments;
  int no_of_segments;
} rvm_t;

/* create an rvm instance and return it */
rvm_t rvm_init(const char *directory);

/* create or extend a extend and return pointer to allocated memory */
segment_t rvm_map(rvm_t rvm, const char *seg_name, int size_to_create);

#endif
