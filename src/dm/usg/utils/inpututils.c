/* $Id: inpututils.c,v 1.9 2000/08/17 15:36:11 bsmith Exp bsmith $ */

/*
       Utilities for inputing, creating and managing simple two dimensional grids
*/

#include "src/dm/ao/aoimpl.h"
#include "petscbt.h"
#include "petscdraw.h"

/*
    cell_n        - number of cells
    max_cell      - maximum space allocated for cell
    cell_vertex   - vertices of each cell
    cell_edge     - edges of the cell
    cell_cell     - neighbors of cell
    vertex_n      - number of vertices
    vertex_max    - maximum space allocated for vertices
    x,y           - vertex coordinates

    xmin,ymin,xmax,ymax - bounding box of grid

    edge_n        - total edges in the grid
    edge_vertex   - vertex of all edges 
    edge_max      - maximum space allocated for edge
    edge_cell     - two neighbor cells who share edge

    vertex_boundary - indicates for each vertex if it is a boundary

*/



#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridToAOData"
int AOData2dGridToAOData(AOData2dGrid agrid,AOData *ao)
{
  int        ierr;
  int        *keys,nmax,i;
  AOData     aodata;

  PetscFunctionBegin;
  /*
      Create the database 
  */
  nmax = PetscMax(agrid->cell_n,agrid->vertex_n);
  nmax = PetscMax(nmax,agrid->edge_n);
  keys = (int*)PetscMalloc(nmax*sizeof(int));CHKPTRQ(keys);
  for (i=0; i<nmax; i++) {
    keys[i] = i;
  }
  ierr = AODataCreateBasic(PETSC_COMM_WORLD,&aodata);CHKERRQ(ierr);
    ierr = AODataKeyAdd(aodata,"cell",PETSC_DECIDE,agrid->cell_n);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"cell","cell",4,agrid->cell_n,keys,agrid->cell_cell,PETSC_INT);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"cell","vertex",4,agrid->cell_n,keys,agrid->cell_vertex,PETSC_INT);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"cell","edge",4,agrid->cell_n,keys,agrid->cell_edge,PETSC_INT);CHKERRQ(ierr);
    ierr = AODataKeyAdd(aodata,"edge",PETSC_DECIDE,agrid->edge_n);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"edge","vertex",2,agrid->edge_n,keys,agrid->edge_vertex,PETSC_INT);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"edge","cell",2,agrid->edge_n,keys,agrid->edge_cell,PETSC_INT);CHKERRQ(ierr);
    ierr = AODataKeyAdd(aodata,"vertex",PETSC_DECIDE,agrid->vertex_n);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"vertex","values",2,agrid->vertex_n,keys,agrid->vertex,PETSC_DOUBLE);CHKERRQ(ierr);
      ierr = AODataSegmentAdd(aodata,"vertex","boundary",1,agrid->vertex_n,keys,agrid->vertex_boundary,PETSC_LOGICAL);CHKERRQ(ierr);
  ierr = PetscFree(keys);CHKERRQ(ierr);
  *ao = aodata;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridInput"
