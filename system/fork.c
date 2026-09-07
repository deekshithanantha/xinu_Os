/* fork.c - fork */

#include <xinu.h>

local pid32 newforkpid(void);

/*------------------------------------------------------------------------
 * fork - create a child that resumes immediately after the fork call
 *------------------------------------------------------------------------
 */

pid32 fork(void)
{
	//Define variables start here 
	intmask mask;

	pid32 parentspid;
	pid32 childspid;

	struct procent *parent_ptr;
	struct procent *child_ptr;

	uint32 *parent_base;
	uint32 *child_base;

	uint32 *parent_low;
	uint32 *child_low;
	int32 delta;

	uint32 *fork_ebp;
	uint32 *child_fork_ebp;

	uint32 saved_ebx;
	uint32 saved_esi;
	uint32 saved_edi;

	uint32 parent_caller_ebp;
	uint32 child_caller_ebp;

	uint32 return_address;

	uint32 data_to_move;
	uint32 value;

	uint32 *parent_space;
	uint32 *child_space;
	int32 transfer_pointer;

	int32		i;

	uint32 *child_ctxsw_frame;

	//Define variables END here 
	
	
	asm volatile("movl %%ebp, %0" : "=r"(fork_ebp));
	asm volatile("movl %%ebx, %0" : "=r"(saved_ebx));
	asm volatile("movl %%esi, %0" : "=r"(saved_esi));
	asm volatile("movl %%edi, %0" : "=r"(saved_edi));


	mask = disable();

	// 1. finding the parent

	parentspid = (pid32)getpid();
	parent_ptr = &proctab[parentspid];

	// 2. create a new pid for the fork child

	childspid = newforkpid();

	if(childspid == SYSERR) {
		restore(mask);
		return SYSERR;
	}

	child_ptr = &proctab[childspid];


	// 3. allocate a stack similar of the parent 


	child_base = (uint32 *)getstk(parent_ptr->prstklen);

	if(child_base == (uint32 *)SYSERR) {
		restore(mask);
		return SYSERR;
	}

	parent_base = (uint32 *)parent_ptr->prstkbase;


	// 4. calculate the reallocation amount to be moved

	parent_low = (uint32 *)((uint32)parent_base - parent_ptr->prstklen + sizeof(uint32));
	child_low = (uint32 *)((uint32)child_base - parent_ptr->prstklen + sizeof(uint32));

	delta = (int32)((uint32)child_base - (uint32)parent_base);

	// 5. get the callers EBP and return address

	if(((uint32)fork_ebp < (uint32)parent_low) || ((uint32)fork_ebp > (uint32)parent_base)) {

		freestk((char *)child_base, parent_ptr->prstklen);
		restore(mask);
		return SYSERR;
	}

	parent_caller_ebp = *fork_ebp;

	return_address = *(fork_ebp + 1);

	if((parent_caller_ebp < (uint32)parent_low) || (parent_caller_ebp > (uint32)parent_base)) {

		freestk((char *)child_base, parent_ptr->prstklen);
		restore(mask);
		return SYSERR;
	}


	// 6. Copy stack and relocate addresses

	data_to_move = parent_ptr->prstklen / sizeof(uint32);

	parent_space = parent_low;
	child_space  = child_low;

	for(transfer_pointer = 0; transfer_pointer < data_to_move; transfer_pointer++) {

		value = parent_space[transfer_pointer];

		if ((value >= (uint32)parent_low) && (value <= (uint32)parent_base)) {
			//if it points to a value insise the satack displace it by delta
			child_space[transfer_pointer] = value + delta;
		}
		else {
			child_space[transfer_pointer] = value;
		}
	}

	// 7. Initilize child PCB

	/* Initialize process table entry for new fork process
	 */


	child_ptr->prstate = PR_SUSP;
	child_ptr->prprio = parent_ptr->prprio;
	child_ptr->prstkbase = (char *)child_base;
	child_ptr->prstklen = parent_ptr->prstklen;

	child_ptr->prname[PNMLEN-1] = NULLCH;


	for(i = 0; i < PNMLEN-1; i++) {
		child_ptr->prname[i] = parent_ptr->prname[i];

		if(parent_ptr->prname[i] ==  NULLCH) {
			break;
		}
	}


	child_ptr->prsem = -1;
	child_ptr->prparent = parentspid;
	child_ptr->prhasmsg = FALSE;
	child_ptr->user_process = parent_ptr->user_process;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	child_ptr->prdesc[0] = CONSOLE;
	child_ptr->prdesc[1] = CONSOLE;
	child_ptr->prdesc[2] = CONSOLE;

	// 8. find the childs stack EBP

	child_fork_ebp = (uint32 *)((uint32)fork_ebp + delta);

	child_caller_ebp = parent_caller_ebp + delta;

	// 9. Construct Childs ctxsw frame

	child_ctxsw_frame = child_fork_ebp - 9;

	if((uint32)child_ctxsw_frame < (uint32)child_low) {

		freestk((char *)child_base,
			parent_ptr->prstklen);

		restore(mask);
		return SYSERR;
	}


	child_ctxsw_frame[0] = saved_edi;          /* %edi */
	child_ctxsw_frame[1] = saved_esi;          /* %esi */
    child_ctxsw_frame[2] = child_caller_ebp;   /* %ebp (while finishing ctxsw)	*/
    child_ctxsw_frame[3] = 0;                  /* %esp; value filled in below	*/
	child_ctxsw_frame[4] = saved_ebx;          /* %ebx */
	child_ctxsw_frame[5] = 0;                  /* %edx */
	child_ctxsw_frame[6] = 0;                  /* %ecx */
	child_ctxsw_frame[7] = NPROC;              /* %eax */
	child_ctxsw_frame[8] = (uint32)mask;       /* EFLAGS */
	child_ctxsw_frame[9] = child_caller_ebp;
	child_ctxsw_frame[10] = return_address;

	child_ptr->prstkptr = (char *)child_ctxsw_frame;

	// 10. process is ready
	prcount++;
	child_ptr->prstate = PR_READY;
	insert(childspid, readylist, child_ptr->prprio);

	// 11. return back to parent

	restore(mask);


	return childspid;
}

/*------------------------------------------------------------------------
 * newforkpid - Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local pid32 newforkpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}
	return (pid32) SYSERR;
}
