// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "hash.h"
#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h" 
#include "utility.h"
#include <map>
#include "../lib/list.h"
#include <algorithm>
#include <vector>

using namespace std; 
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

map<int,Thread *> tMap; 
List<int> *departedList = new List<int>();
std::vector<int> departedL;



void Exit_POS(int id){
	//departedList->Append(id);
	departedL.push_back(id);
	cout<<"Exit POS is called "<<id<<endl;
	kernel->interrupt->SetLevel(IntOff);
	if(!(tMap.find(id) == tMap.end())){
		Thread *t = tMap[id];
		bool flag = false;
		tMap.erase(id);
		std::map<int,Thread *>::iterator iter = tMap.begin();
		while(iter != tMap.end()){
			if(iter->second->getid() == t->getid()){
				flag = true;
				break;
			}
			iter++;
		}
		if(!flag)
			kernel->scheduler->ReadyToRun(t);
	}
	kernel->currentThread->Finish();
	kernel->interrupt->SetLevel(IntOn);
	return; 
}

void ForkTest1(int id)
{
	printf("ForkTest1 is called, its PID is %d\n", id);
	for (int i = 0; i < 3; i++)
	{
		printf("ForkTest1 is in loop %d\n", i);
		for (int j = 0; j < 100; j++) 
			kernel->interrupt->OneTick();
	}
	Exit_POS(id);
}
void ForkTest2(int id)
{
	printf("ForkTest2 is called, its PID is %d\n", id);
	for (int i = 0; i < 3; i++)
	{
		printf("ForkTest2 is in loop %d\n", i);
		for (int j = 0; j < 100; j++) 
			kernel->interrupt->OneTick();
	}
	Exit_POS(id);
}
void ForkTest3(int id)
{
	printf("ForkTest3 is called, its PID is %d\n", id);
	for (int i = 0; i < 3; i++)
	{
		printf("ForkTest3 is in loop %d\n", i);
		for (int j = 0; j < 100; j++) 
			kernel->interrupt->OneTick();
	}
	Exit_POS(id);
}

void
memoryUpdate(int vpn, int ppn){
	char *charBuff = new char[PageSize];
	TranslationEntry tEntry = kernel->currentThread->space->getVPNEntry(vpn);
	kernel->swapFile->ReadAt(charBuff, PageSize, tEntry.swpLocation*PageSize);
	kernel->currentThread->space->cpToMemory(charBuff, PageSize*ppn);
	kernel->currentThread->space->upgradeEntryOfPage(vpn,ppn,TRUE,TRUE);
	kernel->pMap[ppn] = kernel->currentThread;
	kernel->phyPages.Append(ppn);
	return;
}


void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

    switch (which) {
    case PageFaultException:{
	int badVirtualAddr = kernel->machine->ReadRegister(39);
	int vpn = badVirtualAddr/PageSize;
	int pagesAvailable = Kernel::pageSetMap->FindAndSet();

	if(pagesAvailable!=-1){
		memoryUpdate(vpn, pagesAvailable);
	}
	else{
		int pagesRemoved = kernel->phyPages.RemoveFront(); 
		Thread *t = kernel->pMap[pagesRemoved];
		TranslationEntry tEntry = t->space->getPPNEntry(pagesRemoved);
		kernel->swapFile->WriteAt(&(kernel->machine->mainMemory[pagesRemoved*PageSize]),PageSize,tEntry.swpLocation*PageSize);
		t->space->upgradeEntryOfPage(tEntry.virtualPage,-1,FALSE,FALSE);
		memoryUpdate(vpn,pagesRemoved);
	}
	return;
	break;
    }

    case SyscallException:
      switch(type) {

      	case SC_Halt:{
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();

	ASSERTNOTREACHED();
	break;
}
 case SC_Add:{
	DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	int result;
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	
	/* Modify return point */
	{
	  /* set previous programm counter (debugging only)*/
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	  /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	  /* set next programm counter for brach execution */
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}

	return;
	
	ASSERTNOTREACHED();

	break;}
	case SC_Fork_POS:{
	cout<<"Fork POS is called"<<endl;
	int usrProg; 
	usrProg = (int)kernel->machine->ReadRegister(4);
	Thread *t;
	t = new Thread("Thread Forked");
	switch(usrProg){
		case 1:
			t->Fork((VoidFunctionPtr) ForkTest1, (void *) t->getid()); 
			break;
		
		case 2:
			t->Fork((VoidFunctionPtr) ForkTest2, (void *) t->getid());
                        break;
		
		case 3:
                        t->Fork((VoidFunctionPtr) ForkTest3, (void *) t->getid());
                        break;

		default: 
			cout<<"None of the user programs"<<endl;
			break;
	}
	//int thId;
	//thId = t->getid();
	kernel->machine->WriteRegister(2, (int)t->getid());
	kernel->currentThread->Yield();
	cout<<"End of Fork POS"<<endl;
	{
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
        }
	return; 
	ASSERTNOTREACHED();
	break;}
	     case SC_Write:{

		cout<<"Write SC called"<<endl; 
		int strtLoc;
		int noOfChar;	
		strtLoc = (int)kernel->machine->ReadRegister(4);
		noOfChar = (int)kernel->machine->ReadRegister(5);
		int size;
		size = strtLoc + noOfChar;
		int resLoc;
		while(strtLoc<size){
			bool res;
			res = kernel->machine->ReadMem(strtLoc,1,&resLoc);
			char charRead;
			charRead = (char) resLoc;
			if(res){ 
				printf("%c", charRead);
				strtLoc++;
			}
		}
       {
            /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);

       }

        return;
        ASSERTNOTREACHED();
        break;
	     }

	case SC_Wait_POS:{
	int childID;
	childID = (int)kernel->machine->ReadRegister(4);
	cout<<"Wait POS is called"<<endl;
	
	std::vector<int>::iterator iter;
	iter = find(departedL.begin(),departedL.end(), childID);
	//if(!departedList->IsInList(childID))
	if((iter == departedL.end())){
		tMap[childID] = kernel->currentThread;
		kernel->interrupt->SetLevel(IntOff);
		kernel->currentThread->Sleep(FALSE);		
		kernel->interrupt->SetLevel(IntOn);
		cout<<"Wait has been called, parent put to Sleep"<<endl;
	}

        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
        }
	return;
	break;
}

case SC_Exit:{
	kernel->interrupt->SetLevel(IntOff);
	while(true){
		bool flag = true; 
		std::map<int,Thread *>::iterator iter = tMap.begin();
		while(iter!=tMap.end()){
			if(iter->second->getid() == kernel->currentThread->getid()){
				flag = false;
				break;
			}
			iter++;
		}
		
		if(!flag)
			kernel->currentThread->Yield(); 
		else
			break; 
	}

        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
        }
	kernel->interrupt->SetLevel(IntOn);
	return;
	break;	}
      
    default:
      cerr << "Unexpected user mode exception" << (int)which << "\n";
      break;
    }

    default:
    break;
}
}




