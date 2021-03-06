
/* domain for PDE solvers */

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

#include <stdlib.h>
#include <stdio.h>
#include "domain.h"
#include "cut.h"
#include "near.h"
#include "logger.h"

#define DOMAIN_LOGGER_LEVEL (0)

#define TRY(fcn,msg)					      \
  {							      \
    KNIFE_STATUS code;					      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      return code;					      \
    }							      \
  }

#define TRYN(fcn,msg)					      \
  {							      \
    KNIFE_STATUS code;					      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      return NULL;					      \
    }							      \
  }

#define NOT_NULL(pointer,msg)				      \
  if (NULL == (pointer)) {				      \
    printf("%s: %d: %s\n",__FILE__,__LINE__,(msg));	      \
    return KNIFE_NULL;					      \
  }

#define NOT_NULLN(pointer,msg)				      \
  if (NULL == (pointer)) {				      \
    printf("%s: %d: %s\n",__FILE__,__LINE__,(msg));	      \
    return NULL;					      \
  }

Domain domain_create( Primal primal, Surface surface)
{
  Domain domain;

  domain = (Domain) malloc( sizeof(DomainStruct) );
  NOT_NULLN(domain,"malloc failed in domain_create");

  domain->primal = primal;
  domain->surface = surface;

  domain->nnode = EMPTY;
  domain->node = NULL;

  domain->nsegment = EMPTY;
  domain->segment = NULL;

  domain->ntriangle = EMPTY;
  domain->triangle = NULL;

  domain->npoly = EMPTY;
  domain->poly = NULL;

  domain->topo = NULL;

  domain->nside = EMPTY;
  domain->f2s = NULL;
  domain->s2fs = NULL;

  return domain;
}

void domain_free( Domain domain )
{
  int i;
  if ( NULL == domain ) return;

  /* FIXME find a consistant way to free cuts and intersections */
  /* segments have list of intersections */
  /* triangles have list of cuts */

  if ( NULL != domain->node )
    {
      for ( i = 0 ; i < domain->nnode ; i++ ) 
	node_free(domain->node[i]);
      free( domain->node );
    }

  if ( NULL != domain->segment )
    {
      for ( i = 0 ; i < domain->nsegment ; i++ ) 
	segment_free(domain->segment[i]);
      free( domain->segment );
    }

  if ( NULL != domain->triangle )
    {
      for ( i = 0 ; i < domain->ntriangle ; i++ ) 
	triangle_free(domain->triangle[i]);
      free( domain->triangle );
    }

  if ( NULL != domain->poly )
    {
      for ( i = 0 ; i < domain->npoly ; i++ ) 
	poly_free(domain->poly[i]);
      free( domain->poly );
    }

  if ( NULL != domain->topo ) free( domain->topo );
  
  if ( NULL != domain->f2s )  free( domain->f2s );
  if ( NULL != domain->s2fs ) free( domain->s2fs );

  free(domain);
}

Node domain_node( Domain domain, int node_index )
{
  int cell, tri, edge;
  int surface_node, volume_node;
  double xyz[3];

  if ( 0 > node_index || domain_nnode(domain) <= node_index ) 
    {
      printf("%s: %d: domain_node array bound error %d\n",
	     __FILE__,__LINE__, node_index);
      return NULL;
    }

  if (NULL == domain->node[node_index])
    {
      if ( node_index < primal_ncell(domain->primal) )
	{
	  cell = node_index;
	  TRYN( primal_cell_center( domain->primal, cell, xyz), "cell center" );
	  domain->node[node_index] = node_create( xyz );
	  NOT_NULLN(domain->node[node_index],"node_create NULL");
	  return domain->node[node_index];
	}
      if ( node_index < primal_ncell(domain->primal) 
	              + primal_ntri(domain->primal) )
	{
	  tri = node_index - primal_ncell(domain->primal);
	  TRYN( primal_tri_center( domain->primal, tri, xyz), "tri center" );
	  domain->node[node_index] = node_create( xyz );
	  NOT_NULLN(domain->node[node_index],"node_create NULL");
	  return domain->node[node_index];
	}
      if ( node_index < primal_ncell(domain->primal) 
	              + primal_ntri(domain->primal) 
	              + primal_nedge(domain->primal) )
	{
	  edge = node_index - primal_ncell(domain->primal) 
	                    - primal_ntri(domain->primal);
	  TRYN( primal_edge_center( domain->primal, edge, xyz), "edge center" );
	  domain->node[node_index] = node_create( xyz );
	  NOT_NULLN(domain->node[node_index],"node_create NULL");
	  return domain->node[node_index];
	}
      if ( node_index < primal_ncell(domain->primal) 
	              + primal_ntri(domain->primal) 
   	              + primal_nedge(domain->primal) 
	              + primal_surface_nnode(domain->primal) )
	{
	  surface_node = node_index - primal_ncell(domain->primal) 
	                            - primal_ntri(domain->primal)
                                    - primal_nedge(domain->primal);
	  volume_node = primal_surface_volume_node(domain->primal,surface_node);
	  TRYN( primal_xyz(domain->primal,volume_node,xyz), "surf node xyz");
	  domain->node[node_index] = node_create( xyz );
	  NOT_NULLN(domain->node[node_index],"node_create NULL");
	  return domain->node[node_index];
	}
      printf("%s: %d: array bound error %d\n",
	     __FILE__,__LINE__, node_index);
      return NULL;
    }

  return domain->node[node_index];
}

Node domain_node_at_edge_center( Domain domain, int edge_index )
{
  int node_index;
  Node node;

  node_index = edge_index + primal_ntri(domain->primal) 
                          + primal_ncell(domain->primal);

  node = domain_node( domain, node_index );
  NOT_NULLN( node, "domain_node_at_edge_center" );

  return node;
}

