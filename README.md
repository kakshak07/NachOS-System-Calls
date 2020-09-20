# nachos-system-calls
Implemented system calls in nachos and round robin scheduling 

## Objective 
In a real operating system, the kernel not only uses its procedures internally, but allows the user level program to access some of its routines via system calls. We have implemented Fork, Wait, Exit and Write system call routing for user programs and we have implemented round-robin scheduling. 

## What has been developed
Main development has been done in the following files 
 
### exception.cc
*	Exit_POS: This function checks if there are any threads in the map which has the the same threadID as the current thread, if there is any, it puts the current thread into ReadyToRun queue. If not finishes the current thread. 
*	memoryUpdate: This function takes the page table entry using RAM, reads the contents from the swap file into the buffer and then writes the contents of the buffer into the main memory. Updates the page table entry of the main memory, updates the map and updates the main memory list. 
*	SC_Fork_POS: This system call forks off a new thread. The user program that needs to be run is read from register 4. According the value of the register 4 the corresponding thread is spawned, and a user program associated to that thread runs. 
*	SC_Write: This system call writes the contents starting from startLoc and the number of characters to be written from register 5 and writes the contents on to the console character by character.
*	SC_Wait_POS: This system call makes the parent to wait until all of its child process have completed its execution. 
*	SC_Exit: In this system call, if there are any other threads in the map with the same threadID as the current thread, the current thread will be yielded so that other threads could run.  

### addrspace.h
Four new function has been declared. 
*	TranslationEntry getVPNEntry(int vpn) which is a function to get the page table entry from the vpn.
*	TranslationEntry getPPNEntry(int ppn) which is a function to get the page table entry from the ppn.
*	void upgradeEntryOfPage(int vpn, int pageNum, bool valid, bool use) which is a  function to update the page table entry
*	void cpToMemory(char *buff, int index) which is a function to copy the contents of the page table entry into the memory

### addrspace.cc
*	cpTOMemory – copies the contents of the buffer into the main memory. Index gives the starting point from which the buffer needs to be copied. Passes a pointer to the buffer and the index into the function. The characters from the buffer is copied one character at a time. 
*	getVPNEntry – function to retrieve the page table entry from the vpn. The process table has all the details such as the vpn, physical page, valid and if the page is in use for a particular process. Given a vpn, we can easily find out the page table entry for that instance of the vpn.  
*	getPPNEntry – function to retrieve the page table entry from the ppn. Given a ppn, we can obtain its corresponding vpn. From that we can easily get the page table entry for a particular vpn. 
*	upgradeEntryOfPage - update the page table entry of a particular page. When a user program is removed due to page fault exception, new user program is run. This needs to be updated in the page table entry indicating the process which is running that user program, its vpn and other details. 

### Task 1: System calls for user program 
*	In this task we implement system calls for Nachos. These are kernel procedures that user programs can invoke. Here we have implemented Fork, Wait and Exit system calls. 
*	Fork enables the user program to fork a kernel child thread. It takes integer 1, 2 or 3 as parameter, creates a corresponding kernel thread which executes one of the test functions basing on the parameter value, and returns an integer as a process ID of the child. 
*	When a Fork_POS is called by the user program, it results in syscall exception which in turn invokes the Fork_POS system call and it forks a thread for the user program. 
*	When a Wait_POS is called in the user program, it results in the syscall exception and Wait_POS is invoked. It provides a routine for the user program to wait for the execution of its child thread. The parent will be put into sleep until the child wakes it up. 
*	Exit_POS wake up the parent when the child thread is done executing. 

### Task 2: Multitasking
*	The main objective of this task is to run multiple programs in Nachos. Nachos will load the two programs onto the main memory and start executing them as threads with round robin scheduling. 
*	When the command ./nachos -x ../test/mprog1 -x ../test/mprog2 is given the two parameters are loaded into a vector. Then one thread is forked for each of the user program. Each of these threads has a function pointer which points to the user program that needs to be run. When a user program is run, address space for that particular user program is created. 
*	We implement the write system call to print the contents of the user program on to the console. This in turn shows the implementation of multi-tasking in nachos. 



### Task 3: Memory Allocation 
*	Previously in nachos, when a program is run the memory that needed for it was continuously assigned in the main memory. But when you run a program that needs more space than that’s available currently on the RAM it results in an exception. 
*	So, in order to handle this situation, we put the program in the swap file and when there is enough space, we transfer that file on to the buffer and then to the main memory for execution. 
*	We maintain a process table which has all the details of the processes that are currently under execution. When a page fault exception occurs, we check the process table to check to which process that belongs. Then we remove that particular program which is no longer under execution which results in the availability of memory for execution. Then the user program which is currently in swap file is transferred to buffer and then to the main memory for execution.

### Task 4: Round robin scheduling 
*	We implement round robin scheduling to implement multiple user programs in nachos. Each user program is given a quantum to run, after that quantum expires the next user program is run.  

## How to test your solution 
If you’re adding any new user programs to the /nachos/code/test/ folder run make clean; make; command in the test folder. 
If not, navigate to /nachos/code/build.linux/ and run the following commands:
* ./nachos -x ../test/prog1
* ./nachos -x ../test/prog2
* ./nachos -x ../test/prog3
* ./nachos -x ../test/mprog1
* ./nachos -x ../test/mprog2
* ./nachos -x ../test/matmult

By default, the quantum is set to 100, if you want to give quantum by default run the following way 
* ./nachos -x ../test/matmult -q 10000000
