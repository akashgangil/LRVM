#ifndef _RVM_H_
#define _RVM_H_

typedef struct {
  char *directory;
  void *ptrs[4];
} rvm_t;

rvm_t rvm_init(const char *directory);

#endif
