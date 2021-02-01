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
            for(int i=0;i<100;i++){
//                for (int j = 0; j <10000 ; ++j) {
//                    getpid();
//                }
                printf(1,"%d\n",getpid());
            }
            sleep(1000);
            exit();
        }
    }
    int totalTurnaround = 0;
    int totalWaiting = 0;
    int totalBurst = 0;

    for (int i = 0; i <30 ; ++i) {
        int fpid = wait();
        int turnAround = getPTimes(2, fpid) - getPTimes(1, fpid);
        int waitingTime = getPTimes(4, fpid);
        int sleeping = getPTimes(5, fpid);
        int cpuBurst = getPTimes(3, fpid);
        totalTurnaround += turnAround;
        totalWaiting += waitingTime;
        totalBurst += cpuBurst;
        printf(1, "PID: %d | Turnaround Time: %d | Waiting Time: %d"
                  " | CPU Burst Time: %d | Sleeping Time: %d\n", fpid, turnAround, waitingTime, cpuBurst, sleeping);
    }

    totalTurnaround /= 30;
    totalWaiting /= 30;
    totalBurst /= 30;
    printf(1, "AVR: turnaround: %d waiting: %d burst: %d \n", totalTurnaround, totalWaiting, totalBurst);

    exit();
}