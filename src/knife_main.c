/* main driver program for knife package */

/* $Id$ */

/* Michael A. Park (Mike Park)
 * Computational AeroSciences Branch
 * NASA Langley Research Center
 * Hampton, VA 23681
 * Phone:(757) 864-6604
 * Email: Mike.Park@NASA.Gov
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> /* for sleep */
#include <string.h>
#include "knife_definitions.h"
#include "array.h"
#include "primal.h"
#include "surface.h"
#include "domain.h"

#define TRY(fcn,msg)					      \
  {							      \
    int code;						      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      return code;					      \
    }							      \
  }

#define NOT_NULL(pointer,msg)				      \
  if (NULL == (pointer)) {				      \
    printf("%s: %d: %s\n",__FILE__,__LINE__,(msg));	      \
    return KNIFE_NULL;					      \
  }

int main( int argc, char *argv[] )
{
  int argument;

  char surface_filename[1025];
  Primal surface_primal;
  int bc;
  Set bcs;
  Surface surface;

  char volume_filename[1025];
  Primal volume_primal;
  Domain domain;

  KnifeBool inward_pointing_surface_normal = TRUE;
  KnifeBool tecplot_output = FALSE;
  KnifeBool extra_visualization = FALSE;
  KnifeBool arguments_require_stop = FALSE;
  int nnodes0 = EMPTY;

  sprintf( surface_filename, "not_set" );

  bcs = set_create(10,10);
  NOT_NULL(bcs, "Set bcs NULL");

  argument = 1;
  while ( argument < argc )
    {
      if( strcmp(argv[argument],"-s") == 0 ) {
	argument++; 
	sprintf( surface_filename, "%s", argv[argument] );
	printf("-s %s\n", surface_filename);
      }

      if( strcmp(argv[argument],"-r") == 0 ) {
	inward_pointing_surface_normal = FALSE;
	printf("-r\n");
      }

      if( strcmp(argv[argument],"-v") == 0 ) {
	argument++; 
	sprintf( volume_filename, "%s", argv[argument] );
	printf("-v %s\n", volume_filename);
      }

      if( strcmp(argv[argument],"-b") == 0 ) {
	argument++;
	bc = atoi(argv[argument]);
	set_insert( bcs, bc );
	printf("-b %d\n", bc );
      }

      if( strcmp(argv[argument],"-t") == 0 ) {
	tecplot_output = TRUE;
	printf("-t\n");
      }

      if( strcmp(argv[argument],"-x") == 0 ) {
	extra_visualization = TRUE;
	printf("-x\n");
      }

      if( strcmp(argv[argument],"--nnodes0") == 0 ) {
	argument++;
	nnodes0 = atoi(argv[argument]);
	printf("--nnodes0 %d\n", nnodes0 );
      }

      if( (strcmp(argv[argument],"-h") == 0 )    ||
	  (strcmp(argv[argument],"--help") == 0 ) ) {
	printf("-s surface fgrid filename\n");
	printf("-r surface triangle normals point out of the domain\n");
	printf("-b face index of surface grid used to cut\n");
	printf("-v volume fgrid filename\n");
	printf("-t tecplot output\n");
	printf("-x extra visualization\n");
	printf("--nnodes0 parallel debug mode, number of local nodes\n");
	printf("-h,--help display help info and exit\n");
	printf("--version display version info and exit\n");
	arguments_require_stop = TRUE;
      }

      if( (strcmp(argv[argument],"--version") == 0 ) ) {
	printf("knife cut cell driver version %s\n",VERSION);
	arguments_require_stop = TRUE;
      }

      argument++;
    }

  if (arguments_require_stop) return 0;

  surface_primal = primal_from_fast( surface_filename );
  NOT_NULL(surface_primal, "surface_primal NULL");

  if( strcmp(surface_filename,"bump.fgrid") == 0 ) {
    printf("%s: %d: debug scale!!!\n",__FILE__,__LINE__);
    primal_scale_about( surface_primal, 0.0, 1.0, -1.0, 1.0+1.0+0.05 );
  }

  surface = surface_from( surface_primal, bcs, 
			  inward_pointing_surface_normal );
  NOT_NULL(surface, "surface NULL");

  printf("read in volume grid\n");
  volume_primal = primal_from_fast( volume_filename );
  NOT_NULL(volume_primal, "volume_primal NULL");
  if (nnodes0>0) volume_primal->nnode0 = nnodes0;

  if (extra_visualization)
    {
      int poly_index;
      printf("extra visualization for poly\n");
      domain = domain_create( volume_primal, surface );
      TRY( domain_all_dual( domain ), "domain_all_dual" );
      TRY( domain_dual_elements( domain ), "domain_dual_elements" );
      for ( poly_index = 0 ;
	    poly_index < domain_npoly(domain) ;
	    poly_index++ )
	{
	  poly_activate_all_subtri( domain_poly(domain,poly_index) );
	  poly_tecplot( domain_poly(domain,poly_index) );
	}
      printf("clean domain\n");
      domain_free( domain );
    }

  domain = domain_create( volume_primal, surface );
  NOT_NULL(domain, "domain NULL");

  if (nnodes0>0)
    {
      TRY( domain_required_dual( domain ), "domain_required_dual" );
    }
  else
    {
      TRY( domain_all_dual( domain ), "domain_all_dual" );
    }

  TRY( domain_boolean_subtract( domain ), "boolean subtract" );
  if (tecplot_output) domain_tecplot( domain, "cut.t" );

  return 0;
}

