#include "rvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*mkdir()*/
#include <sys/stat.h>
#include <sys/types.h>


/*initialize the RVM library: creates the base directory*/
rvm_t rvm_init(const char* directory){

  rvm_t* rvm = (rvm_t*) malloc( sizeof(rvm_t) );
  rvm->directory = (char*) malloc( strlen(directory) + 1 );

  strcpy(rvm -> directory, directory);

  if(mkdir(rvm->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
    perror("Couldnt create RVM directory");
    exit(1);
  }
  
  return *rvm;
}
