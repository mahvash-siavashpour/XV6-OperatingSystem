#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
    int childID[10];
    for(int k=0;k<10;k++){
        int fpid = fork();
        childID[k] = fpid;

        if( fpid == 0){

            for(int i=0;i<100;i++){
                printf(1,"PID: %d i: %d\n",childID, i);
            }
            exit();
        }
    }




    for(int i=0; i< 10; i++){
        wait();
    }

    for(int i=0;i<10;i++){
        printf(1,"PID: %d running time: %d\n ",childID[i], getPTimes(3, childID[i]));
    }

    printf(1,"PID: %d running time: %d\n ",getpid(), getPTimes(3, -1));

    exit();
}