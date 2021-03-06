
/* volume defined by a watertight collection of masks */

/* Copyright 2007 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The knife platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "poly.h"
#include "set.h"
#include "cut.h"
#include "logger.h"

#define POLY_LOGGER_LEVEL (1)

static int poly_tecplot_frame = 0;

#define TRY(fcn,msg)					      \
  {							      \
    int code;						      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      return code;					      \
    }							      \
  }

#define TRYT(fcn,msg)					      \
  {							      \
    int code;						      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      poly_tecplot( poly );                                   \
      return code;					      \
    }							      \
  }

#define NOT_NULL(pointer,msg)				      \
  if (NULL == (pointer)) {				      \
    printf("%s: %d: %s\n",__FILE__,__LINE__,(msg));	      \
    return KNIFE_NULL;					      \
  }

static int  inward_subnode_order[] = {1, 0, 2};
static int outward_subnode_order[] = {0, 1, 2};

Poly poly_create( void )
{
  Poly poly;
  
  poly = (Poly) malloc( sizeof(PolyStruct) );
  if (NULL == poly) {
    printf("%s: %d: malloc failed in poly_create\n",
	   __FILE__,__LINE__);
    return NULL; 
  }

  if ( KNIFE_SUCCESS != poly_initialize( poly ))
    {
      poly_free( poly );
      return NULL;
    };

  return poly;
}

KNIFE_STATUS poly_initialize( Poly poly )
{

  poly->mask = array_create(4,40);
  NOT_NULL( poly->mask, "poly mask array null" );

  poly->surf = array_create(4,40);
  NOT_NULL( poly->surf, "poly surf array null" );

  return KNIFE_SUCCESS;
}

void poly_free( Poly poly )
{
  int mask_index;

  if ( NULL == poly ) return;

  for ( mask_index = 0; mask_index < poly_nmask(poly); mask_index++)
    mask_free( poly_mask(poly, mask_index) );
  array_free( poly->mask );

  for ( mask_index = 0; mask_index < poly_nsurf(poly); mask_index++)
    mask_free( poly_surf(poly, mask_index) );
  array_free( poly->surf );

  free( poly );
}

KNIFE_STATUS poly_set_frame( int frame )
{
  poly_tecplot_frame = frame;
  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_add_triangle( Poly poly, Triangle triangle, 
				KnifeBool inward_pointing_normal )
{
  return poly_add_mask( poly, mask_create( triangle, inward_pointing_normal ) );
}

KnifeBool poly_has_surf_triangle( Poly poly, Triangle triangle )
{
  int surf_index;
  for ( surf_index = 0;
	surf_index < poly_nsurf(poly); 
	surf_index++)
    {
      if ( triangle == mask_triangle( poly_surf(poly, surf_index) ) )
	   return TRUE;
    }
  
  return FALSE;
}

KNIFE_STATUS poly_mask_with_triangle( Poly poly, Triangle triangle, Mask *mask )
{
  int mask_index;

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    {
      if ( triangle == mask_triangle( poly_surf(poly, mask_index) ) )
	{
	  *mask = poly_surf(poly, mask_index);
	  return KNIFE_SUCCESS;
	}
    }
  
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      if ( triangle == mask_triangle( poly_mask(poly, mask_index) ) )
	{
	  *mask = poly_mask(poly, mask_index);
	  return KNIFE_SUCCESS;
	}
    }
  
  return KNIFE_NOT_FOUND;
}

KNIFE_STATUS poly_gather_surf( Poly poly )
{
  int mask_index;
  Triangle triangle, other;
  int cut_index;
  Cut cut;

  if ( NULL == poly) return KNIFE_NULL;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      triangle = mask_triangle(poly_mask(poly, mask_index));
      for ( cut_index = 0;
	    cut_index < triangle_ncut(triangle); 
	    cut_index++)
	{
	  cut = triangle_cut(triangle,cut_index);
	  other = cut_other_triangle(cut,triangle);
	  if ( !poly_has_surf_triangle(poly,other) )
	    TRY( poly_add_surf(poly,mask_create( other, TRUE ) ), "add surf");
	}
    }
  
  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_activate_all_subtri( Poly poly )
{
  int mask_index;

  if ( NULL == poly ) return KNIFE_NULL;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    TRY(mask_activate_all_subtri( poly_mask(poly, mask_index) ),"deact mask");

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    TRY(mask_activate_all_subtri( poly_surf(poly, mask_index) ),"deact surf");

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_determine_active_subtri( Poly poly )
{
  int mask_index;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    TRYT(mask_deactivate_all_subtri( poly_mask(poly, mask_index) ),"deact mask");

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    TRYT(mask_deactivate_all_subtri( poly_surf(poly, mask_index) ),"deact surf");

   TRYT( poly_activate_subtri_at_cuts( poly ), "activate at cuts");
   TRYT( poly_paint( poly ), "paint");

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_activate_subtri_at_cuts( Poly poly )
{
  int mask_index;
  int cut_index;
  Triangle triangle, cutter;
  Cut cut;
  Subtri subtri;
  Subtri cutter_subtri01, cutter_subtri10;
  Subtri triang_subtri01, triang_subtri10;
  Mask mask, surf;
  double volume01, volume10;
  double area01, area10;
  int region;

  region = 0;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( cut_index = 0;
	    cut_index < triangle_ncut(triangle); 
	    cut_index++)
	{
	  region += 1;
	  cut = triangle_cut(triangle,cut_index);
	  cutter = cut_other_triangle(cut,triangle);
	  TRY( poly_mask_with_triangle(poly, cutter, &surf), "cutter mask" );

	  TRY( triangle_subtri_with_intersections( cutter, 
						   cut_intersection0(cut), 
						   cut_intersection1(cut),
						   &cutter_subtri01 ), 
	       "cutter_subtri01");
	  TRY( triangle_subtri_with_intersections( cutter, 
						   cut_intersection1(cut), 
						   cut_intersection0(cut),
						   &cutter_subtri10 ), 
	       "cutter_subtri10");

	  TRY( triangle_subtri_with_intersections( triangle, 
						   cut_intersection0(cut), 
						   cut_intersection1(cut),
						   &triang_subtri01 ), 
	       "triang_subtri01");
	  TRY( triangle_subtri_with_intersections( triangle, 
						   cut_intersection1(cut), 
						   cut_intersection0(cut),
						   &triang_subtri10 ), 
	       "triang_subtri10");

	  subtri = cutter_subtri01;
	  area10 = subtri_reference_area( cutter_subtri10 );
	  area01 = subtri_reference_area( cutter_subtri01 );
	  if ( area10 > area01 ) subtri = cutter_subtri10;

	  TRY( subtri_contained_volume6( subtri, triang_subtri01, 
					 &volume01), "vol");

	  TRY( subtri_contained_volume6( subtri, triang_subtri10, 
					 &volume10), "vol");

	  if ( ( volume01 > 0.0 && volume10 > 0.0 ) ||
	       ( volume01 < 0.0 && volume10 < 0.0 ) ||
	       knife_double_zero(volume01) || knife_double_zero(volume10) )
	    {
	      subtri_echo( triang_subtri01 );
	      subtri_echo( triang_subtri10 );
	      printf("%s: %d: inside inconsistent %.16e %.16e\n",
		     __FILE__,__LINE__,volume01, volume10);
	      mask_tecplot(surf);
	      mask_tecplot(mask);
	      triangle_tecplot(cutter);
	      triangle_tecplot(triangle);
	      poly_tecplot(poly);
	      return KNIFE_INCONSISTENT;
	    }

	  if ( (  surf->inward_pointing_normal && volume01 > volume10 ) || 
	       ( !surf->inward_pointing_normal && volume01 < volume10 ) )
	    {
	      TRY( mask_activate_subtri(mask, triang_subtri01, region ), 
		   "active m01");
	    }
	  else
	    {
	      TRY( mask_activate_subtri(mask, triang_subtri10, region ), 
		   "active m10");
	    }

	  subtri = triang_subtri01;
	  area10 = subtri_reference_area( triang_subtri10 );
	  area01 = subtri_reference_area( triang_subtri01 );
	  if ( area10 > area01 ) subtri = triang_subtri10;

	  TRY( subtri_contained_volume6( subtri, cutter_subtri01, 
					 &volume01), "vol");

	  TRY( subtri_contained_volume6( subtri, cutter_subtri10, 
					 &volume10), "vol");

	  if ( ( volume01 > 0.0 && volume10 > 0.0 ) ||
	       ( volume01 < 0.0 && volume10 < 0.0 ) ||
	       knife_double_zero(volume01) || knife_double_zero(volume10) )
	    {
	      subtri_echo( cutter_subtri01 );
	      subtri_echo( cutter_subtri10 );
	      printf("%s: %d: inside inconsistent %.16e %.16e\n",
		     __FILE__,__LINE__,volume01, volume10);
	      mask_tecplot(mask);
	      mask_tecplot(surf);
	      triangle_tecplot(triangle);
	      triangle_tecplot(cutter);
	      poly_tecplot(poly);
	      return KNIFE_INCONSISTENT;
	    }

	  if ( (  mask->inward_pointing_normal && volume01 > volume10 ) || 
	       ( !mask->inward_pointing_normal && volume01 < volume10 ) )
	    {
	      TRY( mask_activate_subtri(surf, cutter_subtri01, region ), 
		   "active s01");
	    }
	  else
	    {
	      TRY( mask_activate_subtri(surf, cutter_subtri10, region ), 
		   "active s10");
	    }

	}/* cut loop */
    } /* mask loop */

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_paint( Poly poly )
{
  int mask_index, subtri_index;
  Mask mask;
  Triangle triangle;
  KnifeBool another_coat_of_paint;
  int segment_index;

  int region;
  Set regions;
  int number_cut;

  KNIFE_STATUS verify_code;

  logger_message( POLY_LOGGER_LEVEL, "paint:start");

  /* paint poly mask */
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      TRY( mask_paint( poly_mask(poly, mask_index) ), "mask paint");
      verify_code = mask_verify_paint( poly_mask(poly, mask_index) );

      if ( KNIFE_SUCCESS != verify_code )
	{
	  printf("verify failed after mask paint\n");
	  poly_tecplot( poly );
	  return verify_code;
	}
    }

  /* paint poly surf */
  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    {
      TRY( mask_paint( poly_surf(poly, mask_index) ), "surf paint");
      verify_code = mask_verify_paint( poly_surf(poly, mask_index) );

      if ( KNIFE_SUCCESS != verify_code )
	{
	  printf("verify failed after surf paint\n");
	  poly_tecplot( poly );
	  return verify_code;
	}
    }

  verify_code = poly_verify_painting( poly );

  if ( KNIFE_SUCCESS != verify_code )
    {
      printf("verify failed after mask paint\n");
      poly_tecplot( poly );
      return verify_code;
    }

  logger_message( POLY_LOGGER_LEVEL, "paint:masks");

  /* activate poly masks that were not cut */
  another_coat_of_paint = TRUE;
  while (another_coat_of_paint)
    {
      another_coat_of_paint = FALSE;
      for ( mask_index = 0;
	    mask_index < poly_nmask(poly); 
	    mask_index++)
	{
	  mask = poly_mask(poly,mask_index);
	  triangle = mask_triangle(mask);
	  if ( (1 == triangle_nsubtri(triangle)) && 
	       !mask_subtri_active(mask,0) )
	    {
	      if ( poly_active_mask_with_nodes( poly, 
						triangle->node0,
						triangle->node1,
						triangle->node2,
						&region )  )
		{
		  mask_activate_subtri_index( mask, 0, region );
		  another_coat_of_paint = TRUE;
		}	       
	    }
	}
    }
  
  logger_message( POLY_LOGGER_LEVEL, "paint:uncut masks");

  /* activate poly surfaces that were not cut */
  number_cut = poly_nsurf(poly);
  /* backwards for relax speed up, MAX(region) */
  for ( mask_index = number_cut-1;
	mask_index >= 0 ;
	mask_index--)
    {
      mask = poly_surf(poly,mask_index);
      triangle = mask_triangle(mask);
      for ( segment_index = 0; segment_index < 3; segment_index++ )
	TRY ( poly_paint_surf( poly, mask,
			       triangle_segment(triangle,segment_index)), 
	      "paint surf ");
    }
  logger_message( POLY_LOGGER_LEVEL, "paint:uncut surfs");

  verify_code = poly_verify_painting( poly );

  if ( KNIFE_SUCCESS != verify_code )
    {
      printf("verify failed after uncut neighbor search\n");
      poly_tecplot( poly );
      return verify_code;
    }

  logger_message( POLY_LOGGER_LEVEL, "paint:active");

  /* everyone is active now, relax regions */
  TRY( poly_relax_region( poly ), "region relax" );

  logger_message( POLY_LOGGER_LEVEL, "paint:relax");

  /* compact region indexes (set counter?) */
  regions = set_create( 10, 10 );
  set_insert( regions, 0 ); /* so inactive regions stay inactive */

  for ( mask_index = 0; mask_index < poly_nmask(poly); mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	set_insert( regions, mask_subtri_region(mask,subtri_index) );
    }

  for ( mask_index = 0; mask_index < poly_nsurf(poly); mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	set_insert( regions, mask_subtri_region(mask,subtri_index) );
    }

  for ( mask_index = 0; mask_index < poly_nmask(poly); mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	mask->region[subtri_index] = set_index_of( regions, 
						   mask->region[subtri_index] );
    }
  
  for ( mask_index = 0; mask_index < poly_nsurf(poly); mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	mask->region[subtri_index] = set_index_of( regions, 
						   mask->region[subtri_index] );
    }

  set_free( regions );

  logger_message( POLY_LOGGER_LEVEL, "paint:compact");

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_verify_painting( Poly poly )
{
  int mask_index;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    TRY( mask_verify_paint( poly_mask(poly, mask_index) ), "mask verify");

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    TRY( mask_verify_paint( poly_surf(poly, mask_index) ), "surf verify");

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_relax_region( Poly poly )
{
  int mask_index;

  Triangle triangle, cutter;
  Mask mask, surf;
  int cut_index;
  Cut cut;
  int mask_region, surf_region;

  int segment_index;
  Segment segment;

  KnifeBool more_relaxation;

  more_relaxation = FALSE;

  /* mask interiors */
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    TRY( mask_paint( poly_mask(poly, mask_index) ), "mask paint");

  /* surf interiors */
  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    TRY( mask_paint( poly_surf(poly, mask_index) ), "surf paint");

  /* cuts */
  
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( cut_index = 0;
	    cut_index < triangle_ncut(triangle); 
	    cut_index++)
	{
	  cut = triangle_cut(triangle,cut_index);
	  cutter = cut_other_triangle(cut,triangle);
	  TRY( poly_mask_with_triangle(poly, cutter, &surf), "cutter mask" );
	  TRY( mask_intersection_region( mask,
					 cut_intersection0(cut), 
					 cut_intersection1(cut),
					 &mask_region), "mask region" );
	  TRY( mask_intersection_region( surf,
					 cut_intersection0(cut), 
					 cut_intersection1(cut),
					 &surf_region), "surf region" );
	  if ( mask_region != surf_region )
	    {
	      more_relaxation = TRUE;
	      poly_collapse_regions( poly, mask_region, surf_region );
	    }
	}
    }

  /* uncut mask segments */

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( segment_index = 0; segment_index < 3; segment_index++ )
	{
	  segment = triangle_segment(triangle, segment_index);
	  if ( 0 == segment_nintersection( segment ) ) 
	    TRY( poly_relax_nodes( poly, mask, 
				   segment_node0(segment),
				   segment_node1(segment),
				   &more_relaxation ),
		 "relax_nodes" );
	}
    }

  /* uncut surf segments */

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( segment_index = 0; segment_index < 3; segment_index++ )
	{
	  segment = triangle_segment(triangle, segment_index);
	  TRY( poly_relax_segment( poly, mask, segment, &more_relaxation ),
	       "relax_segment" );
	}
    }

  if ( more_relaxation ) return poly_relax_region( poly );
  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_relax_nodes( Poly poly, Mask mask, Node node0, Node node1,
			       KnifeBool *more_relaxation )
{
  Triangle triangle, other_triangle;
  Mask other_mask;
  int mask_index;
  int subtri_index0, subtri_index1;
  int region0, region1;
  KNIFE_STATUS status;

  triangle = mask_triangle(mask);
  TRY( triangle_subtri_index_with_nodes( triangle, node0, node1,
					 &subtri_index0 ), "st0" );
  region0 = mask_subtri_region(mask, subtri_index0);

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      other_mask = poly_mask(poly, mask_index);
      other_triangle = mask_triangle(other_mask);
      if (triangle != other_triangle)
	{
	  status = triangle_subtri_index_with_nodes( other_triangle, 
						     node0, node1,
						     &subtri_index1 );
	  if ( KNIFE_NOT_FOUND == status ) continue;
	  TRY( status, "st1" );
	  region1 = mask_subtri_region(other_mask, subtri_index1);

	  if ( region0 == region1 )
	    {
	      return KNIFE_SUCCESS;
	    }
	  else
	    {
	      *more_relaxation = TRUE;
	      poly_collapse_regions( poly, region0, region1 );
	      return KNIFE_SUCCESS;
	    }
	}
    }
  
  return KNIFE_NOT_FOUND;
}
KNIFE_STATUS poly_relax_segment( Poly poly, Mask mask, Segment segment, 
				      KnifeBool *more_relaxation )
{
  Triangle triangle, other_triangle;
  Mask other_mask;
  int triangle_index;
  int subtri_index0, subtri_index1;
  KNIFE_STATUS status;
  int region0, region1;

  if ( 0 != segment_nintersection( segment  ) ) return KNIFE_SUCCESS;

  triangle = mask_triangle(mask);
  for ( triangle_index = 0;
	triangle_index < segment_ntriangle(segment); 
	triangle_index++)
    {
      other_triangle = segment_triangle(segment, triangle_index);
      if (triangle != other_triangle)
	{
	  status = poly_mask_with_triangle( poly, other_triangle, &other_mask );
	  if ( KNIFE_NOT_FOUND == status ) continue;
	  TRY( status, "poly mask from triangle" );

	  TRY( triangle_subtri_index_with_nodes( triangle, 
						 segment_node0(segment),
						 segment_node1(segment),
						 &subtri_index0 ), "st0" );
	  region0 = mask_subtri_region(mask, subtri_index0);

	  TRY( triangle_subtri_index_with_nodes( other_triangle, 
						 segment_node0(segment),
						 segment_node1(segment),
						 &subtri_index1 ), "st1" );
	  region1 = mask_subtri_region(other_mask, subtri_index1);

	  if ( region0 != region1 )
	    {
	      *more_relaxation = TRUE;
	      poly_collapse_regions( poly, region0, region1 );
	      return KNIFE_SUCCESS;
	    }
	}
    }
  

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_collapse_regions( Poly poly, int region0, int region1 )
{
  int mask_index;

  for ( mask_index = 0; mask_index < poly_nmask(poly); mask_index++)
    mask_collapse_regions( poly_mask(poly, mask_index), region0, region1 );

  for ( mask_index = 0; mask_index < poly_nsurf(poly); mask_index++)
    mask_collapse_regions( poly_surf(poly, mask_index), region0, region1 );

  return KNIFE_SUCCESS;
}

KnifeBool poly_active_mask_with_nodes( Poly poly, 
				       Node n0, Node n1, Node n2,
				       int *region )
{
  int mask_index, subtri_index;
  Mask mask;
  Triangle triangle;
  
  *region = 0;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      if ( triangle_has2(triangle,n0,n1) )
	{
	  if ( KNIFE_SUCCESS != 
	       triangle_subtri_index_with_nodes( triangle,n0,n1,
						 &subtri_index ) )
	    {
	      printf("%s: %d: REALLY WRONG!! cannot find subtri\n",
		     __FILE__,__LINE__);
	      return FALSE;
	    }
	  if ( mask_subtri_active(mask,subtri_index ) )
	    {
	      *region = mask_subtri_region(mask,subtri_index);
	      return TRUE;
	    }
	}
      if ( triangle_has2(triangle,n1,n2) )
	{
	  if ( KNIFE_SUCCESS != 
	       triangle_subtri_index_with_nodes( triangle,n1,n2,
						 &subtri_index ) )
	    {
	      printf("%s: %d: REALLY WRONG!! cannot find subtri\n",
		     __FILE__,__LINE__);
	      return FALSE;
	    }
	  if ( mask_subtri_active(mask,subtri_index ) )
	    {
	      *region = mask_subtri_region(mask,subtri_index);
	      return TRUE;
	    }
	}
      if ( triangle_has2(triangle,n2,n0) )
	{
	  if ( KNIFE_SUCCESS != 
	       triangle_subtri_index_with_nodes( triangle,n2,n0,
						 &subtri_index ) )
	    {
	      printf("%s: %d: REALLY WRONG!! cannot find subtri\n",
		     __FILE__,__LINE__);
	      return FALSE;
	    }
	  if ( mask_subtri_active(mask,subtri_index ) )
	    {
	      *region = mask_subtri_region(mask,subtri_index);
	      return TRUE;
	    }
	}
    }

  return FALSE;
}

