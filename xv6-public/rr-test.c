#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
    int childID[10];
    for(int k=0;k<10;k++){
        int fpid = fork();
        if(fpid > 0)
            childID[k] = fpid;

        if( fpid == 0){

            for(int i=0;i<100;i++){
                printf(1,"PID: %d i: %d\n",childID[k], i);
            }
            exit();
        }
    }

//    int count = 0;
//    while (count<10){
//        for(int i=0;i<10;i++){
//            if(processState(childID[i]) == 0) {
//                int turnAround = getPTimes(2, childID[i]) - getPTimes(1, childID[i]);
//                int waitingTime = getPTimes(4, childID[i]);
//                int cpuBurst = getPTimes(3, childID[i]);
//                printf(1, "PID: %d has Turnaround Time: %d  and  Waiting Time: %d"
//                          "  and  CPU Burst Time: %d\n ", childID[i], turnAround, waitingTime, cpuBurst);
//                count++;
//            }
//        }
//    }



    for(int i=0; i< 10; i++){
        wait();
    }


    printf(1,"PID: %d Parent Running Time: %d\n ",getpid(), getPTimes(2, -1)-getPTimes(1, -1));

    exit();
}