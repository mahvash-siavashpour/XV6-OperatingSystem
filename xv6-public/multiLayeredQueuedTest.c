#include "types.h"
#include "stat.h"
#include "user.h"

int main() {

    setPolicy(4);
    int size[] = {10, 10, 10, 10, 40};
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

                printf(1, "%d\n", getpid());
            }
            sleep(2000);
            exit();
        }
    }
    int totalTurnaround[5] ;
    int totalWaiting[5] ;
    int totalBurst[5] ;

    for (int i = 0; i < 40; ++i) {
        int fpid = wait();
        int turnAround = getPTimes(2, fpid) - getPTimes(1, fpid);
        int waitingTime = getPTimes(4, fpid);
        int sleeping = getPTimes(5, fpid);
        int cpuBurst = getPTimes(3, fpid);
        totalTurnaround[4] += turnAround;
        totalWaiting[4] += waitingTime;
        totalBurst[4] += cpuBurst;

        totalTurnaround[getQueue(fpid)] += turnAround;
        totalWaiting[getQueue(fpid)] += waitingTime;
        totalBurst[getQueue(fpid)] += cpuBurst;
        printf(1, "PID: %d | Turnaround Time: %d | Waiting Time: %d"
                  " | CPU Burst Time: %d | Sleeping Time: %d\n", fpid, turnAround, waitingTime, cpuBurst, sleeping);
    }
    for (int i = 0; i <5 ; ++i) {
        totalTurnaround[i] /= size[i];
        totalWaiting[i] /= size[i];
        totalBurst[i] /= size[i];
    }

    printf(1, "AVR: turnaround: %d waiting: %d burst: %d \n", totalTurnaround[4], totalWaiting[4], totalBurst[4]);
    for (int i=0;i<4;i++){
        printf(1, "AVR Queue: %d => turnaround: %d | waiting: %d | burst: %d \n",
               i, totalTurnaround[i], totalWaiting[i], totalBurst[i]);
    }
    exit();
}