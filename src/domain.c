
/* domain for PDE solvers */

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
#include "domain.h"
#include "cut.h"
#include "near.h"

Domain domain_create( Primal primal, Surface surface)
{
  Domain domain;

  domain = (Domain) malloc( sizeof(DomainStruct) );
  if (NULL == domain) {
    printf("%s: %d: malloc failed in domain_create\n",__FILE__,__LINE__);
    return NULL;
  }

  domain->primal = primal;
  domain->surface = surface;

  domain->npoly = EMPTY;
  domain->poly = NULL;

  domain->ntriangle = EMPTY;
  domain->triangle = NULL;

  return domain;
}

void domain_free( Domain domain )
{
  if ( NULL == domain ) return;
  
  if ( NULL != domain->poly ) free(domain->poly);
  if ( NULL != domain->triangle ) free(domain->triangle);

  free(domain);
}

#define TRY(fcn,msg)					      \
  {							      \
    int code;						      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      return code;					      \
    }							      \
  }

KNIFE_STATUS domain_tetrahedral_elements( Domain domain )
{
  int node;
  double xyz[3];
  int edge;
  int edge_nodes[2];
  int tri;
  int tri_nodes[3];
  int edge0, edge1, edge2;

  domain->npoly = primal_ncell(domain->primal);
  domain->poly = (PolyStruct *)malloc(domain->npoly * sizeof(PolyStruct));
  domain_test_malloc(domain->poly,
		     "domain_tetrahedral_elements poly");

  domain->nnode = primal_nnode(domain->primal);
  domain->node = (NodeStruct *)malloc( domain->nnode * 
				       sizeof(NodeStruct));
  domain_test_malloc(domain->node,
		     "domain_tetrahedral_elements node");
  for (node = 0 ; node < primal_nnode(domain->primal) ; node++)
    {
      TRY( primal_xyz( domain->primal, node, xyz), "xyz" );
      node_initialize( domain_node(domain,node), xyz, node);
    }

  domain->nsegment = primal_nedge(domain->primal);
  domain->segment = (SegmentStruct *)malloc( domain->nsegment * 
					       sizeof(SegmentStruct));
  domain_test_malloc(domain->segment,
		     "domain_tetrahedral_elements segment");
  for (edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      TRY( primal_edge( domain->primal, edge, edge_nodes), "primal_edge" );
      segment_initialize( domain_segment(domain,edge),
			  domain_node(domain,edge_nodes[0]),
			  domain_node(domain,edge_nodes[1]));
    }

  domain->ntriangle = primal_ntri(domain->primal);
  domain->triangle = (TriangleStruct *)malloc( domain->ntriangle * 
					       sizeof(TriangleStruct));
  domain_test_malloc(domain->triangle,
		     "domain_tetrahedral_elements triangle");
  for (tri = 0 ; tri < primal_ntri(domain->primal) ; tri++)
    {
      TRY( primal_tri( domain->primal, tri, tri_nodes), "tri" );
      TRY( primal_find_edge( domain->primal, 
			     tri_nodes[0], tri_nodes[1],
			     &edge2 ), "edge2" );
      TRY( primal_find_edge( domain->primal, 
			     tri_nodes[1], tri_nodes[2],
			     &edge0 ), "edge0" );
      TRY( primal_find_edge( domain->primal, 
			     tri_nodes[2], tri_nodes[0],
			     &edge1 ), "edge1" );
      triangle_initialize( domain_triangle(domain,tri),
			   domain_segment(domain,edge0),
			   domain_segment(domain,edge1),
			   domain_segment(domain,edge2) );
    }

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_dual_elements( Domain domain )
{
  int node;
  int cell, edge, tri, face;
  int side;
  int cell_center, tri_center, edge_center, face_center;
  int edge_index, segment_index, triangle_index;
  int tri_nodes[3];
  int face_nodes[3];
  double xyz[3];
  int cell_edge;
  int segment0, segment1, segment2;
  
  domain->npoly = primal_nnode(domain->primal);
  domain->poly = (PolyStruct *)malloc(domain->npoly * sizeof(PolyStruct));
  domain_test_malloc(domain->poly,"domain_dual_elements poly");
  
  domain->nnode = 
    primal_ncell(domain->primal) +
    primal_ntri(domain->primal) +
    primal_nedge(domain->primal);
  domain->node = (NodeStruct *)malloc( domain->nnode * 
				       sizeof(NodeStruct));
  domain_test_malloc(domain->node,
		     "domain_tetrahedral_elements node");
  printf("number of dual nodes in the volume %d\n",domain->nnode);
  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      node = cell;
      primal_cell_center( domain->primal, cell, xyz);
      node_initialize( domain_node(domain,node), xyz, node);
    }
  for ( tri = 0 ; tri < primal_ntri(domain->primal) ; tri++)
    {
      node = tri + primal_ncell(domain->primal);
      primal_tri_center( domain->primal, tri, xyz);
      node_initialize( domain_node(domain,node), xyz, node);
    }
  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      node = edge + primal_ntri(domain->primal) + primal_ncell(domain->primal);
      primal_edge_center( domain->primal, edge, xyz);
      node_initialize( domain_node(domain,node), xyz, node);
    }

  domain->nsegment = 
    10 * primal_ncell(domain->primal) +
    3  * primal_ntri(domain->primal) +
    3  * primal_nface(domain->primal);
  domain->segment = (SegmentStruct *)malloc( domain->nsegment * 
					       sizeof(SegmentStruct));
  domain_test_malloc(domain->segment,
		     "domain_tetrahedral_elements segment");
  printf("number of dual segments in the volume %d\n",domain->nsegment);

  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      cell_center = cell;
      for ( side = 0 ; side < 4 ; side++)
	{
	  tri = primal_c2t(domain->primal,cell,side);
	  tri_center = tri + primal_ncell(domain->primal);
	  segment_index = side + 10 * cell;
	  segment_initialize( domain_segment(domain,segment_index),
			      domain_node(domain,cell_center),
			      domain_node(domain,tri_center));
	}
      for ( edge = 0 ; edge < 6 ; edge++)
	{
	  edge_index = primal_c2e(domain->primal,cell,edge);
	  edge_center = edge_index + primal_ntri(domain->primal) 
	                           + primal_ncell(domain->primal);
	  segment_index = edge + 4 + 10 * cell;
	  segment_initialize( domain_segment(domain,segment_index),
			      domain_node(domain,cell_center),
			      domain_node(domain,edge_center));
	}
    }

  for ( tri = 0 ; tri < primal_ntri(domain->primal) ; tri++)
    {
      tri_center = tri + primal_ncell(domain->primal);;
      primal_tri(domain->primal,tri,tri_nodes);
      for ( side = 0 ; side < 3 ; side++)
	{
	  primal_find_edge( domain->primal, 
			    tri_nodes[primal_face_side_node0(side)], 
			    tri_nodes[primal_face_side_node1(side)], 
			    &edge_index );
	  edge_center = edge_index + primal_ntri(domain->primal) 
	                           + primal_ncell(domain->primal);
	  segment_index = side + 3 * tri + 10 * primal_ncell(domain->primal);
	  segment_initialize( domain_segment(domain,segment_index),
			      domain_node(domain,tri_center),
			      domain_node(domain,edge_center));
	}
    }

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++)
    {
      face_center = face + primal_ncell(domain->primal);;
      primal_face(domain->primal,face,face_nodes);
      for ( side = 0 ; side < 3 ; side++)
	{
	  primal_find_edge( domain->primal, 
			    face_nodes[primal_face_side_node0(side)], 
			    face_nodes[primal_face_side_node1(side)], 
			    &edge_index );
	  edge_center = edge_index + primal_nface(domain->primal) 
	                           + primal_ncell(domain->primal);
	  segment_index = side +  3 * face 
                               +  3 * primal_ntri(domain->primal)
	                       + 10 * primal_ncell(domain->primal);
	  segment_initialize( domain_segment(domain,segment_index),
			      domain_node(domain,face_center),
			      domain_node(domain,edge_center));
	}
    }


  domain->ntriangle = 12*primal_ncell(domain->primal);
  domain->triangle = (TriangleStruct *)malloc( domain->ntriangle * 
					       sizeof(TriangleStruct));
  domain_test_malloc(domain->triangle,"domain_dual_elements triangle");
  printf("number of dual triangles in the volume %d\n",domain->ntriangle);

  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      for ( side = 0 ; side < 4 ; side++)
	{
	  segment0 = side + 10 * cell;
	  tri = primal_c2t(domain->primal,cell,side);
	  primal_tri(domain->primal,tri,tri_nodes);
	  for ( edge = 0 ; edge < 3 ; edge++)
	    {
	      segment1 = edge + 3 * tri + 10 * primal_ncell(domain->primal);
	      TRY( primal_find_edge( domain->primal, 
				     tri_nodes[primal_face_side_node0(edge)], 
				     tri_nodes[primal_face_side_node1(edge)], 
				     &edge_index ), "dual int tri find edge" );
	      TRY( primal_find_cell_edge( domain->primal, cell, edge_index,
					  &cell_edge ), "dual int tri ce");
	      segment2 = cell_edge + 4 + 10 * cell;
	      triangle_index = edge + 3 * side + 12 * cell;
	      triangle_initialize( domain_triangle(domain,triangle_index),
				   domain_segment(domain,segment0),
				   domain_segment(domain,segment1),
				   domain_segment(domain,segment2));
	    }
	}
    }

  printf("dual completed\n");

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_boolean_subtract( Domain domain )
{
  int triangle_index;
  int i;
  NearStruct *near_tree;
  double center[3], diameter;
  int max_touched, ntouched;
  int *touched;
  NearStruct target;
  KNIFE_STATUS code;

  near_tree = (NearStruct *)malloc( surface_ntriangle(domain->surface) * 
				    sizeof(NearStruct));
  for (triangle_index=0;
       triangle_index<surface_ntriangle(domain->surface);
       triangle_index++)
    {
      triangle_extent(surface_triangle(domain->surface,triangle_index),
		      center, &diameter);
      near_initialize( &(near_tree[triangle_index]), 
		       triangle_index, 
		       center[0], center[1], center[2], 
		       diameter );
      if (triangle_index > 0) near_insert( near_tree,
					   &(near_tree[triangle_index]) );
    }

  max_touched = surface_ntriangle(domain->surface);

  touched = (int *) malloc( max_touched * sizeof(int) );

  for ( triangle_index = 0;
	triangle_index < domain_ntriangle(domain); 
	triangle_index++)
    {
      triangle_extent(domain_triangle(domain,triangle_index),
		      center, &diameter);
      near_initialize( &target, 
		       EMPTY, 
		       center[0], center[1], center[2], 
		       diameter );
      ntouched = 0;
      near_touched(near_tree, &target, &ntouched, max_touched, touched);
      for (i=0;i<ntouched;i++)
	{
	  cut_between( domain_triangle(domain,triangle_index),
		       surface_triangle(domain->surface,touched[i]) );
	}
    }

  free(touched);

  code = domain_triangulate(domain);
  if (KNIFE_SUCCESS != code)
    {
      printf("%s: %d: domain_triangulate returned %d\n",
	     __FILE__,__LINE__,code);
      return code;
    }

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_triangulate( Domain domain )
{
  KNIFE_STATUS code;
  int triangle_index;

  code = surface_triangulate(domain->surface);
  if (KNIFE_SUCCESS != code)
    {
      printf("%s: %d: surface_triangulate returned %d\n",
	     __FILE__,__LINE__,code);
      return code;
    }

  for ( triangle_index = 0;
	triangle_index < domain_ntriangle(domain); 
	triangle_index++)
    {
      code = triangle_triangulate_cuts( domain_triangle(domain,
							triangle_index) );
      if (KNIFE_SUCCESS != code)
	{
	  printf("%s: %d: triangle_triangulate_cuts returned %d\n",
		 __FILE__,__LINE__,code);
	  return code;
	}
    }

  return KNIFE_SUCCESS;
}
