// mprog1.c for Task 2, 3 and 4 testing
#include "syscall.h"

int
main()
{
	OpenFileId output = ConsoleOutput;
    
	char* str = "current user program: prog1";
	
	int i,j;
	for (i = 0; i < 5; i++)
	{
		Write(str, 27, output);
		for (j = 0; j < 10000; j++);
	}
        
	Exit(0);
}