Segment domain_segment( Domain domain, int segment_index )
{
  int cell, side, edge, tri;
  int edge_index;
  int tri_center, edge_center, cell_center;
  int tri_nodes[3];
  int face, node, face_nodes[4], node_index;
  int face_side, node0, node1;

  if ( 0 > segment_index || domain_nsegment(domain) <= segment_index ) 
    {
      printf("%s: %d: domain_segment array bound error %d\n",
	     __FILE__,__LINE__, segment_index);
      return NULL;
    }

  if (NULL == domain->segment[segment_index])
    {
      if ( segment_index < 10*primal_ncell(domain->primal) )
	{
	  cell = segment_index/10;
	  cell_center = cell;
	  side = segment_index - 10*cell;
	  if ( side < 4 )
	    {
	      tri = primal_c2t(domain->primal,cell,side);
	      tri_center = tri + primal_ncell(domain->primal);
	      domain->segment[segment_index] = 
		segment_create( domain_node(domain,cell_center),
				domain_node(domain,tri_center));
		NOT_NULLN(domain->segment[segment_index],"segment_create NULL");
	    }
	  else
	    {
	      edge = side - 4;
	      edge_index = primal_c2e(domain->primal,cell,edge);
	      edge_center = edge_index + primal_ntri(domain->primal) 
		                       + primal_ncell(domain->primal);
	      domain->segment[segment_index] = 
		segment_create( domain_node(domain,cell_center),
				domain_node(domain,edge_center) );
	      NOT_NULLN(domain->segment[segment_index],"segment_create NULL");
	    }
	  return domain->segment[segment_index];
	}
      if ( segment_index < 10*primal_ncell(domain->primal) 
	                 +  3*primal_ntri(domain->primal) )
	{
	  tri = ( segment_index - 10 * primal_ncell(domain->primal) ) / 3;
	  tri_center = tri + primal_ncell(domain->primal);
	  TRYN( primal_tri(domain->primal,tri,tri_nodes), "primal_tri" );
	  side = segment_index - 3*tri - 10 * primal_ncell(domain->primal);
	  TRYN( primal_find_edge( domain->primal, 
				  tri_nodes[primal_face_side_node0(side)], 
				  tri_nodes[primal_face_side_node1(side)], 
				  &edge_index ), "tri seg find edge" );
	  edge_center = edge_index + primal_ntri(domain->primal) 
	                           + primal_ncell(domain->primal);

	  domain->segment[segment_index] = 
	    segment_create( domain_node(domain,tri_center),
			    domain_node(domain,edge_center) );
	  NOT_NULLN(domain->segment[segment_index],"segment_create NULL");
	  return domain->segment[segment_index];
	}
      if ( segment_index < 10*primal_ncell(domain->primal) 
	                 +  3*primal_ntri(domain->primal) 
	                 +  3*primal_nface(domain->primal))
	{
	  face = ( segment_index - 3  * primal_ntri(domain->primal) 
	                         - 10 * primal_ncell(domain->primal)) / 3;
	  node = segment_index - 3  * face 
                               - 3  * primal_ntri(domain->primal)
                               - 10 * primal_ncell(domain->primal); 
	  primal_face(domain->primal, face, face_nodes);
	  TRYN( primal_find_tri( domain->primal, 
				 face_nodes[0], face_nodes[1], face_nodes[2],
				 &tri ), "find tri for face" );
	  tri_center = tri + primal_ncell(domain->primal);;
	  node_index =
	    primal_surface_node(domain->primal,face_nodes[node]) + 
	    primal_nedge(domain->primal) + 
	    primal_ntri(domain->primal) + 
	    primal_ncell(domain->primal);
	  domain->segment[segment_index] = 
	    segment_create( domain_node(domain,tri_center),
			    domain_node(domain,node_index) );
	  NOT_NULLN(domain->segment[segment_index],"segment_create NULL");
	  return domain->segment[segment_index];
	}
      if ( segment_index < 10*primal_ncell(domain->primal) 
	                 +  3*primal_ntri(domain->primal) 
	                 +  3*primal_nface(domain->primal)
	                 +  2*domain->nside )
	{
	  face_side = ( segment_index - 10*primal_ncell(domain->primal) 
                                      - 3*primal_ntri(domain->primal) 
                                      - 3*primal_nface(domain->primal) )/2;
	  face = domain->s2fs[0+2*face_side];
	  side = domain->s2fs[1+2*face_side];
	  primal_face(domain->primal, face, face_nodes);
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];
	  TRYN( primal_find_edge( domain->primal, node0, node1, 
				  &edge_index ), "face seg find edge" );
	  edge_center = edge_index + primal_ntri(domain->primal) 
		+ primal_ncell(domain->primal);
	  if ( 0 == segment_index - 2*face_side - 
	       10 * primal_ncell(domain->primal) -
	       3  * primal_ntri(domain->primal) -
	       3  * primal_nface(domain->primal) )
	    {
	      node_index = 
		primal_surface_node(domain->primal,node0) +
		primal_nedge(domain->primal) + 
		primal_ntri(domain->primal) + 
		primal_ncell(domain->primal);
	      domain->segment[segment_index] = 
		segment_create( domain_node(domain,node_index),
				domain_node(domain,edge_center) );
	      
	    }
	  else
	    {
	      node_index = 
		primal_surface_node(domain->primal,node1) +
		primal_nedge(domain->primal) + 
		primal_ntri(domain->primal) + 
		primal_ncell(domain->primal);
	      domain->segment[segment_index] = 
		segment_create( domain_node(domain,edge_center),
				domain_node(domain,node_index) );
	    }
	  NOT_NULLN(domain->segment[segment_index],"segment_create NULL");
	  return domain->segment[segment_index];	  
	}
      printf("%s: %d: array bound error %d\n",
	     __FILE__,__LINE__, segment_index);
      return NULL;
    }

  return domain->segment[segment_index];
}

