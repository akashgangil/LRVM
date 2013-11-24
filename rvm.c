#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*mkdir()*/
#include <sys/stat.h>
#include <sys/types.h>


/*Segment List: Maintains a list of segment names currently mapped*/
static segment_list_t* segment_list;

/*Transaction List: Maintains a transaction -> segment name mapping*/

/*initialize the RVM library: creates the base directory*/
rvm_t rvm_init(const char* directory){

  /*Initialize the segment list*/
  segment_list = (segment_node_t*) malloc( sizeof(segment_node_t*) ); 
  segment_list = NULL; 

  /*Create the backing directory for the RVM*/
  rvm_t* rvm = (rvm_t*) malloc( sizeof(rvm_t) );
  rvm->directory = (char*) malloc( strlen(directory) + 1 );

  strcpy(rvm -> directory, directory);

  if(mkdir(rvm->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
    perror("Couldnt create RVM directory");
    exit(1);
  }
  
  return *rvm;
}

/*Delete the segment file if it exists and is not mapped*/
void destroy(rvm_t rvm, const char* seg_name){

  while(segment_list != NULL){
   /*  Check if the segment is mapped. If yes return*/  
    if(strcmp(seg_name, segment_list->segment.name)){
        perror("Segment is currently mapped. Unable to delete");
        return;
    }
    segment_list = segment_list->next_seg;    
  }
      
  /*Segment isnt mapped, delete the .seg file*/
  char *ext = ".seg"; 
  char *seg_path = (char*) malloc( strlen(rvm.directory) +strlen(seg_name) + strlen(ext) +1);
  strcpy(seg_path, rvm.directory);
  strcat(seg_path, ext);

  if(rmdir(seg_path)){
    perror("Error deleting segment");
  }
  free(seg_path);
}

/*Map the segment from disk to memory, if it doesnt exist on disk create it*/
void* rvm_map(rvm_t rvm, const char* seg_name, int size_to_create){

}

