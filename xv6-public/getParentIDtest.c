#include "types.h"
#include "stat.h"
#include "user.h"


int main(){
    int number_of_children = 5;
    int parentID = getpid();
    printf(1,"this is parent process and my id is %d \n",parentID);
    for(int i = 0; i<number_of_children; i++){
        if(getpid() == parentID ) {
            if(0 == fork()){
                printf(1,"this is child %d and my parent is %d\n",getpid(),getParentID());
                exit();
            }
            sleep(100);
        }
    }    
    
    for(int i=0; i< number_of_children; i++){
        wait();
    }

    exit();
}