Triangle domain_triangle( Domain domain, int triangle_index )
{
  int cell, tri;
  int cell_edge, cell_side, tri_side;
  int cell_nodes[4];
  int node0, node1;
  int segment0, segment1, segment2;
  int face, side;
  int other_face, other_side;
  int face_nodes[4];
  int first_side;

  if ( 0 > triangle_index || domain_ntriangle(domain) <= triangle_index ) 
    {
      printf("%s: %d: domain_triangle array bound error %d\n",
	     __FILE__,__LINE__, triangle_index);
      return NULL;
    }

  if (NULL == domain->triangle[triangle_index])
    {
      if ( triangle_index < 12*primal_ncell(domain->primal) )
	{
	  cell = triangle_index/12;
	  cell_edge = (triangle_index - 12*cell)/2;
	  TRYN( primal_cell(domain->primal,cell,cell_nodes), "primal_cell");
	  node0 = cell_nodes[primal_cell_edge_node0(cell_edge)];
	  node1 = cell_nodes[primal_cell_edge_node1(cell_edge)];
	  if ( 0 == triangle_index - 2 * cell_edge - 12 * cell )
	    {
	      cell_side = primal_cell_edge_left_side(cell_edge);
	      tri = primal_c2t(domain->primal,cell,cell_side);
	      TRYN( primal_find_tri_side( domain->primal, tri, node0, node1,
					  &tri_side ), "find lf tri side");
	      segment0 = cell_side + 10 * cell;
	      segment1 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	      segment2 = cell_edge + 4 + 10 * cell;
	    }
	  else
	    {
	      cell_side = primal_cell_edge_right_side(cell_edge);
	      tri = primal_c2t(domain->primal,cell,cell_side);
	      TRYN( primal_find_tri_side( domain->primal, tri, node0, node1,
					  &tri_side ), "find rt tri side");
	      segment0 = cell_side + 10 * cell;
	      segment1 = cell_edge + 4 + 10 * cell;
	      segment2 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	    }
	  domain->triangle[triangle_index] = 
	    triangle_create( domain_segment(domain,segment0),
			     domain_segment(domain,segment1),
			     domain_segment(domain,segment2), EMPTY );
	  
	  NOT_NULLN(domain->triangle[triangle_index],"triangle_create NULL");
	  return domain->triangle[triangle_index];
	}
      if ( triangle_index < 12*primal_ncell(domain->primal) 
                          +  6*primal_nface(domain->primal) )
	{
	  face = ( triangle_index - 12*primal_ncell(domain->primal) )/6;
	  side = ( triangle_index - 6*face - 12*primal_ncell(domain->primal))/2;
	  primal_face(domain->primal, face, face_nodes);
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];
	  /* the other face may not be there if not watertight (parallel) */
	  other_face = EMPTY;
	  primal_find_face_side(domain->primal, node1, node0, 
				&other_face, &other_side);
	  TRYN( primal_find_tri( domain->primal, 
				face_nodes[0], face_nodes[1], face_nodes[2],
				&tri ), "find tri for triangle init" );
	  TRYN( primal_find_tri_side( domain->primal, tri, node0, node1,
				     &tri_side ), "dual int find rt tri side");
	  first_side =  10 * primal_ncell(domain->primal) +
	    3  * primal_ntri(domain->primal) +
	    3  * primal_nface(domain->primal);
	  if ( 0 == triangle_index - 2*side - 6*face 
                                  - 12*primal_ncell(domain->primal) )
	    {
	      segment0 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	      segment1 = primal_face_side_node0(side) + 3 * face + 
		3 *primal_ntri(domain->primal) + 
		10 * primal_ncell(domain->primal);
	      segment2 = 0 + first_side + 2*domain->f2s[side+3*face];
	      if ( EMPTY != other_face && other_face < face) 
		segment2 = 1 + first_side + 2*domain->f2s[side+3*face];
	    }
	  else
	    {
	      segment0 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	      segment1 = 1 + first_side + 2*domain->f2s[side+3*face];
	      if ( EMPTY != other_face && other_face < face) 
		segment1 = 0 + first_side + 2*domain->f2s[side+3*face];
	      segment2 = primal_face_side_node1(side) + 3 * face + 
		3 *primal_ntri(domain->primal) + 
		10 * primal_ncell(domain->primal);

	    }
	  domain->triangle[triangle_index] =
	    triangle_create( domain_segment(domain,segment0),
			     domain_segment(domain,segment1),
			     domain_segment(domain,segment2), face);
	  NOT_NULLN(domain->triangle[triangle_index],"triangle_create NULL");
	  return domain->triangle[triangle_index];
	}
      printf("%s: %d: array bound error %d\n",
	     __FILE__,__LINE__, triangle_index);
      return NULL;
    }

  return domain->triangle[triangle_index];
}

