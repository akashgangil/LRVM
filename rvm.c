#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*mkdir()*/
#include <sys/stat.h>
#include <sys/types.h>

/*LINE_MAX*/
#include <limits.h>

#define LOG_FILE "rvm.log"
#define COMMIT "COMMIT"
#define CHECKPOINT "CHECKPOINT"
#define SEPERATOR ":"

/*Segment List: Maintains a list of segment names currently mapped*/
static segment_list_t* segment_list;

/* maintains a backup of segment_list */
static segment_list_t* backup_segment_list;

static char* seg_file_ext = ".seg";
static char* log_file_ext = ".log";
static trans_t global_tid = 0;

static rvm_t* rvm;


/*initialize the RVM library: creates the base directory*/
rvm_t rvm_init(const char* directory) {

    /*Initialize the segment list*/
    segment_list = (segment_list_t*) malloc( sizeof(segment_list_t*) );
    segment_list = NULL;

    /*Initialize the backup segment list*/
    backup_segment_list = (segment_list_t*) malloc( sizeof(segment_list_t*) );
    backup_segment_list = NULL;

    /*Create the backing directory for the RVM*/
    rvm = (rvm_t*) malloc( sizeof(rvm_t) );
    rvm->directory = (char*) malloc( strlen(directory) + 1 );
    strcpy(rvm -> directory, directory);

    /*Name the log file*/
    rvm->logfile = (char*) malloc(strlen(directory) + strlen(log_file_ext) + 1);
    strcpy(rvm->logfile, directory);
    strcat(rvm->logfile, log_file_ext);

    if(!file_exist(rvm->directory)){
        if(mkdir(rvm->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
            fprintf(stderr,"Failed to create RVM directory\n");
            return *rvm;
        }
    }

    return *rvm;
}

/*Delete the segment file if it exists and is not mapped*/
void rvm_destroy(rvm_t rvm, const char* seg_name){
    /*TODO: don't we have to delete the segment from memory also? */

    segment_list_t* seg_node = segment_list;
    while(seg_node != NULL){
        /*  Check if the segment is mapped. If yes return*/
        if(strcmp(seg_name, seg_node->segment->name)){
            fprintf(stderr, "Segment is currently mapped. Failed to delete");
            return;
        }
        seg_node = seg_node->next_seg;
    }

    /*Segment isnt mapped, delete the .seg file*/
    char *seg_path = (char*) malloc( strlen(rvm.directory)
            + strlen(seg_name) + strlen(seg_file_ext)+1);
    strcpy(seg_path, rvm.directory);
    strcat(seg_path, "/");
    strcat(seg_path, seg_name);
    strcat(seg_path, seg_file_ext);

    if(file_exist(seg_path)){
        if(rmdir(seg_path)){
            fprintf(stderr, "Error deleting segment\n");
        }
    }

    free(seg_path);
}

/*Map the segment from disk to memory, if it doesnt exist on disk create it*/
void* rvm_map(rvm_t rvm, const char* seg_name, int size_to_create){

    fprintf(stdout, "rvm_map called\n");

    segment_t* seg = (segment_t*) calloc(1, sizeof(segment_t));

    seg->name = (char*) malloc(sizeof( strlen(seg_name) +  1));
    strcpy(seg->name, seg_name);

    seg->data = (void*)malloc(sizeof(char) * size_to_create);

    seg->size = size_to_create;

    /*Make a segment node*/
    segment_list_t* new_seg = (segment_list_t*)malloc(sizeof(segment_list_t));
    new_seg->segment  = seg;
    new_seg->next_seg = NULL;
    new_seg->txn      = -1;
    new_seg->offset   = NULL;

    /*Add the segment to the list of segments*/
    segment_list_t* seg_node = segment_list;
    if(seg_node == NULL)
        segment_list = new_seg;
    else{
        while(seg_node -> next_seg != NULL);
        seg_node->next_seg = new_seg;
    }

    /*
     * LOGIC to read from the backing file should go here
     *
        FILE* seg_file;
        char* seg_file_path = (char*)malloc(  strlen(rvm.directory)
        + strlen(seg_name) + strlen(seg_file_ext)+1);

        printf("HERE1\n");
        strcpy(seg_file_path, rvm.directory);
        strcat(seg_file_path, "/");
        strcat(seg_file_path, seg_name);
        strcat(seg_file_path, seg_file_ext);
        printf("HERE2\n");
      */

    /*Check if the segment is in the log file*/
    FILE* log_file;
    if(file_exist(LOG_FILE)) {

        printf("FILE EXISTS\n");

        log_file = fopen(LOG_FILE, "r");

        if(log_file == NULL) {
            fprintf(stderr, "Failed to open segment file %s", LOG_FILE);
            return ;
        }

        char *ch1, *ch2, *token1, *token2;
        char line[LINE_MAX];
        int token_count1, token_count2, pos;

        if(log_file != NULL){
            while(fgets(line, LINE_MAX, log_file) != NULL){
                token_count1 = 0;
                token1 = strtok_r(line, ":", &ch1);
                while(token1 != NULL){
                    token_count1++;
                    if(token_count1 == 2)
                        if(strcmp(token1, seg_name)) break;
                    if(token_count1 > 2){
                        token_count2 = 0; pos = 0;
                        char *token2 = strtok_r(token1, "-", &ch2);
                        while(token2 != NULL){
                            token_count2++;
                            if(token_count2 == 1) pos = atoi(token2);
                            if(token_count2 == 3) {
                                strcpy((char*)seg->data + pos, token2);
                                printf("PUT token2  \n%s \n[%s]\n", token2,  (char*)seg->data+pos);
                            }
                            token2 = strtok_r(NULL, "-", &ch2);
                        }
                    }
                    token1 = strtok_r(NULL, ":", &ch1);
                }
            }
        }

        fclose(log_file);
    }

    printf("Returning\n");
    return seg->data;
}


trans_t rvm_begin_trans(rvm_t rvm, int num_segs, void** seg_bases){

    segment_list_t* seg_node = segment_list;
    int seg_it = 0;

    printf("Beginning Transaction\n");

    /* Check if someone else is doing a transaction on same segment */
    for(seg_it=0; seg_it < num_segs; ++seg_it){
        while(seg_node!= NULL){
            if(seg_node->segment->data == seg_bases[seg_it])
                if(seg_node->txn != -1) return (trans_t)-1;
            seg_node = seg_node -> next_seg;
        }
    }

    trans_t tid = global_tid++;

    /* set transaction ids for the segments that are to be modified */
    seg_it = 0;
    for(seg_it=0; seg_it<num_segs; ++seg_it){
        seg_node = segment_list;
        while(seg_node != NULL){
            if(seg_node->segment->data == seg_bases[seg_it]) {
                seg_node->txn = tid;
                printf("Set tid\n");
            }
            seg_node = seg_node -> next_seg;
        }
    }

    printf("TID: %d\n", tid);
    return tid;
}

void check_segment_list(){

    printf("check_segment_list called\n");
    segment_list_t* segment_node = segment_list;

    while(segment_node != NULL){
        printf("A NODE!!!!\n");
        printf("TXN Id : %d\n",segment_node->txn);
        printf("Name   : %s\n", segment_node->segment->name);
        printf("SIZE:    %d\n", segment_node->segment->size);
        printf("Data:    %s\n", (char*)segment_node->segment->data);

        offset_t* base_offset = segment_node->offset;

        while(base_offset != NULL) {
            printf("Offset:  %d\n", base_offset->offset_val);
            printf("SIZE: %d\n", base_offset->size);
            base_offset = base_offset -> next_offset;
        }

        segment_node = segment_node -> next_seg;
    }
}

void rvm_commit_trans(trans_t tid){

    check_segment_list();

    FILE* log_file;
    log_file = fopen(LOG_FILE, "a");

    if(log_file == NULL)
        fprintf(stderr, "Failed to open the log file");

    int seg_it = 0;

    printf("writing to the log file for transaction id: %d\n", tid);

    if(segment_list == NULL) {
        printf("Still NULL :( \n");
    }

    segment_list_t* seg_node = segment_list;
    while(seg_node != NULL){
        printf("Seg Node : %d\n", seg_node->txn);
        if(seg_node->txn == tid){
            write_seg_to_file(seg_node, log_file);
        }
        seg_node = seg_node ->next_seg;
    }

    /* update the backup also */
    seg_node = segment_list;

    while(seg_node != NULL){
        if(seg_node->txn == tid){
            segment_t* seg = seg_node->segment;
            
            /* create new backup segment */
            segment_t* backup_seg = (segment_t*) calloc(1, sizeof(segment_t));
            
            /* copy name  */
            backup_seg->name = (char*) malloc(sizeof( strlen(seg->name) +  1));
            strcpy(backup_seg->name, seg->name);
            
            /* copy data */
            backup_seg->data = (void*)malloc(sizeof(char) * seg->size);
            backup_seg->data = (char*)(seg_node->segment->data);
            
            /* copy size */
            backup_seg->size = seg->size;

            /* copy segments */
            offset_t* seg_offset = seg_node->offset;
            offset_t* prev_offset = NULL;
            offset_t* start_offset = NULL;
            
            while(seg_offset!= NULL) {
                offset_t* new_offset = (offset_t*)malloc(sizeof(offset_t));
                new_offset -> offset_val  = seg_offset->offset_val;
                new_offset -> size        = seg_offset->size;
                new_offset -> next_offset = NULL;

                if(start_offset == NULL) {
                    start_offset = new_offset;
                }

                if(prev_offset != NULL) {
                    prev_offset->next_offset = new_offset;
                }
                
                prev_offset = new_offset;
                seg_offset = seg_offset->next_offset;
            }
          
            /* create new backup segment node */
            segment_list_t* new_backup_seg = (segment_list_t*)malloc(sizeof(segment_list_t));
            new_backup_seg->segment  = backup_seg;
            new_backup_seg->next_seg = NULL;
            new_backup_seg->txn      = tid;
            new_backup_seg->offset   = start_offset;
          
            /*Add the segment to the list of segments*/
            segment_list_t* backup_seg_node = backup_segment_list;
            if(backup_seg_node == NULL)
                backup_segment_list = new_backup_seg;
            else{
                while(backup_seg_node -> next_seg != NULL);
                backup_seg_node->next_seg = new_backup_seg;
            }
            
        }
        seg_node = seg_node ->next_seg;
    }

    printf("Log file closed\n");
    int f = fclose(log_file);
}

void rvm_abort_trans(trans_t tid) {
    fprintf(stderr, "Aborting transaction!\n");

    segment_list_t* seg_node = segment_list;
    segment_list_t* prev = NULL;
    segment_list_t* curr = NULL;

    /* delete nodes from segment list */
    while(seg_node !=NULL) {
        if (seg_node->txn == tid) {
            if (prev == NULL) {
                segment_list = seg_node->next_seg;
            }
            else {
                prev->next_seg = seg_node->next_seg;
            }
            curr = seg_node;
            seg_node = seg_node->next_seg;
            free(seg_node);
        }
    }

    /* copy backed up nodes to segment list */
    segment_list_t* backup_seg_node = backup_segment_list;
    while(backup_seg_node != NULL) {
        if(backup_seg_node->txn == tid){
            segment_t* seg = backup_seg_node->segment;
            
            /* create new segment */
            segment_t* new_seg = (segment_t*) calloc(1, sizeof(segment_t));
            
            /* copy name  */
            new_seg->name = (char*) malloc(sizeof( strlen(seg->name) +  1));
            strcpy(new_seg->name, seg->name);
            
            /* copy data */
            new_seg->data = (void*)malloc(sizeof(char) * seg->size);
            new_seg->data = (char*)(seg_node->segment->data);
            
            /* copy size */
            new_seg->size = seg->size;

            /* copy segments */
            offset_t* seg_offset = seg_node->offset;
            offset_t* prev_offset = NULL;
            offset_t* start_offset = NULL;
            
            while(seg_offset!= NULL) {
                offset_t* new_offset = (offset_t*)malloc(sizeof(offset_t));
                new_offset -> offset_val  = seg_offset->offset_val;
                new_offset -> size        = seg_offset->size;
                new_offset -> next_offset = NULL;

                if(start_offset == NULL) {
                    start_offset = new_offset;
                }

                if(prev_offset != NULL) {
                    prev_offset->next_offset = new_offset;
                }
                
                prev_offset = new_offset;
                seg_offset = seg_offset->next_offset;
            }
          
            /* create new backup segment node */
            segment_list_t* new_seg_node = (segment_list_t*)malloc(sizeof(segment_list_t));
            new_seg_node->segment  = new_seg;
            new_seg_node->next_seg = NULL;
            new_seg_node->txn      = tid;
            new_seg_node->offset   = start_offset;
          
            /*Add the segment to the list of segments*/
            segment_list_t* seg_node = segment_list;
            if(seg_node == NULL)
                segment_list = new_seg_node;
            else{
                while(seg_node -> next_seg != NULL);
                seg_node->next_seg = new_seg_node;
            }
        }
        
        backup_seg_node = backup_seg_node->next_seg;
    }
}

void rvm_about_to_modify(trans_t tid, void* seg_base, int offset, int size){

    printf("About to modify %d  %d\n", offset , size);

    segment_list_t* seg_node = segment_list;

    while(seg_node != NULL){
        if(seg_node->txn == tid && seg_node->segment->data == seg_base){

            offset_t* new_offset = (offset_t*)malloc(sizeof(offset_t));
            new_offset -> offset_val = offset;
            new_offset -> size       = size;
            new_offset -> next_offset = NULL;

            offset_t* seg_offset = seg_node->offset;

            if(seg_offset == NULL){
                seg_node->offset = new_offset;
            }
            else{
                while(seg_offset -> next_offset != NULL)
                    seg_offset = seg_offset->next_offset;
                seg_offset->next_offset = new_offset;
            }

        }
        seg_node = seg_node -> next_seg;
    }
}

/*void rvm_commit_trans_heavy(trans_t tid){
   
    segment_list_t* seg_node = segment_list;
    FILE* seg_file;
     
    while(seg_node != NULL){
      if(seg_node -> txn == tid ){
        
        char* seg_file_path = (char*)malloc(strlen(rvm -> directory)
                                + strlen(seg_node->segment->name) + strlen(seg_file_ext)+1);

        strcpy(seg_file_path, rvm.directory);
        strcat(seg_file_path, "/");
        strcat(seg_file_path, seg_name);
        strcat(seg_file_path, seg_file_ext);

        fprintf(stdout, "Writing to the segment file %s", seg_file_path);

        
      }
    }
}*/

int file_exist (char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

int write_seg_to_file(segment_list_t* seg_node, FILE* file){
    int data_size = strlen(seg_node->segment->name)
                    + strlen(seg_node->segment->data)
                    + 1 /*for null char*/
                    + 1 /*for space*/
                    + 1 /*for newline*/
                    + strlen(COMMIT)
                    + 2 * strlen(SEPERATOR) + 1 + 1
                    ;

    char* data_to_write = (char*) malloc(sizeof(char)*data_size);
    strcpy(data_to_write, COMMIT);
    strcat(data_to_write, SEPERATOR);
    strcat(data_to_write, seg_node->segment->name); 

    offset_t* base_offset = seg_node -> offset;
    while(base_offset != NULL) {
        strcat(data_to_write, SEPERATOR);
        char off[100];
        sprintf(off, "%d-%d-%s", base_offset->offset_val
                   , base_offset->size
                   , (char*)(seg_node->segment->data)+base_offset->offset_val);
        strcat(data_to_write, off);
        base_offset = base_offset -> next_offset;
    }
    strcat(data_to_write, ":");
    strcat(data_to_write, "\n");
    if(fwrite(data_to_write, strlen(data_to_write), 1, file) >= 0) 
        return 1;
    else
        return -1;
}
