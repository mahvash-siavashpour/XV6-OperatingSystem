#include "types.h"
#include "stat.h"
#include "user.h"

int main() {

    setPolicy(0);
    for (int i = 0; i <10 ; ++i) {
        int fpid= fork();
        if ( fpid== 0) {

            for (int i = 0; i < 100; i++) {
                for (int j = 0; j <10000 ; ++j) {
                    getpid();
                }
//                printf(1, "PID: %d i: %d\n", getpid(), i);
            }
            exit();
        }
    }


    for (int i = 0; i <10 ; ++i) {
        int fpid = wait();
        int turnAround = getPTimes(2, fpid) - getPTimes(1, fpid);
        int waitingTime = getPTimes(4, fpid);
        int sleeping = getPTimes(5, fpid);
        int cpuBurst = getPTimes(3, fpid);
        printf(1, "PID: %d | Turnaround Time: %d | Waiting Time: %d"
                  " | CPU Burst Time: %d | Sleeping Time: %d\n", fpid, turnAround, waitingTime, cpuBurst, sleeping);
    }


//    fpid = getpid();
//    turnAround = getPTimes(2, fpid) - getPTimes(1, fpid);
//    waitingTime = getPTimes(4, fpid);
//    sleeping = getPTimes(5, fpid);
//    cpuBurst = getPTimes(3, fpid);
//    printf(1, "Parent === PID: %d has Turnaround Time: %d \n and  Waiting Time: %d"
//              "\n  and  CPU Burst Time: %d\n and  Sseeping Time: %d\n ", fpid, turnAround, waitingTime, cpuBurst, sleeping);

//    printf(1, "PID: %d Parent Running Time: %d\n ", getpid(), getPTimes(2, -1) - getPTimes(1, -1));

    exit();
}