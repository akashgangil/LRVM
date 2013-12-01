/* abort.c - test that aborting a modification returns the segment to
 * its initial state */

#include "../rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "bleg!"
#define OFFSET2 1000


int main(int argc, char **argv)
{
     rvm_t rvm;
     char *seg;
     char *segs[16];
     trans_t trans;
     
     rvm = rvm_init("rvm_segments");
     
     rvm_destroy(rvm, "testseg");

     int i;

     for(i=0; i<16; ++i){
        char* seg_name = (char*)malloc(10*sizeof(char));
        sprintf(seg_name, "testseg%d",i+1);
        printf("Segname: %s\n", seg_name);
        segs[i] = (char *) rvm_map(rvm, seg_name, 16000000);
     }
          
     trans = rvm_begin_trans(rvm, 16, (void**) segs);
     int offset = 0;
     
     for(i=0; i<100; ++i){
        seg = segs[rand() % 16];
     
        /* write some data and commit it */
        offset = rand() % 16000000;
        rvm_about_to_modify(trans, seg, offset, 32);
        sprintf(seg+offset, TEST_STRING1);
     }

     
     rvm_commit_trans(trans);
     for(i=0; i<16; ++i){
        free(segs[i]);
     }

     return 0;
}

