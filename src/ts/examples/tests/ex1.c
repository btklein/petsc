#ifndef lint
static char vcid[] = "$Id: ex2.c,v 1.2 1996/01/31 03:49:36 bsmith Exp bsmith $";
#endif

/*
          Solves U_t = U_xx 

    1) F(t,u) = (u_i+1 - 2u_i + u_i-1)/h^2

*/

static char help[] = "Solves 1D heat equation.\n\n";

#include "petsc.h"
#include "da.h"
#include "sys.h"
#include "draw.h"
#include <math.h>
#include "ts.h"

#define PI 3.14159265358979
typedef struct {
  Vec    localwork,solution;
  DA     da;
  Viewer viewer1,viewer2;
  int    M;
  double h;
  double norm_2,norm_max;
  int    nox;
} AppCtx;

int Monitor(TS, int, Scalar , Vec, void *);
int RHSFunctionHeat(TS,Scalar,Vec,Vec,void*);
int Initial(Vec, void*);
int RHSMatrixHeat(TS,Scalar,Mat *,Mat *, MatStructure *,void *);
int RHSJacobianHeat(TS,Scalar,Vec,Mat*,Mat*,MatStructure *,void*);

#define linear_no_matrix       0
#define linear_no_time         1
#define linear                 2
#define nonlinear_no_jacobian  3
#define nonlinear              4

