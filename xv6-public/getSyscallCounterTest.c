#include "types.h"
#include "stat.h"
#include "user.h"

 
int main(int argc, char *argv[]){
    // char input[2];
    int input = 0;
    for (int i = 0; i<2 ; i++){
        input *=10;
        input += ((int)argv[1][i] -48);
    }
    if(input < 0){
        printf(1,"you forgot to give me arg try agaun \n");
        exit();
    }
    for(int  i =0; i<5;i++){
        getpid();
    }
    for(int  i =0; i<7;i++){
        getParentID();
    }
    int x =getSyscallCounter(getpid(),input);
    printf(1,"syscall %d has been called for %d time[s]\n",input,x);
 

exit();
}