KNIFE_STATUS poly_paint_surf( Poly poly, Mask surf, Segment segment )
{
  KNIFE_STATUS status;
  Triangle triangle, other_triangle;
  int subtri_index;

  triangle = mask_triangle(surf);

  status = triangle_subtri_index_with_nodes( triangle,
					     segment_node0(segment),
					     segment_node1(segment),
					     &subtri_index );

  if ( KNIFE_NOT_FOUND == status ) return KNIFE_SUCCESS;

  TRY( status, "find subtri with nodes" );
  
  if ( mask_subtri_active(surf,subtri_index) )
    {
      TRY( triangle_neighbor( triangle, segment, &other_triangle), 
	   "other tri" );
      TRY( poly_gather_active_surf( poly, other_triangle,
				    mask_subtri_region(surf,subtri_index)  ), 
	   "gather act surf");
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_gather_active_surf( Poly poly, Triangle triangle,
				      int region )
{
  Mask surf;
  int segment_index;
  Segment segment;
  Triangle other_triangle;

  if ( poly_has_surf_triangle( poly, triangle ) ) return KNIFE_SUCCESS;

  surf = mask_create( triangle, TRUE );

  TRY( mask_deactivate_all_subtri( surf ), "deact");
  TRY( mask_activate_subtri_index( surf, 0, region ), "set region");

  TRY( poly_add_surf(poly,surf), "add surf" );

  for ( segment_index = 0; segment_index < 3; segment_index++ )
    {
      segment = triangle_segment( triangle, segment_index );
      TRY( triangle_neighbor( triangle, segment, &other_triangle), 
	   "other tri" );
      TRY( poly_gather_active_surf( poly, other_triangle, region ), 
	   "rec gather" );
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_mask_surrounding_node_activity( Poly poly, Node node,
						  KnifeBool *active )
{
  int mask_index;
  Mask mask;
  Triangle triangle;
  KnifeBool found;
  KnifeBool found_active;

  found = FALSE;
  found_active = FALSE;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      if ( triangle_has1(triangle,node) &&
	   !triangle_on_boundary(triangle) )
	{
	  found = TRUE;
	  if ( 1 != triangle_nsubtri(triangle) ) 
	    {
	      found_active = TRUE;
	    }
	  else
	    {
	      if ( mask_subtri_active(mask,0) ) found_active = TRUE;
	    }
	}
    }

  if (found) *active = found_active;
  if (!found) printf("%s: %d: no triangles with node found\n",
		     __FILE__,__LINE__);

  return (found?KNIFE_SUCCESS:KNIFE_NOT_FOUND);
}

KNIFE_STATUS poly_regions( Poly poly, int *regions )
{
  int mask_index, subtri_index;
  Mask mask;
  Triangle triangle;

  *regions = EMPTY;
  for ( mask_index = 0; mask_index < poly_nmask(poly); mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	*regions = MAX( *regions, mask_subtri_region(mask,subtri_index) );
    }

  for ( mask_index = 0; mask_index < poly_nsurf(poly); mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	*regions = MAX( *regions, mask_subtri_region(mask,subtri_index) );
    }

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS poly_centroid_volume( Poly poly, int region, double *origin, 
				   double *centroid, double *volume )
{
  int mask_index;

  if (NULL == poly) return KNIFE_NULL;

  centroid[0] = 0.0;
  centroid[1] = 0.0;
  centroid[2] = 0.0;
  (*volume)   = 0.0;

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    TRY(mask_centroid_volume_contribution( poly_mask(poly, mask_index), region,
					   origin, centroid, volume),
	"cent vol mask");

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    TRY(mask_centroid_volume_contribution( poly_surf(poly, mask_index), region,
					   origin, centroid, volume),
	"cent vol surf");

  if ( (*volume) < 1.0e-14 )
    {
      TRY( poly_average_face_center( poly, region, centroid ), "avg face cent");
    }
  else
    {
      centroid[0] = centroid[0] / (*volume) + origin[0];
      centroid[1] = centroid[1] / (*volume) + origin[1];
      centroid[2] = centroid[2] / (*volume) + origin[2];
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_average_face_center( Poly poly, int region, double *centroid )
{
  int mask_index;
  int subtri_index;
  Mask mask;
  Triangle triangle;
  double center[3];
  double contributions;

  centroid[0] = 0.0;
  centroid[1] = 0.0;
  centroid[2] = 0.0;
  contributions = 0.0;

  for ( mask_index = 0; mask_index < poly_nmask(poly); mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	if ( region == mask_subtri_region(mask,subtri_index) )
	  {
	    TRY( subtri_center( triangle_subtri(triangle,subtri_index),center ),
		 "subtri_center");
	    centroid[0] += center[0];
	    centroid[1] += center[1];
	    centroid[2] += center[2];
	    contributions += 1.0;
	  }
    }

  for ( mask_index = 0; mask_index < poly_nsurf(poly); mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      triangle = mask_triangle(mask);
      for ( subtri_index = 0; 
	    subtri_index < triangle_nsubtri(triangle); 
	    subtri_index++)
	if ( region == mask_subtri_region(mask,subtri_index) )
	  {
	    TRY( subtri_center( triangle_subtri(triangle,subtri_index),center ),
		 "subtri_center");
	    centroid[0] += center[0];
	    centroid[1] += center[1];
	    centroid[2] += center[2];
	    contributions += 1.0;
	  }
    }

  if ( contributions < 0.5 ) 
    {
      printf("%s: %d: poly has no subtris in region %d\n",
	     __FILE__,__LINE__,region);
      return KNIFE_DIV_ZERO;
    }

  centroid[0] /= contributions;
  centroid[1] /= contributions;
  centroid[2] /= contributions;

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_nsubtri_between( Poly poly1, int region1, 
				   Poly poly2, int region2,
				   Node node, int *nsubtri )
{
  int mask_index, subtri_index;
  Mask mask1, mask2;
  Triangle triangle;
  int n;

  n = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly1); 
	mask_index++)
    {
      mask1 = poly_mask(poly1,mask_index);
      triangle = mask_triangle(mask1);
      if ( triangle_has1(triangle,node) &&
	   !triangle_on_boundary(triangle) )
	{
	  TRY( poly_mask_with_triangle( poly2, triangle, &mask2 ), "mask2");
	  for ( subtri_index = 0 ; 
		subtri_index < triangle_nsubtri( triangle);
		subtri_index++ )
	    if ( region1 == mask_subtri_region(mask1,subtri_index) &&
		 region2 == mask_subtri_region(mask2,subtri_index) ) n++;
	} /* shared triangle */
    } /* poly1 masks */

  *nsubtri = n;

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_subtri_between( Poly poly1, int region1, 
				  Poly poly2, int region2,
				  Node node, int nsubtri,
				  double *triangle_node0, 
				  double *triangle_node1,
				  double *triangle_node2,
				  double *triangle_normal,
				  double *triangle_area )
{
  int mask_index, subtri_index;
  Mask mask1, mask2;
  Triangle triangle;
  Subtri subtri;
  int n;

  double normal[3], entire_triangle_area, area;

  n = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly1); 
	mask_index++)
    {
      mask1 = poly_mask(poly1,mask_index);
      triangle = mask_triangle(mask1);
      if ( triangle_has1(triangle,node) &&
	   !triangle_on_boundary(triangle) )
	{
	  TRY( poly_mask_with_triangle( poly2, triangle, &mask2 ), "mask2");
	  TRY( triangle_area_normal( triangle, &entire_triangle_area, normal ),
	       "triangle area normal" );
	  if ( mask_inward_pointing_normal( mask1 ) )
	    {
	      normal[0] = -normal[0];
	      normal[1] = -normal[1];
	      normal[2] = -normal[2];
	    }
	  for ( subtri_index = 0 ; 
		subtri_index < triangle_nsubtri( triangle);
		subtri_index++ )
	    if ( region1 == mask_subtri_region(mask1,subtri_index) &&
		 region2 == mask_subtri_region(mask2,subtri_index) ) 
	      {
		if ( n >= nsubtri )
		  {
		    printf("%s: %d: too many subtri found for argument\n",
			   __FILE__,__LINE__);
		    return KNIFE_ARRAY_BOUND;
		  }
		subtri = triangle_subtri( triangle, subtri_index );
		if ( mask_inward_pointing_normal( mask1 ) )
		  {
		    subnode_xyz( subtri_n1(subtri), &(triangle_node0[3*n]) );
		    subnode_xyz( subtri_n0(subtri), &(triangle_node1[3*n]) );
		    subnode_xyz( subtri_n2(subtri), &(triangle_node2[3*n]) );
		  }
		else
		  {
		    subnode_xyz( subtri_n0(subtri), &(triangle_node0[3*n]) );
		    subnode_xyz( subtri_n1(subtri), &(triangle_node1[3*n]) );
		    subnode_xyz( subtri_n2(subtri), &(triangle_node2[3*n]) );
		  }
		triangle_normal[0+3*n]=normal[0];
		triangle_normal[1+3*n]=normal[1];
		triangle_normal[2+3*n]=normal[2];
		area = entire_triangle_area * subtri_reference_area( subtri );
		triangle_area[n] = area;
		n++;
	      }
	} /* shared triangle */
    } /* poly1 masks */

  if ( n != nsubtri )
    {
      printf("%s: %d: not enough subtri found %d of %d\n",
	     __FILE__,__LINE__, n, nsubtri);
      return KNIFE_MISSING;
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_between_sens( Poly poly1, int region1, 
				Poly poly2, int region2,
				Node node, int nsubtri,
				int *parent_int,
				double *parent_xyz, 
				Surface surface )
{
  int mask_index, subtri_index;
  Mask mask1, mask2;
  Triangle triangle;
  Subtri subtri;
  int n;
  int ixyz;
  int subnode_index;
  Intersection intersection;
  Segment segment;
  Subnode subnode;
  Triangle other;
  int triangle_index;

  n = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly1); 
	mask_index++)
    {
      mask1 = poly_mask(poly1,mask_index);
      triangle = mask_triangle(mask1);
      if ( triangle_has1(triangle,node) &&
	   !triangle_on_boundary(triangle) )
	{
	  TRY( poly_mask_with_triangle( poly2, triangle, &mask2 ), "mask2");
	  for ( subtri_index = 0 ; 
		subtri_index < triangle_nsubtri( triangle);
		subtri_index++ )
	    if ( region1 == mask_subtri_region(mask1,subtri_index) &&
		 region2 == mask_subtri_region(mask2,subtri_index) ) 
	      {
		if ( n >= nsubtri )
		  {
		    printf("%s: %d: too many subtri found for argument\n",
			   __FILE__,__LINE__);
		    return KNIFE_ARRAY_BOUND;
		  }
		subtri = triangle_subtri( triangle, subtri_index );
		for ( ixyz = 0 ; ixyz < 3 ; ixyz++ ) 
		  {
		    parent_xyz[ixyz+3*0+9*n]=triangle_xyz0(triangle)[ixyz];
		    parent_xyz[ixyz+3*1+9*n]=triangle_xyz1(triangle)[ixyz];
		    parent_xyz[ixyz+3*2+9*n]=triangle_xyz2(triangle)[ixyz];
		  }
		for ( subnode_index = 0 ; subnode_index < 3 ; subnode_index++ )
		  {
		    if ( mask_inward_pointing_normal( mask1 ) )
		      {
			subnode = 
			  subtri_subnode(subtri,
					 inward_subnode_order[subnode_index]);
		      }
		    else
		      {
			subnode = 
			  subtri_subnode(subtri,
					 outward_subnode_order[subnode_index]);
		      }
		    intersection = subnode_intersection(subnode);
		    if ( NULL == intersection )
		      { /* this subnode is a triangle node, no sensitivity */
			parent_int[0+3*subnode_index+9*n] = 4;
			parent_int[1+3*subnode_index+9*n] = EMPTY;
			parent_int[2+3*subnode_index+9*n] = EMPTY;
		      }
		    else
		      { /* this subnode is an intersection */
			segment = intersection_segment( intersection );
			parent_int[0+3*subnode_index+9*n] = 
			  triangle_segment_index( triangle, segment );
			if ( EMPTY == parent_int[0+3*subnode_index+9*n] )
			  { /* this subnode int has background tri */
			    /* so find cut surface segment nodes */
			    parent_int[0+3*subnode_index+9*n] = 3;
			    parent_int[1+3*subnode_index+9*n] =
			      surface_node_index( surface,
						  segment_node0(segment) );
			    parent_int[2+3*subnode_index+9*n] =
			      surface_node_index( surface,
						  segment_node1(segment) );
			    
			    if ( ( 0 > parent_int[1+3*subnode_index+9*n] ) || 
				 ( surface_nsegment(surface) <=
				   parent_int[1+3*subnode_index+9*n] ) ||
				 (0 > parent_int[2+3*subnode_index+9*n] ) || 
				 ( surface_nsegment(surface) <=
				   parent_int[2+3*subnode_index+9*n] ) )
			      { /* error, the seg nodes are not in cut surf */
				printf("%s: %d: seg node is not in cut surf\n",
				       __FILE__,__LINE__);
				return KNIFE_ARRAY_BOUND;
			      }
			    }
			else
			  { /* this subnode int has background segment */
			    /* so find cut surface triangle index */
			    other = intersection_triangle( intersection );
			    triangle_index = 
			      surface_triangle_index(surface,other);
			    if ( 0 <= triangle_index && 
				 surface_ntriangle(surface) > triangle_index )
			      { /* this subnode int has surf tri */
				parent_int[1+3*subnode_index+9*n] = 
				  triangle_index;
				parent_int[2+3*subnode_index+9*n] = EMPTY;
			      }
			    else
			      { /* error, the tri is not part of cut surf */
				printf("%s: %d: int tri is not in cut surf\n",
				       __FILE__,__LINE__);
				return KNIFE_ARRAY_BOUND;
			      }
			  }
		      }
		  }
		n++;
	      }
	} /* shared triangle */
    } /* poly1 masks */

  if ( n != nsubtri )
    {
      printf("%s: %d: not enough subtri found %d of %d\n",
	     __FILE__,__LINE__, n, nsubtri);
      return KNIFE_MISSING;
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_surface_nsubtri( Poly poly, int region, int *nsubtri )
{
  int surf_index;
  Mask surf;
  Triangle triangle;
  int subtri_index;
  int n;

  n = 0;
  for ( surf_index = 0;
	surf_index < poly_nsurf(poly); 
	surf_index++)
    {
      surf = poly_surf(poly,surf_index);
      triangle = mask_triangle(surf);
      for ( subtri_index = 0 ; 
	    subtri_index < triangle_nsubtri( triangle);
	    subtri_index++ )
	if ( region == mask_subtri_region(surf,subtri_index) ) n++;
    }

  *nsubtri = n;

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_surface_subtri( Poly poly, int region, int nsubtri, 
				  double *triangle_node0, 
				  double *triangle_node1,
				  double *triangle_node2,
				  double *triangle_normal,
				  double *triangle_area,
				  int *triangle_tag )
{
  int surf_index;
  Mask surf;
  Triangle triangle;
  int subtri_index;
  Subtri subtri;
  int n;

  double normal[3], entire_triangle_area, area;

  n = 0;
  for ( surf_index = 0;
	surf_index < poly_nsurf(poly); 
	surf_index++)
    {
      surf = poly_surf(poly,surf_index);
      triangle = mask_triangle(surf);
      TRY( triangle_area_normal( triangle, &entire_triangle_area, normal ),
	   "triangle area normal" );
      if ( mask_inward_pointing_normal( surf ) )
	{
	  normal[0] = -normal[0];
	  normal[1] = -normal[1];
	  normal[2] = -normal[2];
	}
      for ( subtri_index = 0 ; 
	    subtri_index < triangle_nsubtri( triangle);
	    subtri_index++ )
	if ( region == mask_subtri_region(surf,subtri_index) )
	  {
	    if ( n >= nsubtri )
	      {
		printf("%s: %d: too many subtri found for argument\n",
		       __FILE__,__LINE__);
		return KNIFE_ARRAY_BOUND;
	      }
	    subtri = triangle_subtri( triangle, subtri_index );
	    if ( mask_inward_pointing_normal( surf ) )
	      {
		subnode_xyz( subtri_n1(subtri), &(triangle_node0[3*n]) );
		subnode_xyz( subtri_n0(subtri), &(triangle_node1[3*n]) );
		subnode_xyz( subtri_n2(subtri), &(triangle_node2[3*n]) );
	      }
	    else
	      {
		subnode_xyz( subtri_n0(subtri), &(triangle_node0[3*n]) );
		subnode_xyz( subtri_n1(subtri), &(triangle_node1[3*n]) );
		subnode_xyz( subtri_n2(subtri), &(triangle_node2[3*n]) );
	      }
	    triangle_normal[0+3*n]=normal[0];
	    triangle_normal[1+3*n]=normal[1];
	    triangle_normal[2+3*n]=normal[2];
	    area = entire_triangle_area * subtri_reference_area( subtri );
	    triangle_area[n] = area;
	    triangle_tag[n] = triangle_boundary_face_index(triangle);
	    n++;
	  }
    }

  if ( n != nsubtri )
    {
      printf("%s: %d: not enough subtri found %d of %d\n",
	     __FILE__,__LINE__, n, nsubtri);
      return KNIFE_MISSING;
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_surface_sens( Poly poly, int region, int nsubtri, 
				int *constraint_type,
				double *constraint_xyz, 
				Surface surface )
{
  int surf_index;
  Mask surf;
  Triangle triangle, other;
  Subtri subtri;
  int subtri_index;
  Subnode subnode;
  int subnode_index;
  Intersection intersection;
  Segment segment;
  int n, ixyz;

  n = 0;
  for ( surf_index = 0;
	surf_index < poly_nsurf(poly); 
	surf_index++)
    {
      surf = poly_surf(poly,surf_index);
      triangle = mask_triangle(surf);
      for ( subtri_index = 0 ; 
	    subtri_index < triangle_nsubtri( triangle);
	    subtri_index++ )
	if ( region == mask_subtri_region(surf,subtri_index) )
	  {
	    if ( n >= nsubtri )
	      {
		printf("%s: %d: too many subtri found for argument\n",
		       __FILE__,__LINE__);
		return KNIFE_ARRAY_BOUND;
	      }
	    subtri = triangle_subtri( triangle, subtri_index );
	    constraint_type[3+4*n] = surface_triangle_index(surface,triangle);
	    for ( subnode_index = 0 ; subnode_index < 3 ; subnode_index++ )
	      {
		for ( ixyz = 0 ; ixyz < 9 ; ixyz++ )
		  {
		    constraint_xyz[ixyz+9*subnode_index+27*n] = 0.0;
		  }
		  if ( mask_inward_pointing_normal( surf ) )
		    {
		      subnode = 
			subtri_subnode(subtri,
				        inward_subnode_order[subnode_index]);
		    }
		  else
		    {
		      subnode = 
			subtri_subnode(subtri,
				       outward_subnode_order[subnode_index]);
		    }
		/* subnode parent is triangle node [0-2] */
		constraint_type[subnode_index+4*n] = 
		  triangle_node_index(triangle, subnode_node(subnode) );
		if ( EMPTY == constraint_type[subnode_index+4*n] )
		  {
		    intersection = subnode_intersection(subnode);
		    NOT_NULL( intersection,
			      "subnode (without node) intersection NULL");
		    segment = intersection_segment( intersection );
		    constraint_type[subnode_index+4*n] = 
		      triangle_segment_index( triangle, segment );
		    if ( EMPTY == constraint_type[subnode_index+4*n] )
		      {
			/* subnode parent is intersection with triangle [6] */
			constraint_type[subnode_index+4*n] = 6;
			for ( ixyz = 0 ; ixyz < 3 ; ixyz++ )
			  {
			    constraint_xyz[ixyz+0+9*subnode_index+27*n] = 
			      segment_xyz0(segment)[ixyz];
			    constraint_xyz[ixyz+3+9*subnode_index+27*n] = 
			      segment_xyz1(segment)[ixyz];
			    constraint_xyz[ixyz+6+9*subnode_index+27*n] = 
			      segment_xyz0(segment)[ixyz];
			  }
		      }
		    else
		      {
			/* subnode parent is intersection with dual tri [3-5]*/
			constraint_type[subnode_index+4*n] += 3;
			other = intersection_triangle( intersection );
			NOT_NULL( other,
				  "intersection other tri NULL");
			for ( ixyz = 0 ; ixyz < 3 ; ixyz++ )
			  {
			    constraint_xyz[ixyz+0+9*subnode_index+27*n] = 
			      triangle_xyz0(other)[ixyz];
			    constraint_xyz[ixyz+3+9*subnode_index+27*n] = 
			      triangle_xyz1(other)[ixyz];
			    constraint_xyz[ixyz+6+9*subnode_index+27*n] = 
			      triangle_xyz2(other)[ixyz];
			  }
		      }
		  }
	      }
	    n++;
	  }
    }

  if ( n != nsubtri )
    {
      printf("%s: %d: not enough subtri found %d of %d\n",
	     __FILE__,__LINE__, n, nsubtri);
      return KNIFE_MISSING;
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_boundary_nsubtri( Poly poly, int face_index, int region, 
				    int *nsubtri )
{
  int mask_index;
  Mask mask;
  Triangle triangle;
  int subtri_index;
  int n;

  n = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      if ( face_index == triangle_boundary_face_index(triangle) )
	for ( subtri_index = 0 ; 
	      subtri_index < triangle_nsubtri( triangle);
	      subtri_index++ )
	  if ( region == mask_subtri_region(mask,subtri_index) ) n++;
    }

  *nsubtri = n;

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_boundary_subtri( Poly poly, int face_index, int region, 
				   int nsubtri, 
				   double *triangle_node0, 
				   double *triangle_node1,
				   double *triangle_node2,
				   double *triangle_normal,
				   double *triangle_area )
{
  int mask_index;
  Mask mask;
  Triangle triangle;
  int subtri_index;
  Subtri subtri;
  int n;

  double normal[3], entire_triangle_area, area;

  n = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      TRY( triangle_area_normal( triangle, &entire_triangle_area, normal ),
	   "triangle area normal" );
      if ( mask_inward_pointing_normal( mask ) )
	{
	  normal[0] = -normal[0];
	  normal[1] = -normal[1];
	  normal[2] = -normal[2];
	}
      if ( face_index == triangle_boundary_face_index(triangle) )
	for ( subtri_index = 0 ; 
	      subtri_index < triangle_nsubtri( triangle);
	      subtri_index++ )
	  if ( region == mask_subtri_region(mask,subtri_index) )
	    {
	      if ( n >= nsubtri )
		{
		  printf("%s: %d: too many subtri found for argument\n",
			 __FILE__,__LINE__);
		  return KNIFE_ARRAY_BOUND;
		}
	      subtri = triangle_subtri( triangle, subtri_index );
	      if ( mask_inward_pointing_normal( mask ) )
		{
		  subnode_xyz( subtri_n1(subtri), &(triangle_node0[3*n]) );
		  subnode_xyz( subtri_n0(subtri), &(triangle_node1[3*n]) );
		  subnode_xyz( subtri_n2(subtri), &(triangle_node2[3*n]) );
		}
	      else
		{
		  subnode_xyz( subtri_n0(subtri), &(triangle_node0[3*n]) );
		  subnode_xyz( subtri_n1(subtri), &(triangle_node1[3*n]) );
		  subnode_xyz( subtri_n2(subtri), &(triangle_node2[3*n]) );
		}
	      triangle_normal[0+3*n]=normal[0];
	      triangle_normal[1+3*n]=normal[1];
	      triangle_normal[2+3*n]=normal[2];
	      area = entire_triangle_area * subtri_reference_area( subtri );
	      triangle_area[n] = area;
	      n++;
	    } /* face_index subtri_index region */
    } /* mask_index */

  if ( n != nsubtri )
    {
      printf("%s: %d: not enough subtri found %d of %d\n",
	     __FILE__,__LINE__, n, nsubtri);
      return KNIFE_MISSING;
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_boundary_sens( Poly poly, int face_index, int region, 
				 int nsubtri, 
				 int *parent_int,
				 double *parent_xyz, 
				 Surface surface )

{
  int n;
  int ixyz;
  int mask_index;
  Mask mask;
  int triangle_index;
  Triangle triangle, other;
  int subtri_index;
  Subtri subtri;
  int subnode_index;
  Subnode subnode;
  Intersection intersection;
  Segment segment;

  n = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      if ( face_index == triangle_boundary_face_index(triangle) )
	for ( subtri_index = 0 ; 
	      subtri_index < triangle_nsubtri( triangle);
	      subtri_index++ )
	  if ( region == mask_subtri_region(mask,subtri_index) )
	    {
	      if ( n >= nsubtri )
		{
		  printf("%s: %d: too many subtri found for argument\n",
			 __FILE__,__LINE__);
		  return KNIFE_ARRAY_BOUND;
		}
	      subtri = triangle_subtri( triangle, subtri_index );
	      for ( ixyz = 0 ; ixyz < 3 ; ixyz++ ) 
		{
		  parent_xyz[ixyz+3*0+9*n]=triangle_xyz0(triangle)[ixyz];
		  parent_xyz[ixyz+3*1+9*n]=triangle_xyz1(triangle)[ixyz];
		  parent_xyz[ixyz+3*2+9*n]=triangle_xyz2(triangle)[ixyz];
		}

	      for ( subnode_index = 0 ; subnode_index < 3 ; subnode_index++ )
		{
		  if ( mask_inward_pointing_normal( mask ) )
		    {
		      subnode = 
			subtri_subnode(subtri,
				        inward_subnode_order[subnode_index]);
		    }
		  else
		    {
		      subnode = 
			subtri_subnode(subtri,
				       outward_subnode_order[subnode_index]);
		    }
		  intersection = subnode_intersection(subnode);
		  if ( NULL == intersection )
		    { /* this subnode is a triangle node, no sensitivity */
		      parent_int[0+3*subnode_index+9*n] = 4;
		      parent_int[1+3*subnode_index+9*n] = EMPTY;
		      parent_int[2+3*subnode_index+9*n] = EMPTY;
		    }
		  else
		    { /* this subnode is an intersection */
		      segment = intersection_segment( intersection );
		      parent_int[0+3*subnode_index+9*n] = 
			triangle_segment_index( triangle, segment );
		      if ( EMPTY == parent_int[0+3*subnode_index+9*n] )
			{ /* this subnode int has background tri */
			  /* so find cut surface segment nodes */
			  parent_int[0+3*subnode_index+9*n] = 3;
			  parent_int[1+3*subnode_index+9*n] =
			    surface_node_index( surface,
						segment_node0(segment) );
			  parent_int[2+3*subnode_index+9*n] =
			    surface_node_index( surface,
						segment_node1(segment) );
			    
			  if ( ( 0 > parent_int[1+3*subnode_index+9*n] ) || 
			       ( surface_nsegment(surface) <=
				 parent_int[1+3*subnode_index+9*n] ) ||
			       (0 > parent_int[2+3*subnode_index+9*n] ) || 
			       ( surface_nsegment(surface) <=
				 parent_int[2+3*subnode_index+9*n] ) )
			    { /* error, the seg nodes are not in cut surf */
			      printf("%s: %d: seg node is not in cut surf\n",
				     __FILE__,__LINE__);
			      return KNIFE_ARRAY_BOUND;
			    }
			}
		      else
			{ /* this subnode int has background segment */
			  /* so find cut surface triangle index */
			  other = intersection_triangle( intersection );
			  triangle_index = 
			    surface_triangle_index(surface,other);
			  if ( 0 <= triangle_index && 
			       surface_ntriangle(surface) > triangle_index )
			    { /* this subnode int has surf tri */
			      parent_int[1+3*subnode_index+9*n] = 
				triangle_index;
			      parent_int[2+3*subnode_index+9*n] = EMPTY;
			    }
			  else
			    { /* error, the tri is not part of cut surf */
			      printf("%s: %d: int tri is not in cut surf\n",
				     __FILE__,__LINE__);
			      return KNIFE_ARRAY_BOUND;
			    }
			}
		    } 
		} /* subnode_index */
	      n++;
	    }  /* face_index subtri_index region */
    } /* mask_index */

  if ( n != nsubtri )
    {
      printf("%s: %d: not enough subtri found %d of %d\n",
	     __FILE__,__LINE__, n, nsubtri);
      return KNIFE_MISSING;
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_boundary_face_geometry( Poly poly, int face_index, FILE *f )
{
  int mask_index;
  Mask mask;
  Triangle triangle;
  int nsubtri;

  nsubtri = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      if ( face_index == triangle_boundary_face_index(triangle) )
	{
	  nsubtri += mask_nsubtri( mask );
	}
    }

  fprintf(f,"%d\n",nsubtri);

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly,mask_index);
      triangle = mask_triangle(mask);
      if ( face_index == triangle_boundary_face_index(triangle) )
	{
	  TRY( mask_dump_geom( mask, f),
	       "mask_dump_geom" );
	}
    }

  return KNIFE_SUCCESS;
}
KNIFE_STATUS poly_surf_geometry( Poly poly, FILE *f )
{
  int surf_index;
  int nsubtri;

  nsubtri = 0;
  for ( surf_index = 0;
	surf_index < poly_nsurf(poly); 
	surf_index++)
    nsubtri += mask_nsubtri( poly_surf(poly,surf_index) );

  fprintf(f,"%d\n",nsubtri);

  for ( surf_index = 0;
	surf_index < poly_nsurf(poly); 
	surf_index++)
    {
      TRY( mask_dump_geom( poly_surf(poly,surf_index), f), "mask_dump_geom" );
    }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS poly_tecplot( Poly poly )
{
  char filename[1025];
  FILE *f;

  if ( NULL == poly ) return KNIFE_NULL;

  poly_tecplot_frame++;

  sprintf(filename, "poly%08d.t",poly_tecplot_frame );
  printf("producing %s\n",filename);

  f = fopen(filename, "w");
  
  fprintf(f,"title=poly_geometry\nvariables=x,y,z,r\n");

  poly_tecplot_zone( poly, f );

  fclose(f);

  return KNIFE_SUCCESS;

}

KNIFE_STATUS poly_tecplot_zone( Poly poly, FILE *f )
{
  Mask mask;
  int mask_index;
  int nsubtri;
  int subtri_index;

  nsubtri = 0;
  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      nsubtri += mask_nsubtri( mask );
    }

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      nsubtri += mask_nsubtri( mask );
    }

  if (0==nsubtri) return KNIFE_SUCCESS;

  fprintf(f, "zone t=poly, i=%d, j=%d, f=fepoint, et=triangle\n",
	  3*nsubtri, nsubtri );

  for ( mask_index = 0;
	mask_index < poly_nmask(poly); 
	mask_index++)
    {
      mask = poly_mask(poly, mask_index);
      mask_dump_geom(mask, f );
    }

  for ( mask_index = 0;
	mask_index < poly_nsurf(poly); 
	mask_index++)
    {
      mask = poly_surf(poly, mask_index);
      mask_dump_geom(mask, f );
    }

  for ( subtri_index = 0;
	subtri_index < nsubtri; 
	subtri_index++)
    fprintf(f, "%6d %6d %6d\n",
	    1+3*subtri_index,
	    2+3*subtri_index,
	    3+3*subtri_index );

  return KNIFE_SUCCESS;
}
