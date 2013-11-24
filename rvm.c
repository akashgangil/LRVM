#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*mkdir()*/
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_FILE "rvm.log"

/*Segment List: Maintains a list of segment names currently mapped*/
static segment_list_t* segment_list;

static char* seg_file_ext = ".seg";
static char* log_file_ext = ".log";
static trans_t global_tid = 0;

/*initialize the RVM library: creates the base directory*/
rvm_t rvm_init(const char* directory) {

  /*Initialize the segment list*/
  segment_list = (segment_list_t*) malloc( sizeof(segment_list_t*) ); 
  segment_list = NULL; 

  /*Create the backing directory for the RVM*/
  rvm_t* rvm = (rvm_t*) malloc( sizeof(rvm_t) );
  rvm->directory = (char*) malloc( strlen(directory) + 1 );
  strcpy(rvm -> directory, directory);
  
  /*Name the log file*/
  rvm->logfile = (char*) malloc(strlen(directory) + strlen(log_file_ext) + 1);
  strcpy(rvm->logfile, directory);
  strcat(rvm->logfile, log_file_ext);

  if(mkdir(rvm->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
    fprintf(stderr,"Failed to create RVM directory\n");
    return *rvm;
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
  char *seg_path = (char*) malloc( strlen(rvm.directory) 
                                  + strlen(seg_name) + strlen(seg_file_ext)+1);
  strcpy(seg_path, rvm.directory);
  strcat(seg_path, seg_file_ext);
  
  if(rmdir(seg_path)){
    fprintf(stderr, "Error deleting segment\n");
  }
  
  free(seg_path);
}

/*Map the segment from disk to memory, if it doesnt exist on disk create it*/
void* rvm_map(rvm_t rvm, const char* seg_name, int size_to_create){

  segment_t* seg = (segment_t*) calloc(1, sizeof(segment_t));
  
  seg->name = (char*) malloc(sizeof( strlen(seg_name) +  1));
  strcpy(seg->name, seg_name);

  seg->data = (void*)malloc(sizeof(char) * size_to_create);
       
  seg->size = size_to_create;

  /*Make a segment node*/
  segment_list_t* new_seg = (segment_list_t*)malloc(sizeof(segment_list_t*));
  new_seg->segment  = seg;
  new_seg->next_seg = NULL;
  new_seg->txn      = -1;


  /*Add the segment to the list of segments*/
  segment_list_t* seg_node = segment_list;
  if(seg_node == NULL)
      seg_node = new_seg;
  else{
    while(seg_node -> next_seg != NULL);
    seg_node->next_seg = new_seg;
  }


  /*
   * Check if a backing file for that segment exists
   * and if yes, load the data
   */
  FILE* seg_file;
  char* seg_file_path = (char*)malloc(  strlen(rvm.directory) 
                                 + strlen(seg_name) + strlen(seg_file_ext)+1);

  strcpy(seg_file_path, rvm.directory);
  strcat(seg_file_path, "/");
  strcat(seg_file_path, seg_name);
  strcat(seg_file_path, seg_file_ext);

  if(file_exist(seg_file_path)) {

    seg_file = fopen(seg_file_path, "r");
  
    if(seg_file == NULL) {
        fprintf(stderr, "Failed to open segment file %s", seg_file_path);
        return ;
    }  

    fseek(seg_file, 0, SEEK_END);
    long fsize = ftell(seg_file);
    fseek(seg_file, 0, SEEK_SET);

    fread(seg->data, fsize, 1, seg_file);

    fclose(seg_file); 
  }

  return (void*)seg;
}


trans_t rvm_begin_trans(rvm_t rvm, int num_segs, void** seg_bases){

  segment_list_t* seg_node = segment_list;
  int seg_it = 0;
  for(seg_it=0; seg_it < num_segs; ++seg_it){
      while(seg_node!= NULL){
        if(!strcmp(seg_node->segment->name, seg_bases[seg_it]))
          if(seg_node->txn != -1) return (trans_t)-1;
        seg_node = seg_node -> next_seg;
      }
  }

  trans_t tid = global_tid++;
  seg_node = segment_list;

  seg_it = 0;
  for(seg_it=0; seg_it<num_segs; ++seg_it){
      while(seg_node != NULL){
        if(!strcmp(seg_node->segment->name, seg_bases[seg_it]))
          seg_node->txn = tid;
        seg_node = seg_node -> next_seg;
      }
  }

  return tid;
}

void rvm_about_to_modify(trans_t tid, void* seg_base, int offset, int size){

}

void rvm_commit_trans(trans_t tid){
  
  FILE* log_file;
  log_file = fopen(LOG_FILE, "w+");
      
  if(log_file == NULL)
    fprintf(stderr, "Failed to open the log file");
  
  int seg_it = 0;

  printf("writing to the log file for transaction id: %d\n", tid); 
  
  if(segment_list == NULL) printf("Still NULL :( \n");

  segment_list_t* seg_node = segment_list; 
  while(seg_node != NULL){
    printf("Seg Node : %d\n", seg_node->txn);
    if(seg_node->txn == tid){
      
      
      fseek(log_file, 0, SEEK_END);
     
      printf("Segment Name: %s", seg_node->segment->name);
      printf("Segment Data: %s", (char*)seg_node->segment->data);

      char* data_to_write = (char*) malloc(strlen(seg_node->segment->name) 
                                           + strlen((char*)seg_node->segment->data)
                                           + 1 /*for space*/
                                           + 1 /*for null char*/
                                           + 1 /*for newline char*/
                                           );
    
      strcpy(data_to_write, seg_node->segment->name);
      strcat(data_to_write, " ");
      strcat(data_to_write, seg_node->segment->data);
      strcat(data_to_write, "\n");

      fwrite(data_to_write, strlen(data_to_write), 1, log_file);     
      seg_node = seg_node -> next_seg;
       
    }
  }

  printf("Closing the log file\n");
  fclose(log_file);
}

int file_exist (char *filename)
{
  struct stat buffer;   
  return (stat(filename, &buffer) == 0);
}