int main(int argc,char **argv)
{
  int           M = 60, ierr,  time_steps = 100,steps,flg, problem;
  AppCtx        appctx;
  Vec           local, global;
  Scalar        h, dt,ftime;
  TS            ts;
  TSType        type;
  Mat           A = 0;
  MatStructure  A_structure;
  TSProblemType tsproblem;
  Draw          draw;
  Viewer        viewer;
  char          tsinfo[120];
 
  PetscInitialize(&argc,&argv,(char*)0,help);

  OptionsGetInt(PETSC_NULL,"-M",&M,&flg); appctx.M = M;
  OptionsGetInt(PETSC_NULL,"-time",&time_steps,&flg);
    
  OptionsHasName(PETSC_NULL,"-nox",&flg); 
  if (flg) appctx.nox = 1; else appctx.nox = 0;
  appctx.norm_2 = 0.0; appctx.norm_max = 0.0;

  /* Set up the array */ 
  ierr = DACreate1d(MPI_COMM_WORLD,DA_NONPERIODIC,M,1,1,&appctx.da);CHKERRA(ierr);
  ierr = DAGetDistributedVector(appctx.da,&global); CHKERRA(ierr);
  ierr = DAGetLocalVector(appctx.da,&local); CHKERRA(ierr);

  /* Set up display to show wave graph */
  ierr = ViewerDrawOpenX(MPI_COMM_WORLD,0,"",80,380,400,160,&appctx.viewer1);CHKERRA(ierr);
  ierr = ViewerDrawGetDraw(appctx.viewer1,&draw); CHKERRA(ierr);
  ierr = DrawSetDoubleBuffer(draw); CHKERRA(ierr);
  ierr = ViewerDrawOpenX(MPI_COMM_WORLD,0,"",80,0,400,160,&appctx.viewer2); CHKERRA(ierr);
  ierr = ViewerDrawGetDraw(appctx.viewer2,&draw); CHKERRA(ierr);
  ierr = DrawSetDoubleBuffer(draw); CHKERRA(ierr);

  /* make work array for evaluating right hand side function */
  ierr = VecDuplicate(local,&appctx.localwork); CHKERRA(ierr);

  /* make work array for storing exact solution */
  ierr = VecDuplicate(global,&appctx.solution); CHKERRA(ierr);

  h  = 1.0/(M-1.0); appctx.h = h;

  /* set initial conditions */
  ierr = Initial(global,&appctx); CHKERRA(ierr);

 
  /*
     This example is written to allow one to easily test parts 
    of TS, we do not expect users to generally need to use more
    then a single TSProblemType
  */
  ierr = OptionsHasName(PETSC_NULL,"-linear_no_matrix",&flg); CHKERRA(ierr);
  if (flg) {
    tsproblem = TS_LINEAR;
    problem   = linear_no_matrix;
  }
  ierr = OptionsHasName(PETSC_NULL,"-linear_constant_matrix",&flg); CHKERRA(ierr);
  if (flg) {
    tsproblem = TS_LINEAR;
    problem   = linear_no_time;
  }
  ierr = OptionsHasName(PETSC_NULL,"-linear_variable_matrix",&flg); CHKERRA(ierr);
  if (flg) {
    tsproblem = TS_LINEAR;
    problem   = linear;
  }
  ierr = OptionsHasName(PETSC_NULL,"-nonlinear_no_jacobian",&flg); CHKERRA(ierr);
  if (flg) {
    tsproblem = TS_NONLINEAR;
    problem   = nonlinear_no_jacobian;
  }
  ierr = OptionsHasName(PETSC_NULL,"-nonlinear_jacobian",&flg); CHKERRA(ierr);
  if (flg) {
    tsproblem = TS_NONLINEAR;
    problem   = nonlinear;
  }

    
  /* make time step context */
  ierr = TSCreate(MPI_COMM_WORLD,tsproblem,&ts); CHKERRA(ierr);
  ierr = TSSetMonitor(ts,Monitor,&appctx); CHKERRA(ierr);

  OptionsHasName(PETSC_NULL,"-unstable",&flg);
  if (flg) dt = h*h;
  else     dt = h*h/2.01;

  if (problem == linear_no_matrix || problem == nonlinear_no_jacobian) {
    /*
         In this version the user provides the RHS as a function.
    */
    ierr = TSSetRHSFunction(ts,RHSFunctionHeat,&appctx); CHKERRA(ierr);
  } else if (problem == nonlinear) {
    /*
         In this version the user provides the RHS and Jacobian
    */
    ierr = TSSetRHSFunction(ts,RHSFunctionHeat,&appctx); CHKERRA(ierr);
    ierr = MatCreate(MPI_COMM_WORLD,M,M,&A); CHKERRQ(ierr);
    ierr = RHSMatrixHeat(ts,0.0,&A,&A,&A_structure,&appctx);  CHKERRA(ierr);
    ierr = TSSetRHSJacobian(ts,A,A,RHSJacobianHeat,&appctx); CHKERRA(ierr);  
  } else { 
    /*
        In this version the user provides the RHS as a matrix
    */
    ierr = MatCreate(MPI_COMM_WORLD,M,M,&A); CHKERRQ(ierr);
    ierr = RHSMatrixHeat(ts,0.0,&A,&A,&A_structure,&appctx);  CHKERRA(ierr);

    if (problem == linear_no_time) {
      ierr = TSSetRHSMatrix(ts,A,A,PETSC_NULL,&appctx); CHKERRA(ierr);

    } else {
      ierr = TSSetRHSMatrix(ts,A,A,RHSMatrixHeat,&appctx); CHKERRA(ierr);
    }
  }
  ierr = TSSetFromOptions(ts);CHKERRA(ierr);
  ierr = TSGetType(ts,&type,PETSC_NULL); CHKERRA(ierr);

  ierr = TSSetInitialTimeStep(ts,0.0,dt); CHKERRA(ierr);
  ierr = TSSetDuration(ts,time_steps,100.); CHKERRQ(ierr);
  ierr = TSSetSolution(ts,global); CHKERRA(ierr);


  ierr = TSSetUp(ts); CHKERRA(ierr);
  ierr = TSStep(ts,&steps,&ftime); CHKERRA(ierr);
  ViewerStringOpen(MPI_COMM_WORLD,tsinfo,120,&viewer);
  TSView(ts,viewer);

  PetscPrintf(MPI_COMM_WORLD,"Final (sum) 2 norm %g max norm %g %s\n",
              appctx.norm_2/steps,appctx.norm_max/steps,tsinfo);

  ierr = ViewerDestroy(viewer); CHKERRA(ierr);
  ierr = TSDestroy(ts); CHKERRA(ierr);
  ierr = DADestroy(appctx.da); CHKERRA(ierr);
  ierr = ViewerDestroy(appctx.viewer1); CHKERRA(ierr);
  ierr = ViewerDestroy(appctx.viewer2); CHKERRA(ierr);
  ierr = VecDestroy(appctx.localwork); CHKERRA(ierr);
  ierr = VecDestroy(appctx.solution); CHKERRQ(ierr);
  ierr = VecDestroy(local); CHKERRA(ierr);
  ierr = VecDestroy(global); CHKERRA(ierr);
  if (A) {ierr= MatDestroy(A); CHKERRA(ierr);}

  PetscFinalize();
  return 0;
}
 
int Initial(Vec global, void *ctx)
{
  AppCtx *appctx = (AppCtx*) ctx;
  Scalar *localptr,h = appctx->h;
  int    i,mybase,myend,ierr;

  /* determine starting point of each processor */
  ierr = VecGetOwnershipRange(global,&mybase,&myend); CHKERRQ(ierr);

  /* Initialize the array */
  ierr = VecGetArray(global,&localptr); CHKERRQ(ierr);
  for (i=mybase; i<myend; i++) {
    localptr[i-mybase] = sin(PI*i*6.*h) + 3.*sin(PI*i*2.*h);
  }
  ierr = VecRestoreArray(global,&localptr); CHKERRQ(ierr);
  return 0;
}