KNIFE_STATUS domain_face_sides( Domain domain )
{
  int face;
  int side;
  int face_nodes[4];
  int node0, node1;
  int other_face, other_side;

  domain->f2s = (int *)malloc( 3*primal_nface(domain->primal)*sizeof(int) );

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++ ) 
    {
      domain->f2s[0+3*face] = EMPTY;
      domain->f2s[1+3*face] = EMPTY;
      domain->f2s[2+3*face] = EMPTY;
    }

  domain->nside = 0;
  for ( face = 0 ; face < primal_nface(domain->primal) ; face++ ) 
    {
      primal_face(domain->primal, face, face_nodes);
      for ( side = 0 ; side<3; side++ )
	if (EMPTY == domain->f2s[side+3*face])
	  {
	    domain->f2s[side+3*face] = domain->nside;
	    node0 = face_nodes[primal_face_side_node0(side)];
	    node1 = face_nodes[primal_face_side_node1(side)];
	    /* the other face may not be there if not watertight (parallel) */
	    if (KNIFE_SUCCESS == primal_find_face_side(domain->primal, 
						       node1, node0, 
						       &other_face, 
						       &other_side)) 
	      {
		domain->f2s[other_side+3*other_face] = domain->nside;
	      }
	    (domain->nside)++; 
	  }
    }

  domain->s2fs = (int *)malloc( 2*domain->nside*sizeof(int) );
  for ( side = 0 ; side < domain->nside ; side++ ) 
    {
      domain->s2fs[0+2*side] = EMPTY;
      domain->s2fs[1+2*side] = EMPTY;
    }
  
  for ( face = 0 ; face < primal_nface(domain->primal) ; face++ ) 
    for ( side = 0 ; side<3; side++ )
      if (EMPTY == domain->s2fs[0+2*(domain->f2s[side+3*face])] )
	{
	  domain->s2fs[0+2*(domain->f2s[side+3*face])] = face;
	  domain->s2fs[1+2*(domain->f2s[side+3*face])] = side;
	}

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_dual_elements( Domain domain )
{
  int node;
  int cell, face;
  int side;
  int segment_index, triangle_index;
  int cell_nodes[4], face_nodes[4];
  int cell_edge;
  int node0, node1;
  Triangle triangle;

  if ( NULL == domain->poly )
    {
      printf("call domain_required_dual or domain_all_dual or\n");
      printf("  domain_create_dual before domain_dual_elements");
      return KNIFE_NULL;
    }

  TRY( domain_face_sides( domain ), "domain_face_sides" );

  domain->nnode = 
    primal_ncell(domain->primal) +
    primal_ntri(domain->primal) +
    primal_nedge(domain->primal) +
    primal_surface_nnode(domain->primal);

  domain->node = (Node *)malloc( domain->nnode * sizeof(Node));
  domain_test_malloc(domain->node,
		     "domain_tetrahedral_elements node");
  for ( node =0 ; node < domain->nnode ; node++ )
    domain->node[node] = NULL;

  domain->nsegment = 
    10 * primal_ncell(domain->primal) +
    3  * primal_ntri(domain->primal) +
    3  * primal_nface(domain->primal)+
    2  * domain->nside;

  domain->segment = (Segment *)malloc( domain->nsegment * sizeof(Segment));
  domain_test_malloc(domain->segment,
		     "domain_tetrahedral_elements segment");
  for ( segment_index = 0 ; 
	segment_index < domain_nsegment(domain); 
	segment_index++ ) domain->segment[ segment_index ] = NULL;

  domain->ntriangle = 12*primal_ncell(domain->primal)
                    +  6*primal_nface(domain->primal);

  domain->triangle = (Triangle *)malloc( domain->ntriangle * sizeof(Triangle));
  domain_test_malloc(domain->triangle,"domain_dual_elements triangle");
  for ( triangle_index = 0 ; 
	triangle_index < domain_ntriangle(domain); 
	triangle_index++ ) domain->triangle[ triangle_index ] = NULL;
	  
  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      for ( cell_edge = 0 ; cell_edge < 6 ; cell_edge++)
	{
	  primal_cell(domain->primal,cell,cell_nodes);
	  node0 = cell_nodes[primal_cell_edge_node0(cell_edge)];
	  node1 = cell_nodes[primal_cell_edge_node1(cell_edge)];

	  triangle_index = 0 + 2 * cell_edge + 12 * cell;
	  /* triangle normal points from node0 to node1 */
	  if ( NULL != domain_poly(domain,node0) )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node0),
				      triangle, FALSE ), "poly_add_triangle");
	    }
	  if ( NULL != domain_poly(domain,node1) )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node1),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	  triangle_index = 1 + 2 * cell_edge + 12 * cell;
	  /* triangle normal points from node0 to node1 */
	  if ( NULL != domain_poly(domain,node0) )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node0),
				      triangle, FALSE ), "poly_add_triangle");
	    }
	  if ( NULL != domain_poly(domain,node1) )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node1),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	}
    }

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++)
    {
      primal_face(domain->primal, face, face_nodes);
      for ( side = 0 ; side < 3 ; side++)
	{
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];

	  triangle_index= 0 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  if ( NULL != domain_poly(domain,node0) )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node0),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	  triangle_index= 1 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  if ( NULL != domain_poly(domain,node1) )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node1),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	}
    }

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_add_interior_poly( Domain domain, int poly_index )
{
  int cell, face;
  int side;
  int triangle_index;
  int cell_nodes[4], face_nodes[4];
  int cell_edge;
  int node0, node1;
  Triangle triangle;

  AdjIterator it;

  if ( NULL == domain->poly || 
       NULL == domain->node || 
       NULL == domain->segment || 
       NULL == domain->triangle ||
       NULL == domain->f2s ||
       NULL == domain->s2fs )
    {
      printf("call domain_add_interior_poly after domain_dual_elements\n");
      return KNIFE_NULL;
    }

  if ( NULL != domain_poly(domain,poly_index) )
    {
      printf("%s: %d: domain_add_interior_poly: poly at poly_index not NULL \n",
	     __FILE__,__LINE__);
      return KNIFE_INCONSISTENT;
    }

  domain->poly[poly_index] = poly_create( );

  for ( it = adj_first(primal_cell_adj(domain->primal), poly_index);
	adj_valid(it);
	it = adj_next(it) )
    {
      cell = adj_item(it);
      for ( cell_edge = 0 ; cell_edge < 6 ; cell_edge++)
	{
	  primal_cell(domain->primal,cell,cell_nodes);
	  node0 = cell_nodes[primal_cell_edge_node0(cell_edge)];
	  node1 = cell_nodes[primal_cell_edge_node1(cell_edge)];

	  triangle_index = 0 + 2 * cell_edge + 12 * cell;
	  /* triangle normal points from node0 to node1 */
	  if ( poly_index == node0 )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node0),
				      triangle, FALSE ), "poly_add_triangle");
	    }
	  if ( poly_index == node1 )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node1),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	  triangle_index = 1 + 2 * cell_edge + 12 * cell;
	  /* triangle normal points from node0 to node1 */
	  if ( poly_index == node0 )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node0),
				      triangle, FALSE ), "poly_add_triangle");
	    }
	  if ( poly_index == node1 )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node1),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	}
    }

  for ( it = adj_first(primal_face_adj(domain->primal), poly_index);
	adj_valid(it);
	it = adj_next(it) )
    {
      face = adj_item(it);
      primal_face(domain->primal, face, face_nodes);
      for ( side = 0 ; side < 3 ; side++)
	{
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];

	  triangle_index= 0 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  if ( poly_index == node0 )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node0),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	  triangle_index= 1 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  if ( poly_index == node1 )
	    {
	      triangle = domain_triangle(domain,triangle_index);
	      NOT_NULL(triangle,"triangle NULL");
	      TRY( poly_add_triangle( domain_poly(domain,node1),
				      triangle, TRUE ), "poly_add_triangle");
	    }
	}
    }
	 
  TRY( poly_activate_all_subtri( domain_poly(domain,poly_index) ), 
       "poly_activate_all_subtri");
 
  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_all_dual( Domain domain )
{
  int poly_index;
  
  domain->npoly = primal_nnode(domain->primal);
  domain->poly = (Poly *)malloc(domain->npoly * sizeof(Poly));
  domain_test_malloc(domain->poly,"domain_dual_elements poly");
  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    domain->poly[poly_index] = poly_create( );

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_required_local_dual( Domain domain, int *required )
{
  int triangle_index;
  Triangle triangle;
  int segment_index;
  Segment segment;
  int i;
  NearStruct *triangle_tree;
  NearStruct *segment_tree;
  double center[3], diameter;
  int max_touched, ntouched;
  int *touched;
  NearStruct target;

  int poly_index;
  int edge_index, edge_nodes[2];
  int cell_index, cell_nodes[4];
  int tri_index, tri_nodes[3];
  int node, side;
  double xyz0[3], xyz1[3], xyz2[3];
  double t, uvw[3];
  double dx, dy, dz;
  KNIFE_STATUS intersection_status;
  int nrequired;

  for ( poly_index = 0 ; 
	poly_index < primal_nnode(domain->primal); 
	poly_index++)
    required[poly_index] = 0;

  triangle_tree = (NearStruct *)malloc( surface_ntriangle(domain->surface) * 
					sizeof(NearStruct));
  NOT_NULL( triangle_tree, "out of memory, could not malloc triangle_tree");

  for (triangle_index=0;
       triangle_index<surface_ntriangle(domain->surface);
       triangle_index++)
    {
      triangle_extent(surface_triangle(domain->surface,triangle_index),
		      center, &diameter);
      near_initialize( &(triangle_tree[triangle_index]), 
		       triangle_index, 
		       center[0], center[1], center[2], 
		       diameter );
      if (triangle_index > 0) near_insert( triangle_tree,
					   &(triangle_tree[triangle_index]) );
    }

  max_touched = surface_ntriangle(domain->surface);

  touched = (int *) malloc( max_touched * sizeof(int) );
  NOT_NULL( touched, "touched NULL");

  for (edge_index=0;edge_index<primal_nedge(domain->primal);edge_index++)
    {
      primal_edge(domain->primal,edge_index,edge_nodes);
      primal_xyz(domain->primal,edge_nodes[0],xyz0);
      primal_xyz(domain->primal,edge_nodes[1],xyz1);
      dx = xyz0[0]-xyz1[0];
      dy = xyz0[1]-xyz1[1];
      dz = xyz0[2]-xyz1[2];
      diameter = 0.5000001*sqrt(dx*dx+dy*dy+dz*dz);
      primal_edge_center( domain->primal, edge_index, center);
      near_initialize( &target, 
		       EMPTY, 
		       center[0], center[1], center[2], 
		       diameter );
      ntouched = 0;
      near_touched(triangle_tree, &target, &ntouched, max_touched, touched);
      for (i=0;i<ntouched;i++)
	{
	  triangle = surface_triangle(domain->surface,touched[i]);
	  intersection_status = intersection_core( triangle->node0->xyz,
						   triangle->node1->xyz,
						   triangle->node2->xyz,
						   xyz0, xyz1,
						   &t, uvw );

	  if ( KNIFE_SUCCESS == intersection_status )
	    {
	      required[edge_nodes[0]] = 1;
	      required[edge_nodes[1]] = 1;
	    }
	  else
	    if (KNIFE_NO_INT != intersection_status)
	      {
		printf("xyz0  %f %f %f\n",xyz0[0],xyz0[1],xyz0[2]);
		printf("xyz1  %f %f %f\n",xyz1[0],xyz1[1],xyz1[2]);
		printf("node0 %f %f %f\n",
		       (triangle->node0->xyz)[0],
		       (triangle->node0->xyz)[1],
		       (triangle->node0->xyz)[2]);
		printf("node1 %f %f %f\n",
		       (triangle->node1->xyz)[0],
		       (triangle->node1->xyz)[1],
		       (triangle->node1->xyz)[2]);
		printf("node2 %f %f %f\n",
		       (triangle->node2->xyz)[0],
		       (triangle->node2->xyz)[1],
		       (triangle->node2->xyz)[2]);
		printf("%s: %d: %d intersection_core\n",
		       __FILE__,__LINE__,intersection_status); 
		return intersection_status;
	      }
	    
	}
    }

  free(touched);
  free(triangle_tree);

  segment_tree = (NearStruct *)malloc( surface_nsegment(domain->surface) * 
				       sizeof(NearStruct));
  NOT_NULL( segment_tree, "segment_tree NULL");

  for (segment_index=0;
       segment_index<surface_nsegment(domain->surface);
       segment_index++)
    {
      segment_extent(surface_segment(domain->surface,segment_index),
		     center, &diameter);
      near_initialize( &(segment_tree[segment_index]), 
		       segment_index, 
		       center[0], center[1], center[2], 
		       diameter );
      if (segment_index > 0) near_insert( segment_tree,
					  &(segment_tree[segment_index]) );
    }

  max_touched = surface_nsegment(domain->surface);
  
  touched = (int *) malloc( max_touched * sizeof(int) );
  NOT_NULL( touched, "touched NULL");
  
  for (tri_index=0;tri_index<primal_ntri(domain->primal);tri_index++)
    {
      primal_tri(domain->primal,tri_index,tri_nodes);
      if ( 0 != required[tri_nodes[0]] && 
	   0 != required[tri_nodes[1]] && 
	   0 != required[tri_nodes[2]]  ) continue;
      primal_xyz(domain->primal,tri_nodes[0],xyz0);
      primal_xyz(domain->primal,tri_nodes[1],xyz1);
      primal_xyz(domain->primal,tri_nodes[2],xyz2);

      primal_tri_center( domain->primal, tri_index, center);
      
      dx = xyz0[0]-center[0];dy = xyz0[1]-center[1];dz = xyz0[2]-center[2];
      diameter = sqrt(dx*dx+dy*dy+dz*dz);

      dx = xyz1[0]-center[0];dy = xyz1[1]-center[1];dz = xyz1[2]-center[2];
      diameter = MAX(diameter,sqrt(dx*dx+dy*dy+dz*dz));

      dx = xyz2[0]-center[0];dy = xyz2[1]-center[1];dz = xyz2[2]-center[2];
      diameter = MAX(diameter,sqrt(dx*dx+dy*dy+dz*dz));

      near_initialize( &target, 
		       EMPTY, 
		       center[0], center[1], center[2], 
		       diameter );
      ntouched = 0;
      near_touched(segment_tree, &target, &ntouched, max_touched, touched);
      for (i=0;i<ntouched;i++)
	{
	  segment = surface_segment(domain->surface,touched[i]);

	  intersection_status = intersection_core( xyz0, xyz1, xyz2,
						   segment->node0->xyz,
						   segment->node1->xyz,
						   &t, uvw );
	  if ( KNIFE_NO_INT != intersection_status )
	    TRY( intersection_status, "intersection_core" );
	  if ( KNIFE_SUCCESS == intersection_status )
	    {
	      if ( KNIFE_SUCCESS == 
		   primal_find_cell_side( domain->primal, 
					  tri_nodes[0], 
					  tri_nodes[1], 
					  tri_nodes[2], 
					  &cell_index, &side ) )
		{
		  primal_cell(domain->primal,cell_index,cell_nodes);
		  for (node=0;node<4;node++)
		    required[cell_nodes[node]] = 1;
		}
	      if ( KNIFE_SUCCESS == 
		   primal_find_cell_side( domain->primal, 
					  tri_nodes[1], 
					  tri_nodes[0], 
					  tri_nodes[2], 
					  &cell_index, &side ) )
		{
		  primal_cell(domain->primal,cell_index,cell_nodes);
		  for (node=0;node<4;node++)
		    required[cell_nodes[node]] = 1;
		}
	      continue;
	    }

	}
    }

  free(touched);
  free(segment_tree);

  nrequired = 0;
  
  for ( poly_index = 0 ;
        poly_index < primal_nnode(domain->primal); 
        poly_index++)
    if ( 0 != required[poly_index]) nrequired++;

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_create_dual( Domain domain, int *required )
{
  int poly_index;

  domain->npoly = primal_nnode(domain->primal);
  domain->poly = (Poly *)malloc(domain->npoly * sizeof(Poly));
  domain_test_malloc(domain->poly,"domain_dual_elements poly");
  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    domain->poly[poly_index] = NULL;
  
  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    if ( 0 != required[poly_index] ) domain->poly[poly_index] = poly_create( );

  return (KNIFE_SUCCESS);
}


KNIFE_STATUS domain_required_dual( Domain domain )
{
  int triangle_index;
  Triangle triangle;
  int segment_index;
  Segment segment;
  int i;
  NearStruct *triangle_tree;
  NearStruct *segment_tree;
  double center[3], diameter;
  int max_touched, ntouched;
  int *touched;
  NearStruct target;

  int poly_index;
  int edge_index, edge_nodes[2];
  int cell_index, cell_nodes[4];
  int tri_index, tri_nodes[3];
  int node, side;
  double xyz0[3], xyz1[3], xyz2[3];
  double t, uvw[3];
  double dx, dy, dz;
  KNIFE_STATUS intersection_status;
  int nrequired;
  
  domain->npoly = primal_nnode(domain->primal);
  domain->poly = (Poly *)malloc(domain->npoly * sizeof(Poly));
  domain_test_malloc(domain->poly,"domain_dual_elements poly");
  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    domain->poly[poly_index] = NULL;
  
  triangle_tree = (NearStruct *)malloc( surface_ntriangle(domain->surface) * 
					sizeof(NearStruct));
  NOT_NULL( triangle_tree, "triangle_tree NULL");

  for (triangle_index=0;
       triangle_index<surface_ntriangle(domain->surface);
       triangle_index++)
    {
      triangle_extent(surface_triangle(domain->surface,triangle_index),
		      center, &diameter);
      near_initialize( &(triangle_tree[triangle_index]), 
		       triangle_index, 
		       center[0], center[1], center[2], 
		       diameter );
      if (triangle_index > 0) near_insert( triangle_tree,
					   &(triangle_tree[triangle_index]) );
    }

  max_touched = surface_ntriangle(domain->surface);

  touched = (int *) malloc( max_touched * sizeof(int) );
  NOT_NULL( touched, "touched NULL");

  for (edge_index=0;edge_index<primal_nedge(domain->primal);edge_index++)
    {
      primal_edge(domain->primal,edge_index,edge_nodes);
      primal_xyz(domain->primal,edge_nodes[0],xyz0);
      primal_xyz(domain->primal,edge_nodes[1],xyz1);
      dx = xyz0[0]-xyz1[0];
      dy = xyz0[1]-xyz1[1];
      dz = xyz0[2]-xyz1[2];
      diameter = 0.5000001*sqrt(dx*dx+dy*dy+dz*dz);
      primal_edge_center( domain->primal, edge_index, center);
      near_initialize( &target, 
		       EMPTY, 
		       center[0], center[1], center[2], 
		       diameter );
      ntouched = 0;
      near_touched(triangle_tree, &target, &ntouched, max_touched, touched);
      for (i=0;i<ntouched;i++)
	{
	  triangle = surface_triangle(domain->surface,touched[i]);
	  intersection_status = intersection_core( triangle->node0->xyz,
						   triangle->node1->xyz,
						   triangle->node2->xyz,
						   xyz0, xyz1,
						   &t, uvw );
	  if ( KNIFE_NO_INT != intersection_status )
	    TRY( intersection_status, "intersection_core" );
	  if ( KNIFE_SUCCESS == intersection_status )
	    {
	      if ( NULL == domain->poly[edge_nodes[0]] )
		domain->poly[edge_nodes[0]] = poly_create( );
	      if ( NULL == domain->poly[edge_nodes[1]] )
		domain->poly[edge_nodes[1]] = poly_create( );
	    }
	}
    }

  free(touched);
  free(triangle_tree);

  segment_tree = (NearStruct *)malloc( surface_nsegment(domain->surface) * 
				       sizeof(NearStruct));
  NOT_NULL( segment_tree, "segment_tree NULL");

  for (segment_index=0;
       segment_index<surface_nsegment(domain->surface);
       segment_index++)
    {
      segment_extent(surface_segment(domain->surface,segment_index),
		     center, &diameter);
      near_initialize( &(segment_tree[segment_index]), 
		       segment_index, 
		       center[0], center[1], center[2], 
		       diameter );
      if (segment_index > 0) near_insert( segment_tree,
					  &(segment_tree[segment_index]) );
    }

  max_touched = surface_nsegment(domain->surface);
  
  touched = (int *) malloc( max_touched * sizeof(int) );
  NOT_NULL( touched, "touched NULL");
  
  for (tri_index=0;tri_index<primal_ntri(domain->primal);tri_index++)
    {
      primal_tri(domain->primal,tri_index,tri_nodes);
      if ( NULL != domain->poly[tri_nodes[0]] &&
	   NULL != domain->poly[tri_nodes[1]] &&
	   NULL != domain->poly[tri_nodes[2]] ) continue;
      primal_xyz(domain->primal,tri_nodes[0],xyz0);
      primal_xyz(domain->primal,tri_nodes[1],xyz1);
      primal_xyz(domain->primal,tri_nodes[2],xyz2);

      primal_tri_center( domain->primal, tri_index, center);
      
      dx = xyz0[0]-center[0];dy = xyz0[1]-center[1];dz = xyz0[2]-center[2];
      diameter = sqrt(dx*dx+dy*dy+dz*dz);

      dx = xyz1[0]-center[0];dy = xyz1[1]-center[1];dz = xyz1[2]-center[2];
      diameter = MAX(diameter,sqrt(dx*dx+dy*dy+dz*dz));

      dx = xyz2[0]-center[0];dy = xyz2[1]-center[1];dz = xyz2[2]-center[2];
      diameter = MAX(diameter,sqrt(dx*dx+dy*dy+dz*dz));

      near_initialize( &target, 
		       EMPTY, 
		       center[0], center[1], center[2], 
		       diameter );
      ntouched = 0;
      near_touched(segment_tree, &target, &ntouched, max_touched, touched);
      for (i=0;i<ntouched;i++)
	{
	  segment = surface_segment(domain->surface,touched[i]);

	  intersection_status = intersection_core( xyz0, xyz1, xyz2,
						   segment->node0->xyz,
						   segment->node1->xyz,
						   &t, uvw );
	  if ( KNIFE_NO_INT != intersection_status )
	    TRY( intersection_status, "intersection_core" );
	  if ( KNIFE_SUCCESS == intersection_status )
	    {
	      if ( KNIFE_SUCCESS == 
		   primal_find_cell_side( domain->primal, 
					  tri_nodes[0], 
					  tri_nodes[1], 
					  tri_nodes[2], 
					  &cell_index, &side ) )
		{
		  primal_cell(domain->primal,cell_index,cell_nodes);
		  for (node=0;node<4;node++)
		    if ( NULL == domain->poly[cell_nodes[node]] )
		      domain->poly[cell_nodes[node]] = poly_create( );
		}
	      if ( KNIFE_SUCCESS == 
		   primal_find_cell_side( domain->primal, 
					  tri_nodes[1], 
					  tri_nodes[0], 
					  tri_nodes[2], 
					  &cell_index, &side ) )
		{
		  primal_cell(domain->primal,cell_index,cell_nodes);
		  for (node=0;node<4;node++)
		    if ( NULL == domain->poly[cell_nodes[node]] )
		      domain->poly[cell_nodes[node]] = poly_create( );
		}
	      continue;
	    }

	}
    }


  free(touched);
  free(segment_tree);

  touched = (int *) malloc( domain_npoly(domain) * sizeof(int) );

  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    touched[poly_index] = 0;

  for (cell_index=0;cell_index<primal_ncell(domain->primal);cell_index++)
    {
      primal_cell(domain->primal,cell_index,cell_nodes);
      if ( NULL != domain->poly[cell_nodes[0]] ||
	   NULL != domain->poly[cell_nodes[1]] ||
	   NULL != domain->poly[cell_nodes[2]] ||
	   NULL != domain->poly[cell_nodes[3]] ) 
	for (i=0;i<4;i++)
	  if ( NULL == domain->poly[cell_nodes[i]] )
	    touched[cell_nodes[i]] = 1;
    }

  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    if ( NULL == domain->poly[poly_index] && touched[poly_index] )
      domain->poly[poly_index] = poly_create( );

  free(touched);

  nrequired = 0;
  
  for (poly_index = 0 ; poly_index < domain_npoly(domain) ; poly_index++)
    if ( NULL != domain->poly[poly_index]) nrequired++;

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_boolean_subtract( Domain domain )
{
  int triangle_index;
  int i;
  NearStruct *triangle_tree;
  double center[3], diameter;
  int max_touched, ntouched;
  int *touched;
  NearStruct target;

  KNIFE_STATUS cut_status;

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:dual_elements");
  TRY( domain_dual_elements( domain ), "domain_dual_elements" );

  triangle_tree = (NearStruct *)malloc( surface_ntriangle(domain->surface) * 
					sizeof(NearStruct));
  NOT_NULL( triangle_tree, "triangle_tree NULL");

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:surface near");
  for (triangle_index=0;
       triangle_index<surface_ntriangle(domain->surface);
       triangle_index++)
    {
      triangle_extent(surface_triangle(domain->surface,triangle_index),
		      center, &diameter);
      near_initialize( &(triangle_tree[triangle_index]), 
		       triangle_index, 
		       center[0], center[1], center[2], 
		       diameter );
      if (triangle_index > 0) near_insert( triangle_tree,
					   &(triangle_tree[triangle_index]) );
    }

  max_touched = surface_ntriangle(domain->surface);

  touched = (int *) malloc( max_touched * sizeof(int) );
  NOT_NULL( touched, "touched NULL");

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:cut");
  for ( triangle_index = 0;
	triangle_index < domain_ntriangle(domain); 
	triangle_index++)
    if (NULL != domain->triangle[triangle_index] )
      {
	triangle_extent(domain_triangle(domain,triangle_index),
			center, &diameter);
	near_initialize( &target, 
			 EMPTY, 
			 center[0], center[1], center[2], 
			 diameter );
	ntouched = 0;
	near_touched(triangle_tree, &target, &ntouched, max_touched, touched);
	for (i=0;i<ntouched;i++)
	  {
	    cut_status = 
	      cut_establish_between( domain_triangle( domain,triangle_index ),
				     surface_triangle( domain->surface,
						       touched[i] ) );
	    if ( KNIFE_SUCCESS != cut_status)
	      {
		triangle_tecplot( domain_triangle( domain,triangle_index ) );
		triangle_tecplot( surface_triangle( domain->surface,
						    touched[i] ) );
	      }
	    TRY( cut_status, "cut establishment failed" );
	  }
      }

  free(touched);
  free(triangle_tree);

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:triangulate");
  TRY( domain_triangulate(domain), "domain_triangulate\n--> the triangulation step of the cut cell process failed <--" );

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:gather_surf");
  TRY( domain_gather_surf(domain), "domain_gather_surf" );

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:active");
  TRY( domain_determine_active_subtri(domain), 
       "domain_determine_active_subtri" );

  logger_message( DOMAIN_LOGGER_LEVEL, "subtract:set_dual_topology");
  TRY( domain_set_dual_topology( domain ), "domain_set_dual_topology" );

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_triangulate( Domain domain )
{
  int triangle_index;

  TRY( surface_triangulate(domain->surface), "surface_triangulate" );

  for ( triangle_index = 0;
	triangle_index < domain_ntriangle(domain); 
	triangle_index++)
    if (NULL != domain->triangle[triangle_index] )
      TRY( triangle_triangulate_cuts( domain_triangle(domain, 
						      triangle_index) ), 
	   "volume triangulate_cuts" );

  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_gather_surf( Domain domain )
{
  int poly_index;
  int cut_poly;

  cut_poly = 0;
  for ( poly_index = 0;
	poly_index < domain_npoly(domain); 
	poly_index++)
    if ( NULL != domain_poly(domain,poly_index) )
      {
	TRY( poly_gather_surf( domain_poly(domain,poly_index) ), 
	     "poly_gather_surf" );
	if ( poly_has_surf( domain_poly( domain, poly_index ) ) ) cut_poly++;
      }

  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_determine_active_subtri( Domain domain )
{
  int poly_index;

  for ( poly_index = 0;
	poly_index < domain_npoly0(domain); 
	poly_index++)
    if ( NULL != domain_poly(domain,poly_index) )
      if ( poly_has_surf( domain_poly( domain, poly_index ) ) )
	TRY( poly_determine_active_subtri( domain_poly(domain,poly_index) ),
	     "poly_determine_active_subtri" );

  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_set_dual_topology( Domain domain )
{
  int poly_index;
  int edge;
  int edge_nodes[2];
  Poly poly0, poly1;
  POLY_TOPO topo0, topo1;
  int node_index;
  Node node;
  KnifeBool active;

  KnifeBool requires_another_sweep;

  if (NULL == domain) return KNIFE_NULL;

  domain->topo = (POLY_TOPO *)malloc( domain_npoly(domain) * sizeof(POLY_TOPO));
  NOT_NULL( domain->topo, "domain->topo NULL");

  for ( poly_index = 0;
	poly_index < domain_npoly0(domain); 
	poly_index++)
    domain->topo[poly_index] = POLY_INTERIOR;

  for ( poly_index = domain_npoly0(domain);
	poly_index < domain_npoly(domain); 
	poly_index++)
    domain->topo[poly_index] = POLY_GHOST;

  for ( poly_index = 0;
	poly_index < domain_npoly0(domain); 
	poly_index++)
    if ( NULL != domain_poly(domain,poly_index) )
      {
	if ( poly_has_surf( domain_poly( domain, poly_index ) ) ) 
	  domain->topo[poly_index] = POLY_CUT;
      }

  for (edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      node_index = edge + 
	primal_ntri(domain->primal) + primal_ncell(domain->primal);
      node = domain_node(domain,node_index);
      TRY( primal_edge( domain->primal, edge, edge_nodes), 
	   "dual_topo cut int primal_edge" );
      poly0 = domain_poly(domain,edge_nodes[0]);
      poly1 = domain_poly(domain,edge_nodes[1]);
      if ( NULL != poly0 && NULL != poly1 )
	{
	  topo0 = domain->topo[edge_nodes[0]];
	  topo1 = domain->topo[edge_nodes[1]];

	  if ( POLY_CUT == topo0 &&  
	       ( POLY_INTERIOR == topo1 || POLY_GHOST == topo1 ) )
	    {
	      TRY( poly_mask_surrounding_node_activity( poly0, node,
							&active ), "active01");
	      if ( !active ) domain->topo[edge_nodes[1]] = POLY_EXTERIOR;
	    }

	  if ( POLY_CUT == topo1 && 
	       ( POLY_INTERIOR == topo0 || POLY_GHOST == topo0 ) )
	    {
	      TRY( poly_mask_surrounding_node_activity( poly1, node,
							&active ), "active10");
	      if ( !active ) domain->topo[edge_nodes[0]] = POLY_EXTERIOR;
	    }
	}

    }

  requires_another_sweep = TRUE;
  while (requires_another_sweep) 
    {
      requires_another_sweep = FALSE;
      for (edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
	{
	  TRY( primal_edge( domain->primal, edge, edge_nodes), 
	       "dual_topo ext int primal_edge" );
	  if ( edge_nodes[0] < domain_npoly0(domain) &&
	       edge_nodes[1] < domain_npoly0(domain) )
	    {
	      topo0 = domain->topo[edge_nodes[0]];
	      topo1 = domain->topo[edge_nodes[1]];
	      
	      if ( POLY_EXTERIOR == topo0 && POLY_INTERIOR == topo1 )
		{
		  requires_another_sweep = TRUE;
		  domain->topo[edge_nodes[1]] = POLY_EXTERIOR;
		}

	      if ( POLY_EXTERIOR == topo1 && POLY_INTERIOR == topo0 )
		{
		  requires_another_sweep = TRUE;
		  domain->topo[edge_nodes[0]] = POLY_EXTERIOR;
		}
	    }
	}
    }

  {
    int poly_index;
    int ninterior, ncut, nexterior;

    ninterior = 0;
    ncut = 0;
    nexterior = 0;
    
    for ( poly_index = 0;
	  poly_index < domain_npoly(domain);
	  poly_index++)
      {
	if ( POLY_INTERIOR == domain->topo[poly_index] )
	  ninterior++;
	if ( POLY_CUT      == domain->topo[poly_index] )
	  ncut++;
	if ( POLY_EXTERIOR == domain->topo[poly_index] )
	  nexterior++;
      }

  }
  
  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_tecplot( Domain domain, char *filename )
{
  FILE *f;
  int poly;

  if (NULL == filename) 
    {
      f = fopen("domain.t", "w");
    }
  else
    {
      f = fopen(filename, "w");
    }
  
  fprintf(f,"title=domain_geometry\nvariables=x,y,z,r\n");
  
  for (poly = 0 ; poly < domain_npoly(domain) ; poly++)
    if ( NULL != domain->poly[poly] )
      if ( domain_cut(domain,poly) )
	TRY( poly_tecplot_zone(domain_poly(domain,poly), f ), 
	     "poly_tecplot_zone");

  fclose(f);
  
  return KNIFE_SUCCESS;
}
