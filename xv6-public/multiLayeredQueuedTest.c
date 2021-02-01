#include "types.h"
#include "stat.h"
#include "user.h"

int main() {

    setPolicy(4);
    for (int k = 0; k < 40; k++) {
        int fpid = fork();
        if (fpid == 0) {
            if (k < 10)
                setQueuePriority(0);
            else if (k < 20)
                setQueuePriority(1);
            else if (k < 30) {
                setQueuePriority(2);
                if (k<23)
                    setPriority(1);
                if (k>27)
                    setPriority(5);
            } else {
                setQueuePriority(3);
                if (k<33)
                    setPriority(1);
                if (k>37)
                    setPriority(5);
            }

            for (int i = 0; i < 200; i++) {
//                for (int j = 0; j <10000 ; ++j) {
//                    getpid();
//                }
                printf(1, "%d\n", getpid());
            }
            sleep(1000);
            exit();
        }
    }
    int totalTurnaround = 0;
    int totalWaiting = 0;
    int totalBurst = 0;

    for (int i = 0; i < 40; ++i) {
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