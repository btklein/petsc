/*$Id: matregis.c,v 1.3 2000/07/10 03:39:27 bsmith Exp bsmith $*/

#include "petscmat.h"  /*I "petscmat.h" I*/

EXTERN_C_BEGIN
EXTERN int MatCreate_MAIJ(Mat);
EXTERN int MatCreate_IS(Mat);
EXTERN_C_END
  
/*
    This is used by MatSetType() to make sure that at least one 
    MatRegisterAll() is called. In general, if there is more than one
    DLL, then MatRegisterAll() may be called several times.
*/
EXTERN PetscTruth MatRegisterAllCalled;

#undef __FUNC__  
#define __FUNC__ /*<a name="MatRegisterAll"></a>*/"MatRegisterAll"
/*@C
  MatRegisterAll - Registers all of the matrix types in PETSc

  Not Collective

  Level: advanced

.keywords: KSP, register, all

.seealso:  MatRegisterDestroy()
@*/
int MatRegisterAll(char *path)
{
  int ierr;

  PetscFunctionBegin;
  MatRegisterAllCalled = PETSC_TRUE;

  ierr = MatRegisterDynamic(MATMPIMAIJ, path,"MatCreate_MAIJ",   MatCreate_MAIJ);CHKERRQ(ierr);
  ierr = MatRegisterDynamic(MATSEQMAIJ, path,"MatCreate_MAIJ",   MatCreate_MAIJ);CHKERRQ(ierr);
  ierr = MatRegisterDynamic(MATIS,      path,"MatCreate_IS",     MatCreate_IS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

