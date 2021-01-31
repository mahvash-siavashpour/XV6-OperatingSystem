#include "types.h"
#include "stat.h"
#include "user.h"

int main(){

    for(int i=0;i<10;i++){
        int pid = fork();
        if(pid == 0){
//            for(int j=0;j<1000;j++){
////                printf(1, "%d : %d\n", getpid, j);
//            }
            return 0;
        }else
        {
            continue;
        }
        
    }
    printf(1, "%d\n", getCreationTime());
    return 0;
}