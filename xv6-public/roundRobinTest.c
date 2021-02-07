#include "types.h"
#include "stat.h"
#include "user.h"

int main() {

    setPolicy(1);
    for (int i = 0; i <6 ; i++) {
        int fpid= fork();
        if ( fpid== 0) {

            for (int i = 0; i < 1000; i++) {
//                for (int j = 0; j <10000 ; ++j) {
//                    getpid();
//                }
                printf(1, " %d \n", getpid());
            }
            sleep(300);
            exit();
        }
    }

    int totalTurnaround = 0;
    int totalWaiting = 0;
    int totalBurst = 0;

    for (int i = 0; i <6 ; i++) {
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

    totalTurnaround /= 6;
    totalWaiting /= 6;
    totalBurst /= 6;
    printf(1, "AVR: turnaround: %d waiting: %d burst: %d \n", totalTurnaround, totalWaiting, totalBurst);
    exit();
}