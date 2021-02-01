#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "syscall.h"

struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;
int policy;
int nextpid = 1;

extern void forkret(void);

extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void) {
    initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
    return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void) {
    int apicid, i;

    if (readeflags() & FL_IF)
        panic("mycpu called with interrupts enabled\n");

    apicid = lapicid();
    // APIC IDs are not guaranteed to be contiguous. Maybe we should have
    // a reverse map, or reserve a register to store &cpus[i].
    for (i = 0; i < ncpu; ++i) {
        if (cpus[i].apicid == apicid)
            return &cpus[i];
    }
    panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void) {
    struct cpu *c;
    struct proc *p;
    pushcli();
    c = mycpu();
    p = c->proc;
    popcli();
    return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void) {
    struct proc *p;
    char *sp;

    acquire(&ptable.lock);

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

    found:
    p->state = EMBRYO;
    p->pid = nextpid++;
    p->creationTime = ticks;
    p->runningTime = 0;
    p->sleepingTime = 0;
    p->readyTime = 0;
    p->priority = 3;
    p->groupPriority = 0;
    p->queuePriority = 3;
    release(&ptable.lock);

    // Allocate kernel stack.
    if ((p->kstack = kalloc()) == 0) {
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe *) sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint *) sp = (uint) trapret;

    sp -= sizeof *p->context;
    p->context = (struct context *) sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint) forkret;

    return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void) {
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    initproc = p;
    if ((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int) _binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;

    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n) {
    uint sz;
    struct proc *curproc = myproc();

    sz = curproc->sz;
    if (n > 0) {
        if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if (n < 0) {
        if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    curproc->sz = sz;
    switchuvm(curproc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void) {
    int i, pid;
    struct proc *np;
    struct proc *curproc = myproc();
    curproc->sysCallCounter[SYS_fork]++;
    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process state from proc.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;
    release(&ptable.lock);

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void) {
    struct proc *curproc = myproc();
    struct proc *p;
    int fd;
    curproc->sysCallCounter[SYS_exit]++;
    if (curproc == initproc)
        panic("init exiting");

    // Close all open files.
    for (fd = 0; fd < NOFILE; fd++) {
        if (curproc->ofile[fd]) {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->parent == curproc) {
            p->parent = initproc;
            if (p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }

    // Jump into the scheduler, never to return.
    curproc->state = ZOMBIE;
    curproc->terminationTime = ticks;
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void) {
    struct proc *p;
    int havekids, pid;
    struct proc *curproc = myproc();
    curproc->sysCallCounter[SYS_wait]++;

    acquire(&ptable.lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->parent != curproc)
                continue;
            havekids = 1;
            if (p->state == ZOMBIE) {
                // Found one.
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
            //    p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || curproc->killed) {
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &ptable.lock);  //DOC: wait-sleep
    }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for (;;) {
        // Enable interrupts on this processor.
        sti();
        struct proc *defaultProcess;
        // Loop over process table looking for process to run.
        acquire(&ptable.lock);

        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            //for policy 0, 1 (round robin)
            if (p->state != RUNNABLE)
                continue;

            // for policy 3 and 2(priority selection)
            if (policy > 1 && policy < 4) {
                defaultProcess = p;
                // choose one with highest priority
                struct proc *p1;
                for (p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++) {
                    if (p1->state != RUNNABLE)
                        continue;
                    if (policy == 2) {
                        if (defaultProcess->priority > p1->priority)
                            defaultProcess = p1;
                        else if (defaultProcess->priority == p1->priority) {
                            if (defaultProcess->groupPriority > p1->groupPriority) {
                                defaultProcess = p1;
                            }
                        }
                    } else {
                        if (defaultProcess->priority < p1->priority)
                            defaultProcess = p1;
                        else if (defaultProcess->priority == p1->priority) {
                            if (defaultProcess->groupPriority > p1->groupPriority) {
                                defaultProcess = p1;
                            }

                        }

                    }
                }
                defaultProcess->groupPriority++;
                p = defaultProcess;
            } else if (policy == 4) {

                struct proc *p1;
                defaultProcess = p;
                for (p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++) {
                    if (p1->state != RUNNABLE)
                        continue;
                    // check queue priority of 2 processes
                    if (defaultProcess->queuePriority > p1->queuePriority) {
                        defaultProcess = p1;
                    }
                        //if in the same queue pick from their scheduling algorithm
                    else if (defaultProcess->queuePriority == p1->queuePriority) {
                        // default or customize round robin
                        if (defaultProcess->queuePriority < 2) {
                            if (defaultProcess->groupPriority > p1->groupPriority) {
                                defaultProcess = p1;
                            }
                        }
                            //priority
                        else if (defaultProcess->queuePriority == 2) {
                            if (defaultProcess->priority > p1->priority)
                                defaultProcess = p1;
                            else if (defaultProcess->priority == p1->priority) {
                                if (defaultProcess->groupPriority > p1->groupPriority) {
                                    defaultProcess = p1;
                                }
                            }
                        }
                            // reverse priority
                        else if (defaultProcess->queuePriority == 3) {
                            if (defaultProcess->priority < p1->priority)
                                defaultProcess = p1;
                            else if (defaultProcess->priority == p1->priority) {
                                if (defaultProcess->groupPriority > p1->groupPriority) {
                                    defaultProcess = p1;
                                }

                            }
                        }

                    }
                }

                defaultProcess->groupPriority++;
                p = defaultProcess;
            }

            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
        }

        release(&ptable.lock);

    }
}

// Enter  .  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void) {
    int intena;
    struct proc *p = myproc();

    if (!holding(&ptable.lock))
        panic("sched ptable.lock");
    if (mycpu()->ncli != 1)
        panic("sched locks");
    if (p->state == RUNNING)
        panic("sched running");
    if (readeflags() & FL_IF)
        panic("sched interruptible");
    intena = mycpu()->intena;
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    myproc()->state = RUNNABLE;
    sched();
    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void) {
    static int first = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();
    p->sysCallCounter[SYS_sleep]++;

    if (p == 0)
        panic("sleep");

    if (lk == 0)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if (lk != &ptable.lock) {  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }
    // Go to sleep.
    p->chan = chan;
    p->state = SLEEPING;

    sched();

    // Tidy up.
    p->chan = 0;

    // Reacquire original lock.
    if (lk != &ptable.lock) {  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan) {
    struct proc *p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == SLEEPING && p->chan == chan)
            p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan) {
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid) {
    struct proc *p;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {
            p->sysCallCounter[SYS_kill]++;
            p->killed = 1;
            // Wake process from sleep if necessary.
            if (p->state == SLEEPING)
                p->state = RUNNABLE;
            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void) {
    static char *states[] = {
            [UNUSED]    "unused",
            [EMBRYO]    "embryo",
            [SLEEPING]  "sleep ",
            [RUNNABLE]  "runble",
            [RUNNING]   "run   ",
            [ZOMBIE]    "zombie"
    };
    int i;
    struct proc *p;
    char *state;
    uint pc[10];
    myproc()->sysCallCounter[SYS_dup]++;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if (p->state == SLEEPING) {
            getcallerpcs((uint *) p->context->ebp + 2, pc);
            for (i = 0; i < 10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}

int
getParentID() {
    struct proc *p = myproc();
    p->sysCallCounter[SYS_getParentID]++;
    int parentID = p->parent->pid;
    return parentID;
}


int
getChildren(void *ch_list, int curpid) {
    struct proc *p;
    char *children_list = (char *) ch_list;
    int cnt = 0;
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {

        if (p->parent->pid == curpid) {
                int childID = p->pid;
                if(childID >9)
                {
                    children_list[cnt] = (char) ((childID/10) + 48);
                    cnt++;
                    childID %=10;
                }
                children_list[cnt] = (char) (childID + 48);
                cnt++;
                children_list[cnt] = '*';
                cnt++;
        }
    }
    cnt--;
    children_list[cnt] = 'f';
    release(&ptable.lock);
    return 1;

}

int
getSyscallCounter(int pid, int syscallNum) {
    struct proc *p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {

            // cprintf("In the next line you can see how many times a syscall has been called:\n");
            // for (int i = 1; i <= 29; i++)
            // {
            //   cprintf("syscall(%d) --> %d time[s]\n",i,p->sysCallCounter[i]);
            // }
            // cprintf("syscall(%d) --> %d time[s]\n",syscallNum,p->sysCallCounter[syscallNum]);
            return p->sysCallCounter[syscallNum];
        }
    }
    return -1;
}

int
setPolicy(int newPLC) {

    if (newPLC <= 4 && newPLC >= 0) {
        policy = newPLC;
        return 1;
    }
    return 0;
}

void
updateProcTimes() {
    struct proc *p;
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
//        if(p->pid > 2)
//        cprintf("TICKS: %d\n",ticks);
        if (p->state == SLEEPING) {
            p->sleepingTime += 1;
//            if(p->pid > 2)
//            cprintf("pid: %d >>>>> SLEEPING: %d\n",p->pid, p->sleepingTime);
        } else if (p->state == RUNNING) {
            p->runningTime += 1;
//            if(p->pid > 2)
//            cprintf("pid: %d >>>>> RUNNING: %d\n",p->pid, p->runningTime);
        } else if (p->state == RUNNABLE) {
            p->readyTime += 1;
//            if(p->pid > 2)
//            cprintf("pid: %d >>>>> READY: %d\n",p->pid, p->readyTime);
        }
    }
    release(&ptable.lock);
}

int
getPTimes(int type, int pid) {
    struct proc *currp = myproc();
    if (pid != -1) {
        acquire(&ptable.lock);
        struct proc *p;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->pid == pid) {
                currp = p;
                break;
            }
        }
        release(&ptable.lock);
    }
    //type:
    // 1  creationTime;
    // 2  terminationTime;
    // 3  runningTime;
    // 4  readyTime;
    // 5  sleepingTime;
    switch (type) {
        case 1:
//            cprintf("pid: %d => creation: %d\n",currp->pid, currp->creationTime);
            return currp->creationTime;
        case 2:
//            cprintf("pid: %d => termination: %d\n",currp->pid, currp->terminationTime);
//            if (currp->terminationTime == 0)
//                currp -> terminationTime = ticks;
            return currp->terminationTime;
        case 3:
//            cprintf("pid: %d => running: %d\n",currp->pid, currp->runningTime);
            return currp->runningTime;
        case 4:
            return currp->readyTime;
        case 5:
            return currp->sleepingTime;
    }
    return 0;
}

int
setPriority(int priority) {
    struct proc *currp = myproc();
    acquire(&ptable.lock);
    if (priority >= 1 && priority <= 6)
        currp->priority = priority;
    else
        currp->priority = 5;
    release(&ptable.lock);
    return currp->pid;
}

int
setQueuePriority(int queuePriority) {
    struct proc *currp = myproc();
    acquire(&ptable.lock);
    if (queuePriority >= 0 && queuePriority <= 3)
        currp->queuePriority = queuePriority;
    release(&ptable.lock);
    return currp->pid;
}