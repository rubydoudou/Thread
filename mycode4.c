/* mycode4.c: UMIX thread package
 *
 *   	Below are functions that comprise the UMIX user-level thread package. 
 * 	These functions are called by user programs that use threads.  You may
 *  	modify the bodies of these functions (and add code outside of them)
 *  	in any way you wish (however, you cannot change their interfaces).  
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mycode4.h"

static int MyInitThreadCalled = 0;	// 1 if MyInitThreads called, else 0
static int head;  // head of queue
static int tail;  // tail of queue
static int lastCreateThread;  
static int currThread;

static struct thread {			// thread table
	int valid;			// 1 if entry is valid, else 0
	jmp_buf env;		// current context
  jmp_buf envInit; // the env saved that is initialized in MyInitThread()

  int prev;         // the previous thread in the queue, -1 if not in queue
  int next;         // the next thread in the queue, -1 if not in queue
  void (*func)();   
  int param;        
  int schedFlag;    // 1 if yielded due to a call to MySchedThread, else 0
} thread[MAXTHREADS];

#define STACKSIZE	65536		// maximum size of thread stack

/* 	MyInitThreads() initializes the thread package. Must be the first
 * 	function called by any user program that uses the thread package. 
 */

void MyInitThreads()
{
	int i;

	if (MyInitThreadsCalled) {		// run only once
		Printf("MyInitThreads: should be called only once\n");
		Exit();
	}

	for (i = 0; i < MAXTHREADS; i++) {	// initialize thread table
		thread[i].valid = 0;
    thread[i].prev = -1;
    thread[i].next = -1;	
    thread[i].schedFlag = 0;
    thread[i].func = 0;
    thread[i].param = 0;

	  char stack[i * STACKSIZE];	// reserve space for thread i's stack
	  if (((int) &stack[STACKSIZE-1]) - ((int) &stack[0]) + 1 != STACKSIZE) {
			Printf("Stack space reservation failed\n");
			Exit();
		}

    if (setjmp(thread[i].env) == 0) {	// save context of thread i
			longjmp(thread[0].env, 1);	// back to thread 0
		}

		/* here when thread 1 is scheduled for the first time */

		(*func)(param);			// execute func(param)

		MyExitThread();			// thread 1 is done - exit
	}

	}

	thread[0].valid = 1;			// initialize thread 0
  currThread = 0;     

	MyInitThreadsCalled = 1;

}

/* 	MyCreateThread(f, p) creates a new thread to execute f(p),
 *   	where f is a function with no return value and p is an
 * 	integer parameter. The new thread does not begin executing
 *  	until another thread yields to it. 
 */

int MyCreateThread(void (*f)(), int p)
	// f: function to be executed
	// p: integer parameter
{
	if (! MyInitThreadsCalled) {
		Printf("MyCreateThread: Must call MyInitThreads first\n");
		Exit();
	}

	if (setjmp(thread[0].env) == 0) {	// save context of thread 0

		char stack[STACKSIZE];	// reserve space for thread 0's stack
		void (*func)() = f;	// func saves f on top of stack
		int param = p;		// param saves p on top of stack

		if (((int) &stack[STACKSIZE-1]) - ((int) &stack[0]) + 1 != STACKSIZE) {
			Printf("Stack space reservation failed\n");
			Exit();
		}

		if (setjmp(thread[1].env) == 0) {	// save context of 1
			longjmp(thread[0].env, 1);	// back to thread 0
		}

		/* here when thread 1 is scheduled for the first time */

		(*func)(param);			// execute func(param)

		MyExitThread();			// thread 1 is done - exit
	}

	thread[1].valid = 1;	// mark the entry for the new thread valid

	return(1);		// done, return new thread ID
}

/*  	MyYieldThread(t) causes the running thread, call it T, to yield to
 * 	thread t.  Returns the ID of the thread that yielded to the calling
 * 	thread T, or -1 if t is an invalid ID.  Example: given two threads
 * 	with IDs 1 and 2, if thread 1 calls MyYieldThread(2), then thread 2
 *   	will resume, and if thread 2 then calls MyYieldThread(1), thread 1
 * 	will resume by returning from its call to MyYieldThread(2), which
 *  	will return the value 2.
 */

int MyYieldThread(int t)
	// t: thread being yielded to
{
	if (! MyInitThreadsCalled) {
		Printf("MyYieldThread: Must call MyInitThreads first\n");
		Exit();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf("MyYieldThread: %d is not a valid thread ID\n", t);
		return(-1);
	}
	if (! thread[t].valid) {
		Printf("MyYieldThread: Thread %d does not exist\n", t);
		return(-1);
	}

        if (setjmp(thread[1-t].env) == 0) {
                longjmp(thread[t].env, 1);
        }
}

/*  	MyGetThread() returns ID of currently running thread. 
 */

int MyGetThread()
{
	if (! MyInitThreadsCalled) {
		Printf("MyGetThread: Must call MyInitThreads first\n");
		Exit();
	}
  return currThread;
}

/* 	MySchedThread() causes the running thread to simply give up the
 * 	CPU and allow another thread to be scheduled. Selecting which
 * 	thread to run is determined here. Note that the same thread may
 *   	be chosen (as will be the case if there are no other threads). 
 */

void MySchedThread()
{
	if (! MyInitThreadsCalled) {
		Printf("MySchedThread: Must call MyInitThreads first\n");
		Exit();
	}
}

/* 	MyExitThread() causes the currently running thread to exit.  
 */

void MyExitThread()
{
	if (! MyInitThreadsCalled) {
		Printf("MyExitThread: Must call MyInitThreads first\n");
		Exit();
	}
}
