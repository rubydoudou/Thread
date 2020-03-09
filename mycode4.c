/* mycode4.c: UMIX thread package
 *
 *   	Below are functions that comprise the UMIX user-level thread package. 
 * 	These functions are called by user programs that use threads.  You may
 *  	modify the bodies of these functions (and add code outside of them)
 *  	in any way you wish (however, you cannot change their interfaces).  
 */

#include <setjmp.h>
#include <string.h>
#include "aux.h"
#include "umix.h"
#include "mycode4.h"

static int MyInitThreadsCalled = 0;	// 1 if MyInitThreads called, else 0
static int head = -1;  // head of queue
static int tail = -1;  // tail of queue
static int lastCreateThread = 0;  
static int currThread = 0;
static int lastRunThread = 0; 
static int queueSize = 0;

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

/*------- helper functions ----------*/

// enqueue thread t to the end 
static void enqueue(int t) {
  if (head == -1) {
    head = t;
    tail = t;
  } else {
    thread[tail].next = t;
    thread[t].prev = tail;
    tail = t;
  }
    queueSize++;
}
// dequeue the front of the queue, return the dequeued id.
static int dequeue() {
  int returnID = head;
  int second;
  if (head == -1) return -1;
  if (head == tail) {
    head = -1;
    tail = -1;
  } else {
    second = thread[head].next;
    thread[second].prev = -1; 
    thread[head].next = -1;
    head = second;
  }
  queueSize--;
  //DPrintf("after dequeue,head = %d,tail = %d,size = %d\n",head,tail,queueSize);
  return returnID;
}
// remove thread t in the queue
static void removeFromQ(int t) {
  if (head == -1) return;
  int tPrev = thread[t].prev;

  if (head == t) {
    dequeue();
    return;
  } else if (tail == t) {
    thread[tPrev].next = -1;
    thread[t].prev = -1;
    tail = tPrev;
  } else {
    thread[tPrev].next = thread[t].next;
    thread[thread[t].next].prev = tPrev;
    thread[t].next = -1;
    thread[t].prev = -1;
    }
  queueSize--; 
}

static void printQ() {
  if (head == -1) return;
  int ptr = head;
  while (ptr != tail) {
    DPrintf("%d->", ptr);
    ptr = thread[ptr].next;
  }
  DPrintf("%d\n",ptr);
}

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
    thread[i].schedFlag = 0;

	  char stack[i * STACKSIZE];	// reserve space for thread i's stack
	  if (((int) &stack[i * STACKSIZE-1]) - ((int) &stack[0]) + 1 != i*STACKSIZE) {
			Printf("Stack space reservation failed\n");
			Exit();
		}

    if (setjmp(thread[i].env) != 0) {	// save context of thread i
      if (thread[currThread].func) {
        thread[currThread].func(thread[currThread].param);
      }
		  MyExitThread();			// thread i is done - exit
		} else {
        memcpy(thread[i].envInit, thread[i].env, sizeof(jmp_buf)); // copy the initial env
    }
	}

	thread[0].valid = 1;			// thread 0 is the current thread
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

  int newID = (lastCreateThread + 1) % MAXTHREADS;
  int findSpot = 0;

  // find the new thread ID to assign to
  for (int i = 0; i < MAXTHREADS; i++) {
    if (thread[newID].valid == 0) {
      findSpot = 1;
      break;
    }
    newID = (newID + 1) % MAXTHREADS;
  }
  if (findSpot == 0) return -1;

	thread[newID].func = f;	    // func saves f on top of stack
	thread[newID].param = p;		// param saves p on top of stack
  thread[newID].valid = 1;	// mark the entry for the new thread valid
  
  memcpy(thread[newID].env, thread[newID].envInit, sizeof(jmp_buf));
  lastCreateThread = newID;
  enqueue(newID);

	return newID;		// done, return new thread ID
}

/*  MyYieldThread(t) causes the running thread, call it T, to yield to
 * 	thread t.  Returns the ID of the thread that yielded to the calling
 * 	thread T, or -1 if t is an invalid ID.  Example: given two threads
 * 	with IDs 1 and 2, if thread 1 calls MyYieldThread(2), then thread 2
 *  will resume, and if thread 2 then calls MyYieldThread(1), thread 1
 * 	will resume by returning from its call to MyYieldThread(2), which
 *  will return the value 2.
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
  if (t == currThread) return t;
  if (setjmp(thread[currThread].env) == 0) {
    if (thread[currThread].valid) { // check if current thread exits! 
      enqueue(currThread);
    }  
    if (thread[t].schedFlag != 1) {  // check if from sched and already dequeue once!
      removeFromQ(t);
    }
    lastRunThread = currThread; 
    currThread = t;
    if (thread[t].schedFlag != 1) thread[t].schedFlag = 0;
    longjmp(thread[t].env, 1);
  } 

  if (thread[currThread].schedFlag == 0) {
    return lastRunThread;
  } else {    
    thread[currThread].schedFlag = 0;
    return -1;
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
  if (head == -1) {
    return;
  } else {
    int t = dequeue();
    thread[t].schedFlag = 1;
    MyYieldThread(t);
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

  thread[currThread].valid = 0;
  thread[currThread].func = 0; 
  thread[currThread].param = 0; 
  thread[currThread].prev = -1; 
  thread[currThread].next = -1;  
  thread[currThread].schedFlag = 0;
  
  if (head != -1) {
    MySchedThread();
  } else {
    Exit();
  }
}
