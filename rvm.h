#ifndef _RVM_H_
#define _RVM_H_
  
typedef struct {
  char*     name;
  int       size;
  void*     data;
} segment_t;

typedef struct {
  char*     directory;
} rvm_t;

typedef struct{
  int       tid;
  segment_t seg;
} trans_t;

rvm_t rvm_init(const char* directory);

void* rvm_map(rvm_t rvm, const char* seg_name, int size_to_create);

void rvm_unmap(rvm_t rvm, void* seg_base);

void rvm_destroy(rvm_t rvm, const char* seg_name);

trans_t rvm_begin_trans(rvm_t rvm, int num_segs, void** seg_bases);

void rvm_about_to_modify(trans_t tid, void* seg_base, int offset, int size);

void rvm_commit_trans(trans_t tid);

void rvm_commit_trans_heavy(trans_t tid);

void rvm_abort_trans(trans_t tid);

void rvm_truncate_log(rvm_t rvm);

#endif
