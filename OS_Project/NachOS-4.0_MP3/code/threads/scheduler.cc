// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------
int SJFCompare(Thread *a, Thread *b)
{
    if(a->getBurstTime() == b->getBurstTime())
	return 0;
    return (a->getBurstTime() > b->getBurstTime()) ? 1 : -1;
}

int PriorityCompare(Thread *a, Thread *b)
{
    if(a->getExecPriority() == b->getExecPriority())
	return 0;
    return (a->getExecPriority() > b->getExecPriority()) ? -1 : 1;
}

int RoundRobinCompare(Thread *a, Thread *b)
{
    return -1;
}

Scheduler::Scheduler()
{
    L[2] = new SortedList<Thread *>(RoundRobinCompare);
    L[0] = new SortedList<Thread *>(SJFCompare);
    L[1] = new SortedList<Thread *>(PriorityCompare);
  
    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete L[0];
    delete L[1];
    delete L[2];
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    int priority = thread->getExecPriority();
    thread->setLastCheckInQueueTime(kernel->stats->totalTicks); 
    if(kernel->currentThread != thread) isPreemptive = true;
 
    cout << "Tick [" << kernel->stats->totalTicks  << "]: Thread [" << thread->getID() << "] is inserted into queue L";
    if(priority < 50){
	cout << "3\n";
	L[2]->Insert(thread);
    }else if(priority < 100){
	cout << "2\n";
	L[1]->Insert(thread);
    }else{
	cout << "1\n";
        L[0]->Insert(thread);
    }
//    Print();
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{    
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    if (L[0]->IsEmpty() && L[1]->IsEmpty() && L[2]->IsEmpty()) {
	return NULL;
    }else if(L[1]->IsEmpty() && L[0]->IsEmpty()){
        cout << "Tick [" << kernel->stats->totalTicks << "]: Thread [";
	cout << L[2]->Front()->getID() << "] is removed from queue L3\n";
	return L[2]->RemoveFront();
    }else if(L[0]->IsEmpty()){
        cout << "Tick [" << kernel->stats->totalTicks << "]: Thread [";
        cout << L[1]->Front()->getID() << "] is removed from queue L2\n";
        return L[1]->RemoveFront();
    } else {
        cout << "Tick [" << kernel->stats->totalTicks << "]: Thread [";
	cout << L[0]->Front()->getID() << "] is removed from queue L1\n";
    	return L[0]->RemoveFront();
    }
}

Thread*
Scheduler::FindNext()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    if (L[0]->IsEmpty() && L[1]->IsEmpty() && L[2]->IsEmpty()) {
        return NULL;
    }else if(L[1]->IsEmpty() && L[0]->IsEmpty()){
        return L[2]->Front();
    }else if(L[0]->IsEmpty()){
        return L[1]->Front();
    } else {
        return L[0]->Front();
    }
}
//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------
void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
   
    oldThread->setLastCheckInQueueTime(kernel->stats->totalTicks);

    kernel->currentThread->setExecTime(0);
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    cout << "Tick [" << kernel->stats->totalTicks  << "]: Thread [" << nextThread->getID() << "] is now selected for execution\n";

    cout << "Tick [" << kernel->stats->totalTicks  << "]: Thread [" << oldThread->getID() << "] is replaced, and it has executed [";

    cout << oldThread->getExecTime() << "] ticks\n";
    if(nextThread->getExecPriority() < 50)
	kernel->alarm->SetActive(TRUE);
    else
	kernel->alarm->SetActive(FALSE);
	
    SWITCH(oldThread, nextThread);


    oldThread->setBurstTime(oldThread->getBurstTime()/2 + oldThread->getExecTime()/2);    
    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    cout << "L1:\n";
    L[0]->Apply(ThreadPrint);
    cout << "\nL2:\n";
    L[1]->Apply(ThreadPrint);
    cout << "\nL3:\n";
    L[2]->Apply(ThreadPrint);
}
