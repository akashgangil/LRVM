#include<stdio.h>
#include<unistd.h>
int main(void) {

    FILE *log_file;
    FILE *daemon_log_file;
    long int sz;
    while (1) {
    
        log_file = fopen("rvm.log", "r");
        fseek(log_file, 0L, SEEK_END);
        sz = ftell(log_file);
        printf("Size: %ld\n", sz); fflush(stdout);
        if(sz > 5000000)
            system("./truncate");            
        sleep(2); 
    }
    return 0;
}
