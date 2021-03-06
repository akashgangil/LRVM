/* compare_trans.c - Compare elapsed time of commit_trans for different transaction loads */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"

#define TEST_STRING1 "hello, world"


int main(int argc, char **argv)
{
     if (argc != 2){
        fprintf(stderr, "Usage: ./compare_trans <# of transaction loads>\n");
        exit(1);
     }

     int txn_load = atoi(argv[1]);

     rvm_t rvm;
     char *seg;
     char *segs[16];
     trans_t trans;
    
     struct stopwatch_t* sw = stopwatch_create();
     stopwatch_init();

     rvm = rvm_init("rvm_segments");
     
     int i;

     for(i=0; i<16; ++i){
        char* seg_name = (char*)malloc(10*sizeof(char));
        sprintf(seg_name, "testseg%d",i+1);
        //printf("Segname: %s\n", seg_name);
        segs[i] = (char *) rvm_map(rvm, seg_name, 16000000);
     }
          
     trans = rvm_begin_trans(rvm, 16, (void**) segs);
     int offset = 0;
     
     stopwatch_start(sw);
     for(i=0; i<txn_load; ++i){
        seg = segs[rand() % 16];
     
        /* write some data and commit it */
        offset = rand() % 16000000;
        rvm_about_to_modify(trans, seg, offset, 32);
        sprintf(seg+offset, TEST_STRING1);
     }
     
     rvm_commit_trans(trans);
     
     stopwatch_stop(sw);
     printf("%Lg\n", stopwatch_elapsed(sw));
     
     for(i=0; i<16; ++i){
        free(segs[i]);
     }

     stopwatch_destroy(sw);

     return 0;
}

