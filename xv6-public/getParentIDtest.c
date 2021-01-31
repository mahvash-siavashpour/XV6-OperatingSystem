#include "types.h"
#include "stat.h"
#include "user.h"


int main(){
    // for(int i =0; i<5;i++){
    //     if (fork() == 0){
        printf(1,"this is process %d and my parent ID is %d\n",getpid(),getParentID());
    //     }
    // }
    return 0;
}