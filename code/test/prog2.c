/*** prog2.c ***/
#include "syscall.h"

int
main()
{
	int child1, child2;
	child1 = Fork_POS(1);
	child2 = Fork_POS(2);
	Wait_POS(child1);
	Wait_POS(child2);
	Exit(0);
}

