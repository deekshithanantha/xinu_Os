/*  main_kill.c  - main_kill */

#include <xinu.h>
#include <stdarg.h>

#define STACK_SIZE 1024
#define PR0C_PRIOR 30

// Spawn at least 15 user processes using the create system call. 
//  * createchildren - construct the fixed 15-node user process tree
/*
 * Test tree:
 *
 *                 0
 *          /      |      \
 *         1       2       3
 *       /  \    / | \      \
 *      4    5  6  7  8      9
 *     / \          |       / \
 *   10  11        12      13  14
 */


pid32 p0, p1, p2, p3, p4, p5, p6, p7;
pid32 p8, p9, p10, p11, p12, p13, p14;

process proc0(void);
process proc1(void);
process proc2(void);
process proc3(void);
process proc4(void);
process proc7(void);
process proc9(void);
process leaf(void);

void print_user_tree(char *title);



process main (void) {

    p0 = create(proc0, STACK_SIZE, PR0C_PRIOR, "P0",0);
    proctab[p0].user_process = TRUE;
    resume(p0);

    sleep(3);

    print_user_tree("[INITIAL TREE]");

    kprintf("\n[KILL LEAF]\n");
    kprintf("Killing logical node 11, PID = %d\n", p11);

    kill(p11);

    print_user_tree("[AFTER KILLING LEAF]");

////////////////////////////////////////////////////////
        
    kprintf("\n[KILL INTERNAL PARENT]\n");
    kprintf("Killing logical node 2, PID = %d\n", p2);

    kill(p2);

    print_user_tree("[AFTER KILLING INTERNAL PARENT]");

///////////////////////////////////////////////////////////////

    kprintf("\n[KILL ROOT]\n");
    kprintf("Killing logical node 0, PID = %d\n", p0);

    kill(p0);

    print_user_tree("[AFTER KILLING ROOT]");


    return OK;

}

process proc0(void) {

    p1 = create(proc1, STACK_SIZE, PR0C_PRIOR, "P1", 0);
    proctab[p1].user_process = TRUE;
    
    p2 = create(proc2, STACK_SIZE, PR0C_PRIOR, "P2", 0);
    proctab[p2].user_process = TRUE;
    
    p3 = create(proc3, STACK_SIZE, PR0C_PRIOR, "P3", 0);
    proctab[p3].user_process = TRUE;
    
    
    
    resume(p1);
    resume(p2);
    resume(p3);

    while (TRUE) {
        receive();
    }

    return OK;
    
}


process proc1(void) {

    p4 = create(proc4, STACK_SIZE, PR0C_PRIOR, "P4", 0);
    proctab[p4].user_process = TRUE;
    
    p5 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P5", 0);
    proctab[p5].user_process = TRUE;
    
    
    resume(p4);
    resume(p5);

    while (TRUE) {
        receive();
    }

    return OK;
}


process proc2(void)
{
    p6 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P6", 0);
    proctab[p6].user_process = TRUE;

    p7 = create(proc7, STACK_SIZE, PR0C_PRIOR, "P7", 0);
    proctab[p7].user_process = TRUE;

    p8 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P8", 0);
    proctab[p8].user_process = TRUE;

    resume(p6);
    resume(p7);
    resume(p8);

    while (TRUE) {
        receive();
    }
}


process proc3(void)
{
    p9 = create(proc9, STACK_SIZE, PR0C_PRIOR, "P9", 0);
    proctab[p9].user_process = TRUE;

    resume(p9);

    while (TRUE) {
        receive();
    }

    return OK;
}


process proc4(void)
{
    p10 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P10", 0);
    proctab[p10].user_process = TRUE;

    p11 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P11", 0);
    proctab[p11].user_process = TRUE;

    resume(p10);
    resume(p11);

    while (TRUE) {
        receive();
    }

    return OK;
}


process proc7(void)
{
    p12 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P12", 0);
    proctab[p12].user_process = TRUE;

    resume(p12);

    while (TRUE) {
        receive();
    }

    return OK;
}


process proc9(void)
{
    p13 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P13", 0);
    proctab[p13].user_process = TRUE;

    p14 = create(leaf, STACK_SIZE, PR0C_PRIOR, "P14", 0);
    proctab[p14].user_process = TRUE;

    resume(p13);
    resume(p14);

    while (TRUE) {
        receive();
    }

    return OK;
}

process leaf(void)
{
    while (TRUE) {
        receive();
    }

    return OK;
}



void print_user_tree(char *title) {
    kprintf("\n%s\n", title);;

    pid32 pid;
    pid32 child;
    bool8 child_found;


    for (pid = 0; pid< NPROC; pid++) {
        
        if(proctab[pid].prstate == PR_FREE) {
            continue;
        }
        
        if(proctab[pid].user_process == FALSE) {
            continue;
        }

        kprintf("Process : %d",pid);

        child_found = FALSE;

        for (child = 0; child < NPROC; child++) {
            
            if(proctab[child].prstate == PR_FREE) {
                continue;
            }
            
            if(proctab[child].user_process == FALSE) {
                continue;
            }
            
            if (proctab[child].prparent == pid) {
                kprintf(" %d", child);
                child_found = TRUE;
            }
        }

    if (child_found == FALSE) {
        kprintf(" <none>");
    }

    kprintf("\n");
        
    }
}


