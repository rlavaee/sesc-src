/*
 * SECOND: returns elapsed CPU in seconds.  This version can be called
 * from fortran routines in double precision mode.
 */

#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>

#ifndef HZ
#  include <time.h>
#  define HZ CLK_TCK
#endif

second_(t)			
double *t;
{
    struct tms buffer;
        
    if (times(&buffer) == -1) {
	printf("times() call failed\n");
	exit(1);
    }
    *t =  buffer.tms_utime / (double) HZ;
}
