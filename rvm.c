#include "rvm.h"

#include <stdlib.h>
#include <string.h>

/*mkdir()*/
#include <sys/stat.h>
#include <sys/types.h>

/*LINE_MAX*/
#include <limits.h>

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
static char* LOG_FILE = "rvm.log";
static char* UNDO_FILE = "rvm_undo.log";
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
    char* seg_path = get_seg_file_path(seg_name);

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

    printf("created\n");

    /*Make a segment node*/
    segment_list_t* new_seg = (segment_list_t*)malloc(sizeof(segment_list_t));
    new_seg->segment  = seg;
    new_seg->next_seg = NULL;
    new_seg->txn      = -1;
    new_seg->offset   = NULL;

    printf("created2\n");

    /*Add the segment to the list of segments*/
    segment_list_t* seg_node = segment_list;
    if(seg_node == NULL)
        segment_list = new_seg;
    else{
        while(seg_node -> next_seg != NULL){
            seg_node = seg_node->next_seg ;
        }
        seg_node -> next_seg = new_seg;
    }

    printf("added\n");

    restore_seg_from_log(seg_name, seg);

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
        printf("IT: %d\n", seg_it);
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

        offset_t* base_offset = segment_node->offset;

        while(base_offset != NULL) {
            printf("Offset:  %d\n", base_offset->offset_val);
            printf("SIZE: %d\n", base_offset->size);
            printf("Data:    %s\n", (char*)segment_node->segment->data + base_offset->offset_val);
            base_offset = base_offset -> next_offset;
        }

        segment_node = segment_node -> next_seg;
    }
}

void check_backup_segment_list(){

    printf("check_segment_list called\n");
    segment_list_t* segment_node = backup_segment_list;

    while(segment_node != NULL){
        fprintf(stderr, "TXN Id : %d\n",segment_node->txn);
        fprintf(stderr,"Data:    %s\n", (char*)segment_node->segment->data);

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

    segment_list_t* seg_node = segment_list;
    while(seg_node != NULL){
        printf("Seg Node : %d\n", seg_node->txn);
        if(seg_node->txn == tid){
            write_seg_to_file(seg_node, log_file);
        }
        seg_node = seg_node ->next_seg;
    }

    remove_seg_from_transaction(tid);
    printf("Log file closed\n");
    fflush(log_file);
    //    fclose(log_file);
}

void rvm_abort_trans(trans_t tid) {
    printf("Abort and copy back\n");
    
    check_segment_list();

    segment_list_t* seg_node = segment_list;

    /* delete nodes from segment list */
    while(seg_node !=NULL) {
        if (seg_node->txn == tid) {
            restore_seg_from_log(seg_node->segment->name, seg_node->segment);        
        }
        seg_node = seg_node -> next_seg;
    }

    printf("Abort txn\n");
    check_segment_list();
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


void rvm_commit_trans_heavy(trans_t tid){

    segment_list_t* seg_node = segment_list;
    FILE* seg_file;
    FILE* undo_file;

    undo_file = fopen(UNDO_FILE, "a");

    while(seg_node != NULL){
        if(seg_node -> txn == tid ){

            char* seg_file_path = get_seg_file_path(seg_node->segment->name);

            fprintf(stdout, "Writing to the segment file %s\n", seg_file_path);
            seg_file = fopen(seg_file_path, "a");
            write_seg_to_file(seg_node, seg_file);
            write_seg_to_file(seg_node, undo_file);
        }
        seg_node = seg_node -> next_seg;
    }
    fclose(undo_file);
    remove_seg_from_transaction(tid);
}

void rvm_truncate_log(rvm_t rvm){

    FILE* log_file;
    FILE* seg_file;

    log_file = fopen(LOG_FILE, "r");

    char *ch1, *token1;
    char line[LINE_MAX];
    int token_count1;

    if(log_file != NULL){
        while(fgets(line, LINE_MAX, log_file) != NULL){

            char *data = (char*)malloc(strlen(line)  + 1);
            strcpy(data, line);

            printf("LINE: %s\n", line);

            token_count1 = 0;
            token1 = strtok_r(line, ":", &ch1);

            while(token1 != NULL){
                token_count1++;
                if(token_count1 == 2){
                    seg_file = fopen(get_seg_file_path(token1), "a");
                    break;
                }
                token1 = strtok_r(NULL, ":", &ch1);
            }

            printf("LINE: %s\n", data);

            fwrite(data, strlen(data), 1, seg_file);
            fclose(seg_file);
            free(data);
        }
    }

    //erase all the contents of the log file
    log_file = fopen(LOG_FILE, "w");
    fclose(log_file);
}   

void rvm_unmap(rvm_t rvm, void* seg_base){
}

int file_exist (char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

char* get_seg_file_path(const char* seg_name){

    char *seg_path = (char*) malloc( strlen(rvm->directory)
            + strlen(seg_name) + strlen(seg_file_ext)+1);
    strcpy(seg_path, rvm->directory);
    strcat(seg_path, "/");
    strcat(seg_path, seg_name);
    strcat(seg_path, seg_file_ext);

    return seg_path;
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

    printf("Writing the following data to the seg file %s\n", data_to_write);

    if(fwrite(data_to_write, strlen(data_to_write), 1, file) >= 0) 
        return 1;
    else
        return -1;
}

void restore_seg_from_log(char* seg_name, segment_t* seg){
    /*Check if the segment is in the log file*/
    FILE* log_file;
    if(file_exist(LOG_FILE)) {

        printf("FILE EXISTS\n");
        printf("%s\n", LOG_FILE);

        log_file = fopen("rvm.log", "r");

        if(log_file == NULL) {
            fprintf(stderr, "Failed to open segment file %s", LOG_FILE);
            return ;
        }
   
        printf("Printing...\n");
        int c;
        if (log_file) {
            while ((c = getc(log_file)) != EOF)
                putchar(c);
        }
        printf("End...\n");

        char *ch1, *ch2, *token1, *token2;
        char line[LINE_MAX];
        int token_count1, token_count2, pos;
        
        fseek(log_file, 0, SEEK_END);
        printf("Current : %d\n", ftell(log_file));

        fseek(log_file, 0, SEEK_SET);
        printf("Current : %d\n", ftell(log_file));
        
        fseek(log_file, 4, SEEK_CUR);
        printf("Current : %d\n", ftell(log_file));

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
            printf("I didnt get a line!\n");
        }

     //   fclose(log_file);
    }

}

/*Should be called in the commit functions as the segment is no 
 longer involved in the transaction
 */
void remove_seg_from_transaction(trans_t tid){
    segment_list_t* seg_node = segment_list;
    while(seg_node != NULL){
        if(seg_node->txn == tid){
            seg_node->txn = -1;
        }
        seg_node = seg_node -> next_seg;
    }
}
