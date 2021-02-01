#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "syscall.h"
int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{ myproc()->sysCallCounter[SYS_getpid]++;
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  myproc()->sysCallCounter[SYS_sbrk]++;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    myproc()->sysCallCounter[SYS_sleep]++;
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  myproc()->sysCallCounter[SYS_uptime]++;
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


int 
sys_getParentID(void)
{
  return getParentID();
}


int 
sys_getChildren(void)
{  
  int curpid ;
  char* children_list;
  int size =  sizeof(char) * 140;
  if(((argptr(0,(void*)&children_list,size) < 0) || argint(1,&curpid) < 0)  ){
    return -1;
  }
  myproc()->sysCallCounter[SYS_getChildren]++;
  // cprintf("pid of father is %d\n",curpid);
  // void* ch = (void*)children_list;
  getChildren(children_list,curpid);
  // children_list = (char*)ch;
  // for(int i =0;i<30;i++){
  //   cprintf("%d",children_list[i]);
  // }
  return 1;
 }
int 
sys_getCount(void){
  int sysCallnum ; 
  int pid;
  if(argint(0,&pid) < 0 || (argint(1,&sysCallnum) <0)){
    return -1;
  }
  return getCount(myproc()->pid,sysCallnum);
}

// change policy of scheduler
int
sys_setPolicy(void){
    int plc;
    if(argint(0, &plc) < 0)
        return -1;

    myproc()->sysCallCounter[SYS_setPolicy]++;
    return setPolicy(plc);
}
int
sys_getPTimes(void){
    int pTimeType;
    int pid;
    if(argint(0, &pTimeType) < 0 || argint(1, &pid) < 0)
        return -1;
    myproc()->sysCallCounter[SYS_getPTimes]++;
    return getPTimes(pTimeType, pid);
}

int
sys_processState(void){
    int pid;
    if(argint(0, &pid) < 0)
        return -1;
    return processState(pid);
}

