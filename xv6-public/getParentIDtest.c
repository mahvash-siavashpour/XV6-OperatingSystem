#include "types.h"
#include "stat.h"
#include "user.h"


int main(){
    int ParentID = getpid();
    printf(1,"this is parent process and my id is %d \n",ParentID);
    if( fork() == 0){
        int childID = getpid();
        printf(1,"this is process %d and my parent ID is %d\n",childID,getParentID());
    }
    wait();

    exit();
}