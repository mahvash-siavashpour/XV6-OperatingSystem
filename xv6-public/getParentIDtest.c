#include <types.h>
#include <stat.h>
#include <user.h>
#include <proc.h>

int main(){
    for(int i =0; i<5;i++){
        fork();
        printf("this is process %d and my parent ID is %d",getpid,getParentID());
    }
}