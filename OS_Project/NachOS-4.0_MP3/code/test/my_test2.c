#include "syscall.h"

int
main()
{
   int n=0;
   while(n<10000)
   {
     n++;
     if(n==87) PrintInt(n);
   }
	Exit(0);
}

