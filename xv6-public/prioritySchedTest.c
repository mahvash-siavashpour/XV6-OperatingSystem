#include "types.h"
#include "stat.h"
#include "user.h"

int main(){

    setPolicy(2);
    for(int k=0;k<30;k++){
        int fpid = fork();
        if( fpid == 0){
            if(k<5)
                setPriority(6);
            else if(k<10)
                setPriority(5);
            else if(k>24)
                setPriority(1);
            for(int i=0;i<250;i++){
//                for (int j = 0; j <10000 ; ++j) {
//                    getpid();
//                }
                printf(1,"%d\n",getpid());
            }
            sleep(1000);
            exit();
        }
    }
    int totalTurnaround[7] ;
    int totalWaiting[7] ;
    int totalBurst[7];


    for (int i = 0; i <30 ; ++i) {
        int fpid = wait();
        int turnAround = getPTimes(2, fpid) - getPTimes(1, fpid);
        int waitingTime = getPTimes(4, fpid);
        int sleeping = getPTimes(5, fpid);
        int cpuBurst = getPTimes(3, fpid);
        totalTurnaround[0] += turnAround;
        totalWaiting[0] += waitingTime;
        totalBurst[0] += cpuBurst;

        totalTurnaround[getPriority(fpid)] += turnAround;
        totalWaiting[getPriority(fpid)] += waitingTime;
        totalBurst[getPriority(fpid)] += cpuBurst;
        printf(1, "PID: %d | Turnaround Time: %d | Waiting Time: %d"
                  " | CPU Burst Time: %d | Sleeping Time: %d\n", fpid, turnAround, waitingTime, cpuBurst, sleeping);
    }

    for (int i = 0; i <7 ; ++i) {
        totalTurnaround[i] /= 30;
        totalWaiting[i] /= 30;
        totalBurst[i] /= 30;
    }
    printf(1, "AVR: turnaround: %d waiting: %d burst: %d \n", totalTurnaround[0], totalWaiting[0], totalBurst[0]);
    for (int i=1;i<7;i++){
        printf(1, "AVR priority: %d => turnaround: %d | waiting: %d | burst: %d \n",
               i, totalTurnaround[i], totalWaiting[i], totalBurst[i]);
    }

    exit();
}