int Solution(Scalar t,Vec solution, void *ctx)
{
  AppCtx *appctx = (AppCtx*) ctx;
  Scalar *localptr,h = appctx->h,ex1,ex2,sc1,sc2;
  int    i,mybase,myend,ierr;

  /* determine starting point of each processor */
  ierr = VecGetOwnershipRange(solution,&mybase,&myend); CHKERRQ(ierr);

  ex1 = exp(-36.*PI*PI*t); ex2 = exp(-4.*PI*PI*t);
  sc1 = PI*6.*h;           sc2 = PI*2.*h;
  ierr = VecGetArray(solution,&localptr); CHKERRQ(ierr);
  for (i=mybase; i<myend; i++) {
    localptr[i-mybase] = sin(i*sc1)*ex1 + 3.*sin(i*sc2)*ex2;
  }
  ierr = VecRestoreArray(solution,&localptr); CHKERRQ(ierr);
  return 0;
}

int Monitor(TS ts, int step, double time,Vec global, void *ctx)
{
  AppCtx   *appctx = (AppCtx*) ctx;
  int      ierr;
  double   norm_2,norm_max;
  Scalar   mone = -1.0;
  MPI_Comm comm;

  ierr = PetscObjectGetComm((PetscObject)ts,&comm); CHKERRQ(ierr);

  ierr = VecView(global,appctx->viewer1); CHKERRQ(ierr);

  ierr = Solution(time,appctx->solution, ctx); CHKERRQ(ierr);
  ierr = VecAXPY(&mone,global,appctx->solution); CHKERRQ(ierr);
  ierr = VecNorm(appctx->solution,NORM_2,&norm_2); CHKERRQ(ierr);
  norm_2 = sqrt(appctx->h)*norm_2;
  ierr = VecNorm(appctx->solution,NORM_MAX,&norm_max); CHKERRQ(ierr);

  if (!appctx->nox || step == 1) {
    PetscPrintf(comm,"Time-step %d time %g norm of error %g %g\n",step,time,
                     norm_2,norm_max);
  }

  appctx->norm_2   += norm_2;
  appctx->norm_max += norm_max;

  ierr = VecView(appctx->solution,appctx->viewer2); CHKERRQ(ierr);

  return 0;
}

int RHSFunctionHeat(TS ts, Scalar t,Vec globalin, Vec globalout, void *ctx)
{
  AppCtx *appctx = (AppCtx*) ctx;
  DA     da = appctx->da;
  Vec    local, localwork = appctx->localwork;
  int    ierr,i,localsize; 
  Scalar *copyptr, *localptr,sc;

  /*Extract local array */ 
  ierr = DAGetLocalVector(da,&local); CHKERRQ(ierr);
  ierr = DAGlobalToLocalBegin(da,globalin,INSERT_VALUES,local); CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(da,globalin,INSERT_VALUES,local); CHKERRQ(ierr);
  ierr = VecGetArray(local,&localptr); CHKERRQ(ierr);

  /* Extract work vector */
  ierr = VecGetArray(localwork,&copyptr); CHKERRQ(ierr);

  /* Update Locally - Make array of new values */
  /* Note: I don't do anything for the first and last entry */
  sc = 1.0/(appctx->h*appctx->h);
  ierr = VecGetLocalSize(local,&localsize); CHKERRQ(ierr);
  copyptr[0] = 0.0;
  for (i=1; i<localsize-1; i++) {
    copyptr[i] = sc * (localptr[i+1] + localptr[i-1] - 2.0*localptr[i]);
  }
  copyptr[localsize-1] = 0.0;
  ierr = VecRestoreArray(localwork,&copyptr); CHKERRQ(ierr);

  /* Local to Global */
  ierr = DALocalToGlobal(da,localwork,INSERT_VALUES,globalout); CHKERRQ(ierr);
  return 0;
}

/* ---------------------------------------------------------------------*/
int RHSMatrixHeat(TS ts,Scalar t,Mat *AA,Mat *BB, MatStructure *str,void *ctx)
{
  Mat    A = *AA;
  AppCtx *appctx = (AppCtx*) ctx;
  int    ierr,i,mstart,mend,rank,size, idx[3];
  Scalar v[3],stwo = -2./(appctx->h*appctx->h), sone = -.5*stwo;

  *str = SAME_NONZERO_PATTERN;

  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&size);

  ierr = MatGetOwnershipRange(A,&mstart,&mend); CHKERRQ(ierr);
  if (mstart == 0) mstart++;
  if (mend == appctx->M) mend--;

  /*
     Construct matrice one row at a time
  */
  v[0] = sone; v[1] = stwo; v[2] = sone;  
  for ( i=mstart; i<mend; i++ ) {
    idx[0] = i-1; idx[1] = i; idx[2] = i+1;
    ierr = MatSetValues(A,1,&i,3,idx,v,INSERT_VALUES); CHKERRQ(ierr);
  }

  ierr = MatAssemblyBegin(A,FINAL_ASSEMBLY); CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,FINAL_ASSEMBLY); CHKERRQ(ierr);
  return 0;
}

int RHSJacobianHeat(TS ts,Scalar t,Vec x,Mat *AA,Mat *BB, MatStructure *str,
                    void *ctx)
{
  return RHSMatrixHeat(ts,t,AA,BB,str,ctx);
}
