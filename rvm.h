#ifndef _RVM_H_
#define _RVM_H_
  
typedef struct {
  char*     name;
  int       size;
  void*     data;
} segment_t;

typedef struct {
  char*     directory;
  char*     logfile;
} rvm_t;

typedef int trans_t;

typedef struct offset_node {
  int                 offset_val;
  int                 size;
  struct offset_node* next_offset;
} offset_t;

typedef struct seg_node{
  segment_t*       segment;
  trans_t          txn;
  offset_t*        offset;
  struct seg_node* next_seg;  
} segment_list_t;


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

int file_exist(char* filename);

void check_segment_list(void);

#endif