/*
       User input the cell by drawing them one at a time
*/
int AOData2dGridInput(AOData2dGrid agrid,Draw draw)
{
  Draw       popup;                           /* help window */
  DrawButton button;                          /* mouse button pressed */
  int        cn, ierr,*cell;
  double     *vertex,cx,cy;
  char       title[120];

  PetscFunctionBegin;
  agrid->cell_max = 500;
  agrid->cell_n   = 0;
  agrid->vertex_max    = 500;
  agrid->vertex_n      = 0;
  agrid->xmin      = PETSC_MAX;
  agrid->xmax      = PETSC_MIN;
  agrid->ymin      = PETSC_MAX;
  agrid->ymax      = PETSC_MIN;

  /*
     Allocate large arrays to hold the nodes and cellrilateral lists 
  */
  vertex = agrid->vertex = (double *)PetscMalloc(2*agrid->vertex_max*sizeof(double));CHKPTRQ(vertex);
  cell = agrid->cell_vertex = (int *)PetscMalloc(4*agrid->cell_max*sizeof(int));CHKPTRQ(cell);



  /*
     Open help window and enter helpful messages
  */
  ierr = DrawGetPopup(draw,&popup);CHKERRQ(ierr);
  ierr = DrawString(popup,.1,.9,DRAW_BLUE,"Use left button to\n   enter cell.");
  ierr = DrawString(popup,.1,.7,DRAW_BLUE,"Use center button to\n   end.");
  ierr = DrawFlush(popup);

  ierr     = DrawGetMouseButton(draw,&button,&cx,&cy,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
  ierr     = AOData2dGridAddNode(agrid,cx,cy,&cn);CHKERRQ(ierr);
  cell[0] = cn;
  sprintf(title,"Input grid: Number vertex %d Number cell %d",agrid->vertex_n,agrid->cell_n);
  ierr = DrawSetTitle(draw,title);CHKERRQ(ierr);
  while (button == BUTTON_LEFT) {
    /* wait for second vertex */
    ierr = DrawGetMouseButton(draw,&button,&cx,&cy,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
    if (button != BUTTON_LEFT) {
      SETERRQ(1,1,"Must press left button to complete cellrilateral");
    }
    ierr     = AOData2dGridAddNode(agrid,cx,cy,&cn);CHKERRQ(ierr);
    cell[4*agrid->cell_n+1] = cn;
    ierr = DrawLine(draw,vertex[2*cell[4*agrid->cell_n]],vertex[1+2*cell[4*agrid->cell_n]],
                         vertex[2*cell[4*agrid->cell_n+1]],vertex[1+2*cell[4*agrid->cell_n+1]],DRAW_RED);
    sprintf(title,"Input grid: Number vertex %d Number cell %d",agrid->vertex_n,agrid->cell_n);
    ierr = DrawSetTitle(draw,title);CHKERRQ(ierr);
    /* wait for third vertex */
    ierr = DrawGetMouseButton(draw,&button,&cx,&cy,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
    if (button != BUTTON_LEFT) {
      SETERRQ(1,1,"Must press left button to complete cellrilateral");
    }
    ierr     = AOData2dGridAddNode(agrid,cx,cy,&cn);CHKERRQ(ierr);
    cell[4*agrid->cell_n+2] = cn;
    ierr = DrawLine(draw,vertex[2*cell[4*agrid->cell_n+1]],vertex[1+2*cell[4*agrid->cell_n+1]],
                         vertex[2*cell[4*agrid->cell_n+2]],vertex[1+2*cell[4*agrid->cell_n+2]],DRAW_RED);
    sprintf(title,"Input grid: Number vertex %d Number cell %d",agrid->vertex_n,agrid->cell_n);
    ierr = DrawSetTitle(draw,title);CHKERRQ(ierr);
    /* wait for fourth vertex */
    ierr = DrawGetMouseButton(draw,&button,&cx,&cy,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
    if (button != BUTTON_LEFT) {
      SETERRQ(1,1,"Must press left button to complete cellrilateral");
    }
    ierr = AOData2dGridAddNode(agrid,cx,cy,&cn);CHKERRQ(ierr);
    cell[4*agrid->cell_n+3] = cn;
    ierr = DrawLine(draw,vertex[2*cell[4*agrid->cell_n+2]],vertex[1+2*cell[4*agrid->cell_n+2]],
                         vertex[2*cell[4*agrid->cell_n+3]],vertex[1+2*cell[4*agrid->cell_n+3]],DRAW_RED);
    ierr = DrawLine(draw,vertex[2*cell[4*agrid->cell_n]],vertex[1+2*cell[4*agrid->cell_n]],
                         vertex[2*cell[4*agrid->cell_n+3]],vertex[1+2*cell[4*agrid->cell_n+3]],DRAW_RED);
    agrid->cell_n++;
    sprintf(title,"Input grid: Number vertex %d Number cell %d",agrid->vertex_n,agrid->cell_n);
    ierr = DrawSetTitle(draw,title);CHKERRQ(ierr);

    /* Get the first for the next cellralateral, or BUTTON_CENTER to end */
    ierr     = DrawGetMouseButton(draw,&button,&cx,&cy,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
    if (button != BUTTON_LEFT) {break;}
    ierr     = AOData2dGridAddNode(agrid,cx,cy,&cn);CHKERRQ(ierr);
    cell[4*agrid->cell_n] = cn;

    sprintf(title,"Input grid: Number vertex %d Number cell %d",agrid->vertex_n,agrid->cell_n);
    ierr = DrawSetTitle(draw,title);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridFlipCell"
/*
   Changes the node numbering for the cell to make sure they are all in 
   clockwise ordering
*/
int AOData2dGridFlipCells(AOData2dGrid agrid)
{
  int    i,*cell = agrid->cell_vertex, cell_n = agrid->cell_n;
  double *vertex = agrid->vertex, sign;

  PetscFunctionBegin;
  for (i=0; i<cell_n; i++) {
    /*
       compute the quantity

            x0      x1    x2      x3
            y0      y1    y2      y3
     */

     sign = vertex[2*cell[4*i]]*vertex[1+2*cell[4*i+1]]   + vertex[2*cell[4*i+1]]*vertex[1+2*cell[4*i+2]] + 
            vertex[2*cell[4*i+2]]*vertex[1+2*cell[4*i+3]] + vertex[2*cell[4*i+3]]*vertex[1+2*cell[4*i]]   -
            vertex[1+2*cell[4*i]]*vertex[2*cell[4*i+1]]   - vertex[1+2*cell[4*i+1]]*vertex[2*cell[4*i+2]] -
            vertex[1+2*cell[4*i+2]]*vertex[2*cell[4*i+3]] - vertex[1+2*cell[4*i+3]]*vertex[2*cell[4*i]];

     if (sign == 0.0) {
       SETERRQ(1,1,"Bad cell");
     } else if (sign > 0) {
       int q1tmp = cell[4*i+1];
       cell[4*i+1] = cell[4*i+3];
       cell[4*i+3] = q1tmp;
     }
  }
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridAddNode"
/*
     AOData2dGridAddNode - Maintains a list of nodes given so far
*/
int AOData2dGridAddNode(AOData2dGrid agrid, double cx, double cy, int *cn)
{
  int i;

  PetscFunctionBegin;
  for (i=0; i<agrid->vertex_n; i++) {
    if ((PetscAbsDouble(agrid->vertex[2*i] - cx) < 1.e-9) && (PetscAbsDouble(agrid->vertex[1+2*i] - cy) < 1.e-9)) {
      *cn = i;
      PetscFunctionReturn(0);
    }
  }
  agrid->vertex[2*agrid->vertex_n] = cx;
  agrid->vertex[1+2*agrid->vertex_n] = cy;
  *cn     = (agrid->vertex_n)++;

  if (cx < agrid->xmin)      agrid->xmin = cx;
  else if (cx > agrid->xmax) agrid->xmax = cx;
  if (cy < agrid->ymin)      agrid->ymin = cy;
  else if (cy > agrid->ymax) agrid->ymax = cy;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridComputeNeighbors"
int AOData2dGridComputeNeighbors(AOData2dGrid agrid)
{
  int  i,j,*cell_edge,*edge_cell,*edge,*cell,*neighbors,e;

  PetscFunctionBegin;
  agrid->edge_max = 2*agrid->vertex_n;
  agrid->edge_n   = 0;
  edge            = agrid->edge_vertex = (int*)PetscMalloc(2*agrid->edge_max*sizeof(int));CHKPTRA(edge);
  cell_edge       = agrid->cell_edge   = (int*)PetscMalloc(4*agrid->cell_max*sizeof(int));CHKPTRA(cell_edge);
  edge_cell       = agrid->edge_cell   = (int*)PetscMalloc(2*agrid->edge_max*sizeof(int));CHKPTRA(edge_cell);
  cell = agrid->cell_vertex;

  /*
       Mark all neighbors (to start) with -1 to indicate missing neighbor
  */
  for (i=0; i<2*agrid->edge_max; i++) {
    edge_cell[i] = -1;
  }

  for (i=0; i<agrid->cell_n; i++) {
    for (j=0; j<agrid->edge_n; j++) {
      if (cell[4*i] == edge[2*j+1] && cell[4*i+1] == edge[2*j]) {
        cell_edge[4*i]   = j;
        edge_cell[2*j+1] = i;
        goto found0;
      }
    }
    /*
       Add a new edge to the list 
    */
    edge_cell[2*agrid->edge_n]   = i;
    edge[2*agrid->edge_n]        = cell[4*i];
    edge[2*agrid->edge_n+1]      = cell[4*i+1];
    cell_edge[4*i]                = agrid->edge_n;
    agrid->edge_n++;
    found0:;
    for (j=0; j<agrid->edge_n; j++) {
      if (cell[4*i+1] == edge[2*j+1] && cell[4*i+2] == edge[2*j]) {
        cell_edge[4*i+1] = j;
        edge_cell[2*j+1] = i;
        goto found1;
      } 
    }
    /*
       Add a new edge to the list 
    */
    edge_cell[2*agrid->edge_n]   = i;
    edge[2*agrid->edge_n]        = cell[4*i+1];
    edge[2*agrid->edge_n+1]      = cell[4*i+2];
    cell_edge[4*i+1]              = agrid->edge_n;
    agrid->edge_n++;
    found1:;
    for (j=0; j<agrid->edge_n; j++) {
      if (cell[4*i+2] == edge[2*j+1] && cell[4*i+3] == edge[2*j]) {
        cell_edge[4*i+2] = j;
        edge_cell[2*j+1] = i;
        goto found2;
      } 
    }
    /*
       Add a new edge to the list 
    */
    edge_cell[2*agrid->edge_n]   = i;
    edge[2*agrid->edge_n]        = cell[4*i+2];
    edge[2*agrid->edge_n+1]      = cell[4*i+3];
    cell_edge[4*i+2]              = agrid->edge_n;
    agrid->edge_n++;
    found2:;
    for (j=0; j<agrid->edge_n; j++) {
      if (cell[4*i+3] == edge[2*j+1] && cell[4*i] == edge[2*j]) {
        cell_edge[4*i+3] = j;
        edge_cell[2*j+1] = i;
        goto found3;
      }
    }
    /*
       Add a new edge to the list 
    */
    edge_cell[2*agrid->edge_n]   = i;
    edge[2*agrid->edge_n]        = cell[4*i+3];
    edge[2*agrid->edge_n+1]      = cell[4*i];
    cell_edge[4*i+3]              = agrid->edge_n;
    agrid->edge_n++;
    found3:;

  }

  neighbors = agrid->cell_cell = (int*)PetscMalloc(4*agrid->cell_n*sizeof(int));CHKPTRQ(neighbors);
  for (i=0; i<agrid->cell_n; i++) {
    for (j=0; j<4; j++) {
      e = 2*agrid->cell_edge[4*i+j]; 

      /* get the edge neighbor that is not the current cell */
      if (i == agrid->edge_cell[e]) e++;
      neighbors[4*i+j] = agrid->edge_cell[e];
    }
  }

  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridComputeVertexBoundary"
int AOData2dGridComputeVertexBoundary(AOData2dGrid agrid)
{
  int  i,j,*count,*cell_vertex = agrid->cell_vertex,ierr;

  PetscFunctionBegin;
  /*
      allocate bitarray for boundary info
  */
  ierr = PetscBTCreate(agrid->vertex_n,agrid->vertex_boundary);CHKERRQ(ierr);

  /*
      count contains number of cell that contain the given vertex 
  */
  count = (int *)PetscMalloc(agrid->vertex_n*sizeof(int));CHKPTRQ(count);
  ierr = PetscMemzero(count,agrid->vertex_n*sizeof(int));CHKERRQ(ierr);

  for (i=0; i<agrid->cell_n; i++) {
    for (j=0; j<4; j++) {
      count[cell_vertex[4*i+j]]++;
    }
  }


  for (i=0; i<agrid->vertex_n; i++) {
    /* UGLY! Just for a quick solution: I want Dirichlet b.c. only at left edge! */
    PetscTruth neumann_bc;
    ierr = OptionsHasName(PETSC_NULL,"-dirichlet_on_left",&neumann_bc);CHKERRQ(ierr);
    if (neumann_bc) {
      if ((count[i] < 4) && (agrid->vertex[2*i] == agrid->xmin)) {
        PetscBTSet(agrid->vertex_boundary,i);
      }
    } else {
      if (count[i] < 4) {
        PetscBTSet(agrid->vertex_boundary,i);
      }
    }
  }

  ierr = PetscFree(count);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridDraw"
/*
     Show the numbering of the vertex, cell and edge
*/
int AOData2dGridDraw(AOData2dGrid agrid,Draw draw)
{
  int    i, *cell = agrid->cell_vertex, *edge = agrid->edge_vertex,ierr;
  char   str[5];
  double *vertex = agrid->vertex,xx,yy,xmin,xmax,ymin,ymax,h,w;

  PetscFunctionBegin;
  w = agrid->xmax - agrid->xmin;
  h = agrid->ymax - agrid->ymin;
  xmin = agrid->xmin - .1*w;
  xmax = agrid->xmax + .1*w;
  ymin = agrid->ymin - .1*h;
  ymax = agrid->ymax + .1*h;
  ierr = DrawSetCoordinates(draw,xmin,ymin,xmax,ymax);CHKERRQ(ierr);

  /*
     Number the vertex
  */
  for (i=0; i<agrid->vertex_n; i++) {
    sprintf(str,"%d",i);
    ierr = DrawString(draw,vertex[2*i],vertex[1+2*i],DRAW_BLUE,str);CHKERRQ(ierr);
  }

  /*
     Number the cell
  */
  for (i=0; i<agrid->cell_n; i++) {
    sprintf(str,"%d",i);
    xx = .25*(vertex[2*cell[4*i]] + vertex[2*cell[4*i+1]] + vertex[2*cell[4*i+2]] + vertex[2*cell[4*i+3]]);
    yy = .25*(vertex[1+2*cell[4*i]] + vertex[1+2*cell[4*i+1]] + vertex[1+2*cell[4*i+2]] + vertex[1+2*cell[4*i+3]]);
    ierr = DrawString(draw,xx,yy,DRAW_GREEN,str);CHKERRQ(ierr);
  }

  /*
     Number the edge
  */
  for (i=0; i<agrid->edge_n; i++) {
    sprintf(str,"%d",i);
    xx = .5*(vertex[2*edge[2*i]] + vertex[2*edge[2*i+1]]);
    yy = .5*(vertex[1+2*edge[2*i]] + vertex[1+2*edge[2*i+1]]);
    ierr = DrawLine(draw,vertex[2*edge[2*i]],vertex[1+2*edge[2*i]],vertex[2*edge[2*i+1]],vertex[1+2*edge[2*i+1]],DRAW_BLACK);CHKERRQ(ierr);
    ierr = DrawString(draw,xx,yy,DRAW_VIOLET,str);CHKERRQ(ierr);
  }

  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridDestroy"
/*
    Frees all the memory space allocated in AGrid
*/
int AOData2dGridDestroy(AOData2dGrid agrid)
{
   int ierr;

   PetscFunctionBegin;
   ierr = PetscFree(agrid->vertex);CHKERRQ(ierr);
   ierr = PetscFree(agrid->cell_vertex);CHKERRQ(ierr);
   ierr = PetscFree(agrid->cell_edge);CHKERRQ(ierr);
   ierr = PetscFree(agrid->edge_vertex);CHKERRQ(ierr);
   ierr = PetscFree(agrid->edge_cell);CHKERRQ(ierr);
   ierr = PetscFree(agrid->cell_cell);CHKERRQ(ierr);
   ierr = PetscFree(agrid->vertex_boundary);CHKERRQ(ierr);
   ierr = PetscFree(agrid);CHKERRQ(ierr);
   PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"AOData2dGridCreate"
/*
    
*/
int AOData2dGridCreate(AOData2dGrid *agrid)
{
   PetscFunctionBegin;
   *agrid = PetscNew(struct _p_AOData2dGrid);CHKPTRQ(*agrid);
   PetscFunctionReturn(0);
}




