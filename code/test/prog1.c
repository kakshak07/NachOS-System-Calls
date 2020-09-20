
#include "syscall.h"

int
main()
{
	int child;
	child = Fork_POS(1);
	Wait_POS(child);
	child = Fork_POS(2);
	Wait_POS(child);
	child = Fork_POS(3);
	Wait_POS(child);
	Exit(0);
}

