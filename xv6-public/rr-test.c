#include "types.h"
#include "stat.h"
#include "user.h"

int main(){


    int fpid = fork();
    if( fpid == 0){
        int childID = getpid();
        for(int i=0;i<10;i++){
            printf(1,"%d ", i);
        }
        printf(1,"this is process %d and my running time is %d\n",childID,getPTimes(5, childID));
        exit();
    }
    wait();
    printf(1,"this is process parent and my running time is %d\n",getPTimes(3, -1));
    exit();
}