#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: fdate.c,v 1.15 1997/09/05 22:13:42 balay Exp gropp $";
#endif

#include "src/sys/src/files.h"
#if defined (PARCH_IRIX64) || defined (PARCH_IRIX)
#include <sys/resource.h>
#if defined(__cplusplus)
extern "C" {
extern int gettimeofday(struct timeval *, struct timezone *);
}
#endif
#endif
/*
     Some versions of the Gnu g++ compiler require a prototype for gettimeofday()
  on the IBM rs6000. Also CC on some IRIX64 machines

#if defined(__cplusplus)
extern "C" {
extern int gettimeofday(struct timeval *, struct timezone *);
}
#endif

*/
   
#undef __FUNC__  
#define __FUNC__ "PetscGetDate"
char *PetscGetDate()
{
#if defined (PARCH_nt)
  time_t aclock;
  time( &aclock);
  return asctime(localtime(&aclock));
#else
  struct timeval tp;
  gettimeofday( &tp, (struct timezone *)0 );
  return asctime(localtime((time_t *) &tp.tv_sec));
#endif
}
