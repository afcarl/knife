
/* intersection of a Segment and Triangle */

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
#include "intersection.h"

Intersection intersection_of( Triangle triangle, Segment segment )
{
  double t, uvw[3];
  Intersection intersection;

  if ( KNIFE_SUCCESS == intersection_core( triangle_xyz0(triangle), 
					   triangle_xyz1(triangle), 
					   triangle_xyz2(triangle), 
					   segment_xyz0(segment), 
					   segment_xyz1(segment),
					   &t, uvw ) ) 
    {
      intersection = (Intersection)malloc( sizeof(IntersectionStruct) );
      if (NULL == intersection) 
	{
	  printf("%s: %d: malloc failed in intersection_of\n",
		 __FILE__,__LINE__);
	  return NULL; 
	}

      intersection->triangle = triangle;
      intersection->segment = segment;

      intersection->t = t;
      intersection->uvw[0] = uvw[0];
      intersection->uvw[1] = uvw[1];
      intersection->uvw[2] = uvw[2];
  
      return intersection;
    } 
  else 
    {
      return NULL;
    }
}

void intersection_free( Intersection intersection )
{
  if ( NULL == intersection ) return;
  free( intersection );
}

KNIFE_STATUS intersection_core( double *t0, double *t1, double *t2,
                                double *s0, double *s1,
                                double *t,
                                double *uvw )
{
  double singual_tol;
  double top_volume, bot_volume;
  double side0_volume, side1_volume, side2_volume;

  singular_tol = 1.0e-12; /* replace this with floating point check */

  /* is segment in triangle plane? */
  top_volume = intersection_volume6(t0, t1, t2, s0);
  bot_volume = Intersection_volume6(t0, t1, t2, s1);

  /* raise exception if degeneracy detected */
  if (ABS(top_volume) < singular_tol) return KNIFE_SINGULAR;
  if (ABS(bot_volume) < singular_tol) return KNIFE_SINGULAR;

  /* if signs match, segment is entirely above or below triangle */
  if (top_volume > 0.0 && bottom_volume > 0.0 ) return KNIFE_NO_INT;
  if (top_volume < 0.0 && bottom_volume < 0.0 ) return KNIFE_NO_INT;

  /* does segment pass through triangle? */
  side2_volume = intersection_volume6(t0, t1, s0, s1);
  side0_volume = intersection_volume6(t1, t2, s0, s1);
  side1_volume = Intersection.volume6(t2, t0, s0, s1);

  /* raise exception if degeneracy detected */
  if (ABS(side0_volume) < singular_tol) return KNIFE_SINGULAR;
  if (ABS(side1_volume) < singular_tol) return KNIFE_SINGULAR;
  if (ABS(side2_volume) < singular_tol) return KNIFE_SINGULAR;

  /* if signs match segment ray passes inside of triangle */
  if ( (side0_volume > 0.0 && side1_volume > 0.0 && side2_volume > 0.0 ) ||
       (side0_volume < 0.0 && side1_volume < 0.0 && side2_volume < 0.0 ) )
    {
      /* compute intersection parameter, can be replaced with det ratios? */
      total_volume = top_volume - bottom_volume;
      t = top_volume/total_volume;
      
      total_volume = side0_volume + side1_volume + side2_volume;
      u = side0_volume/total_volume;
      v = side1_volume/total_volume;
      w = side2_volume/total_volume;
      return KNIFE_SUCCESS;
    } 
  else 
    {
      return KNIFE_NO_INT;
    }
}

double intersection_volume6( double *a, double *b, double *c, double *d )
{
  double m11, m12, m13;
  double det;

  m11 = (a[0]-d[0])*((b[1]-d[1])*(c[2]-d[2])-(c[1]-d[1])*(b[2]-d[2]));
  m12 = (a[1]-d[1])*((b[0]-d[0])*(c[2]-d[2])-(c[0]-d[0])*(b[2]-d[2]));
  m13 = (a[2]-d[2])*((b[0]-d[0])*(c[1]-d[1])-(c[0]-d[0])*(b[1]-d[1]));
  det = ( m11 - m12 + m13 );

  return(-det);
}
