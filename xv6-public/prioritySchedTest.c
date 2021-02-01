#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
//    int childID[30];
    setPolicy(2);
    for(int k=0;k<30;k++){
        int fpid = fork();
        if(fpid > 0) {
//            childID[k] = fpid;
//            printf(1, "CHILD NUMBER: %d\n", childID[k]);
        }

        if( fpid == 0){
            if(k<5)
                setPriority(6);
            else if(k<10)
                setPriority(5);
            else if(k>24)
                setPriority(1);
            for(int i=0;i<100;i++){
                printf(1,"CHILD NUMBER: %d\n",k);
            }
            exit();
        }
    }



    for(int i=0; i< 30; i++){
        wait();
    }


    printf(1,"PID: %d Parent\n ",getpid());

    exit();
}