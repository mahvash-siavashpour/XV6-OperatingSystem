#include "types.h"
#include "stat.h"
#include "user.h"


int main(){
    int number_of_children = 5;
    char mlist[140];
    int parent_id =getpid();
    printf(1,"in test parent_pid %d\n",parent_id);
    for(int i = 0; i<number_of_children; i++){
        if(getpid() == parent_id ) 
            fork();
    }
    if(getpid() == parent_id){
        getChildren((void*)mlist,parent_id);
        for(int i = 0;i<140;i++){
            if(mlist[i] != 'f'){
                printf(1,"%c",mlist[i]);
            }
            else
            {
                break;
            }
            
        }
    }
    for(int i=0; i< number_of_children; i++){
        wait();
    }
    exit();
}