#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*mkdir()*/
#include <sys/stat.h>
#include <sys/types.h>


/*Segment List: Maintains a list of segment names currently mapped*/
static segment_list_t* segment_list;

static char* seg_file_ext = ".seg";

static trans_t global_tid = 0;


/*initialize the RVM library: creates the base directory*/
rvm_t rvm_init(const char* directory){

  /*Initialize the segment list*/
  segment_list = (segment_list_t*) malloc( sizeof(segment_list_t*) ); 
  segment_list = NULL; 

  /*Create the backing directory for the RVM*/
  rvm_t* rvm = (rvm_t*) malloc( sizeof(rvm_t) );
  rvm->directory = (char*) malloc( strlen(directory) + 1 );

  strcpy(rvm -> directory, directory);

  if(mkdir(rvm->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
    fprintf(stderr,"Failed to create RVM directory\n");
    return *rvm;
  }
  
  return *rvm;

}

/*Delete the segment file if it exists and is not mapped*/
void rvm_destroy(rvm_t rvm, const char* seg_name){

  while(segment_list != NULL){
   /*  Check if the segment is mapped. If yes return*/  
    if(strcmp(seg_name, segment_list->segment.name)){
        fprintf(stderr, "Segment is currently mapped. Failed to delete");
        return;
    }
    segment_list = segment_list->next_seg;    
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

  int seg_it = 0;
  for(seg_it=0; seg_it < num_segs; ++seg_it){
  
      while(segment_list != NULL){

        if(!strcmp(segment_list->segment.name, seg_bases[seg_it])){

          if(segment_list->txn != -1) return (trans_t)-1;
        }
      }
  }

  trans_t tid = global_tid++;

  seg_it = 0;
  for(seg_it=0; seg_it<num_segs; ++seg_it){
  
      while(segment_list != NULL){

        if(!strcmp(segment_list->segment.name, seg_bases[seg_it]))
          segment_list->txn = tid;
        
      }
  }

  return tid;

}

void rvm_about_to_modify(trans_t tid, void* seg_base, int offset, int size){}

void rvm_commit_trans(trans_t tid){}

int file_exist (char *filename)
{
  struct stat buffer;   
  return (stat(filename, &buffer) == 0);
}
