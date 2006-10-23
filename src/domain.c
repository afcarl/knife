
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

#define TRY(fcn,msg)					      \
  {							      \
    int code;						      \
    code = (fcn);					      \
    if (KNIFE_SUCCESS != code){				      \
      printf("%s: %d: %d %s\n",__FILE__,__LINE__,code,(msg)); \
      return code;					      \
    }							      \
  }

#define TRYN(fcn,msg)					      \
  {							      \
    int code;						      \
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

Node domain_node( Domain domain, int node_index )
{
  int cell, tri, edge;
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
      /* boundary nodes should have been added ahead for time */
      printf("%s: %d: domain_node face node not precomputed %d\n",
	     __FILE__,__LINE__, node_index);
      return NULL;
    }

  return domain->node[node_index];
}

Segment domain_segment( Domain domain, int segment_index )
{
  int cell, side, edge, tri;
  int edge_index;
  int tri_center, edge_center, cell_center;
  int tri_nodes[3];

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
      /* boundary nodes should have been added ahead for time */
      printf("%s: %d: domain_segment face segment not precomputed %d\n",
	     __FILE__,__LINE__, segment_index);
      return NULL;
    }

  return domain->segment[segment_index];
}



KNIFE_STATUS domain_dual_elements( Domain domain )
{
  int node;
  int cell, tri, face;
  int side;
  int tri_center, edge_center;
  int edge_index, segment_index, triangle_index;
  int tri_side, cell_side;
  int tri_nodes[3], cell_nodes[4], face_nodes[4];
  double xyz[3];
  int cell_edge;
  int segment0, segment1, segment2;
  int node0, node1;
  int poly;
  int surface_nnode;
  int *node_g2l;
  int node_index;
  int other_face, other_side;
  int *f2s;

  printf("primal: nnode %d nface %d ncell %d nedge %d ntri %d\n",
	 primal_nnode(domain->primal),
	 primal_nface(domain->primal),
	 primal_ncell(domain->primal),
	 primal_nedge(domain->primal),
	 primal_ntri(domain->primal));

  printf("create poly for primal nodes\n");

  domain->npoly = primal_nnode(domain->primal);
  domain->poly = (PolyStruct *)malloc(domain->npoly * sizeof(PolyStruct));
  domain_test_malloc(domain->poly,"domain_dual_elements poly");
  for (poly = 0 ; poly < domain_npoly(domain) ; poly++)
    TRY( poly_initialize(domain_poly(domain,poly)), "poly init");
  

  node_g2l = (int *)malloc( primal_nnode(domain->primal)*sizeof(int) );
  for ( node = 0 ; node < primal_nnode(domain->primal) ; node++) 
    node_g2l[node] = EMPTY;

  surface_nnode = 0;
  for ( face = 0 ; face < primal_nface(domain->primal) ; face++ )
    {
      primal_face(domain->primal, face, face_nodes);
      for (node=0;node<3;node++)
	if (EMPTY == node_g2l[face_nodes[node]]) 
	  {
	    node_g2l[face_nodes[node]] = surface_nnode;
	    surface_nnode++;
	  }
    }

  printf("number of nodes in the surface %d\n",surface_nnode);

  printf("create dual nodes\n");

  domain->nnode = 
    primal_ncell(domain->primal) +
    primal_ntri(domain->primal) +
    primal_nedge(domain->primal) +
    surface_nnode;
  domain->node = (Node *)malloc( domain->nnode * sizeof(Node));
  domain_test_malloc(domain->node,
		     "domain_tetrahedral_elements node");
  printf("number of dual nodes in the volume %d\n",domain->nnode);
  for ( node =0 ; node < domain->nnode ; node++ )
    domain->node[node] = NULL;

  /* create boundary nodes even though they may not be needed */
  for ( node = 0 ; 
	node < primal_nnode(domain->primal) ; 
	node++) 
    if ( EMPTY != node_g2l[node] )
      {
	node_index = 
	  node_g2l[node] + 
	  primal_nedge(domain->primal) + 
	  primal_ntri(domain->primal) + 
	  primal_ncell(domain->primal);
	primal_xyz(domain->primal,node,xyz);
	domain->node[node_index] = node_create( xyz );
	NOT_NULL( domain->node[node_index], "boundary face node create NULL");
      }

  printf("create dual segments\n");

  domain->nsegment = 
    10 * primal_ncell(domain->primal) +
    3  * primal_ntri(domain->primal) +
    3  * primal_nface(domain->primal);

  f2s = (int *)malloc( 3*primal_nface(domain->primal)*sizeof(int) );

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++ ) 
    {
      f2s[0+3*face] = EMPTY;
      f2s[1+3*face] = EMPTY;
      f2s[2+3*face] = EMPTY;
    }

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++ ) 
    {
      primal_face(domain->primal, face, face_nodes);
      for ( side = 0 ; side<3; side++ )
	if (EMPTY == f2s[side+3*face])
	  {
	    f2s[side+3*face] = domain->nsegment;
	    node0 = face_nodes[primal_face_side_node0(side)];
	    node1 = face_nodes[primal_face_side_node1(side)];
	    TRY( primal_find_face_side(domain->primal, node1, node0, 
				       &other_face, &other_side), "face_side"); 
	    f2s[other_side+3*other_face] = domain->nsegment;
	    domain->nsegment += 2; /* a tri side has 2 segments */
	  }
    }

  domain->segment = (Segment *)malloc( domain->nsegment * sizeof(Segment));
  domain_test_malloc(domain->segment,
		     "domain_tetrahedral_elements segment");
  for ( segment_index = 0 ; 
	segment_index < domain_nsegment(domain); 
	segment_index++ ) domain->segment[ segment_index ] = NULL;
  printf("number of dual segments in the volume %d\n",domain->nsegment);

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++)
    {
      primal_face(domain->primal, face, face_nodes);
      TRY( primal_find_tri( domain->primal, 
			    face_nodes[0], face_nodes[1], face_nodes[2],
			    &tri ), "find tri for face" );
      tri_center = tri + primal_ncell(domain->primal);;
      primal_tri(domain->primal,tri,tri_nodes);
      for ( node = 0 ; node < 3 ; node++)
	{
	  segment_index = node + 3 * face + 
	    3 *primal_ntri(domain->primal) + 10 * primal_ncell(domain->primal);
	  node_index = 
	    node_g2l[face_nodes[node]] + 
	    primal_nedge(domain->primal) + 
	    primal_ntri(domain->primal) + 
	    primal_ncell(domain->primal);
	  domain->segment[segment_index] = 
	    segment_create( domain_node(domain,tri_center),
			    domain_node(domain,node_index) );
	}
      for ( side = 0 ; side < 3 ; side++)
	{
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];
	  TRY( primal_find_face_side(domain->primal, node1, node0, 
				     &other_face, &other_side), "i face_side"); 
	  if ( face < other_face ) /* only init segment once */
	    {
	      TRY( primal_find_edge( domain->primal, node0, node1, 
				     &edge_index ), "face seg find edge" );
	      edge_center = edge_index + primal_ntri(domain->primal) 
		                       + primal_ncell(domain->primal);
	      segment_index = 0 + f2s[side+3*face];
	      node_index = 
		node_g2l[node0] + 
		primal_nedge(domain->primal) + 
		primal_ntri(domain->primal) + 
		primal_ncell(domain->primal);
	      domain->segment[segment_index] = 
		segment_create( domain_node(domain,node_index),
				domain_node(domain,edge_center) );
	      segment_index = 1 + f2s[side+3*face];
	      node_index = 
		node_g2l[node1] + 
		primal_nedge(domain->primal) + 
		primal_ntri(domain->primal) + 
		primal_ncell(domain->primal);
	      domain->segment[segment_index] = 
		segment_create( domain_node(domain,edge_center),
				domain_node(domain,node_index) );
	    }
	}
    }

  printf("create interior dual triangles\n");

  domain->ntriangle = 12*primal_ncell(domain->primal)
                    +  6*primal_nface(domain->primal);
  domain->triangle = (TriangleStruct *)malloc( domain->ntriangle * 
					       sizeof(TriangleStruct));
  domain_test_malloc(domain->triangle,"domain_dual_elements triangle");
  printf("number of dual triangles in the volume %d\n",domain->ntriangle);

  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      for ( cell_edge = 0 ; cell_edge < 6 ; cell_edge++)
	{
	  primal_cell(domain->primal,cell,cell_nodes);
	  node0 = cell_nodes[primal_cell_edge_node0(cell_edge)];
	  node1 = cell_nodes[primal_cell_edge_node1(cell_edge)];

	  cell_side = primal_cell_edge_left_side(cell_edge);
	  tri = primal_c2t(domain->primal,cell,cell_side);
	  TRY( primal_find_tri_side( domain->primal, tri, node0, node1,
				     &tri_side ), "dual int find lf tri side");
	  triangle_index = 0 + 2 * cell_edge + 12 * cell;
	  segment0 = cell_side + 10 * cell;
	  segment1 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	  segment2 = cell_edge + 4 + 10 * cell;
	  /* triangle normal points from node0 to node1 */
	  TRY( triangle_initialize( domain_triangle(domain,triangle_index),
				    domain_segment(domain,segment0),
				    domain_segment(domain,segment1),
				    domain_segment(domain,segment2), EMPTY ), 
	       "triangle init");

	  cell_side = primal_cell_edge_right_side(cell_edge);
	  tri = primal_c2t(domain->primal,cell,cell_side);
	  TRY( primal_find_tri_side( domain->primal, tri, node0, node1,
				     &tri_side ), "dual int find rt tri side");
	  triangle_index = 1 + 2 * cell_edge + 12 * cell;
	  segment0 = cell_side + 10 * cell;
	  segment1 = cell_edge + 4 + 10 * cell;
	  segment2 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	  /* triangle normal points from node0 to node1 */
	  TRY( triangle_initialize( domain_triangle(domain,triangle_index),
				    domain_segment(domain,segment0),
				    domain_segment(domain,segment1),
				    domain_segment(domain,segment2), EMPTY ), 
	       "int tri init");

	}
    }

  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      for ( cell_edge = 0 ; cell_edge < 6 ; cell_edge++)
	{
	  primal_cell(domain->primal,cell,cell_nodes);
	  node0 = cell_nodes[primal_cell_edge_node0(cell_edge)];
	  node1 = cell_nodes[primal_cell_edge_node1(cell_edge)];

	  triangle_index = 0 + 2 * cell_edge + 12 * cell;
	  /* triangle normal points from node0 to node1 */
	  poly_add_triangle( domain_poly(domain,node0),
			     domain_triangle(domain,triangle_index), FALSE );
	  poly_add_triangle( domain_poly(domain,node1),
			     domain_triangle(domain,triangle_index), TRUE );

	  triangle_index = 1 + 2 * cell_edge + 12 * cell;
	  /* triangle normal points from node0 to node1 */
	  poly_add_triangle( domain_poly(domain,node0),
			     domain_triangle(domain,triangle_index), FALSE );
	  poly_add_triangle( domain_poly(domain,node1),
			     domain_triangle(domain,triangle_index), TRUE );
	}
    }

  printf("create boundary dual triangles\n");

  for ( face = 0 ; face < primal_nface(domain->primal) ; face++)
    {
      primal_face(domain->primal, face, face_nodes);
      TRY( primal_find_tri( domain->primal, 
			    face_nodes[0], face_nodes[1], face_nodes[2],
			    &tri ), "find tri for triangle init" );
      for ( side = 0 ; side < 3 ; side++)
	{
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];
	  TRY( primal_find_face_side(domain->primal, node1, node0, 
				     &other_face, &other_side), "u face_side"); 
	  TRY( primal_find_tri_side( domain->primal, tri, node0, node1,
				     &tri_side ), "dual int find rt tri side");
	  triangle_index= 0 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  segment0 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	  segment1 = primal_face_side_node0(side) + 3 * face + 
	    3 *primal_ntri(domain->primal) + 10 * primal_ncell(domain->primal);
	  segment2 = 0 + f2s[side+3*face];
	  if (other_face < face) segment2 = 1 + f2s[side+3*face];
	  TRY( triangle_initialize( domain_triangle(domain,triangle_index),
				    domain_segment(domain,segment0),
				    domain_segment(domain,segment1),
				    domain_segment(domain,segment2), face),
	       "boundary tri init 0");

	  triangle_index= 1 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  segment0 = tri_side + 3 * tri + 10 * primal_ncell(domain->primal);
	  segment1 = 1 + f2s[side+3*face];
	  if (other_face < face) segment1 = 0 + f2s[side+3*face];
	  segment2 = primal_face_side_node1(side) + 3 * face + 
	    3 *primal_ntri(domain->primal) + 10 * primal_ncell(domain->primal);
	  TRY( triangle_initialize( domain_triangle(domain,triangle_index),
				    domain_segment(domain,segment0),
				    domain_segment(domain,segment1),
				    domain_segment(domain,segment2), face),
	       "boundary tri init 1");
	}
    }
	  
  for ( face = 0 ; face < primal_nface(domain->primal) ; face++)
    {
      primal_face(domain->primal, face, face_nodes);
      TRY( primal_find_tri( domain->primal, 
			    face_nodes[0], face_nodes[1], face_nodes[2],
			    &tri ), "find tri for triangle init" );
      for ( side = 0 ; side < 3 ; side++)
	{
	  node0 = face_nodes[primal_face_side_node0(side)];
	  node1 = face_nodes[primal_face_side_node1(side)];

	  triangle_index= 0 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  poly_add_triangle( domain_poly(domain,node0),
			     domain_triangle(domain,triangle_index), TRUE );

	  triangle_index= 1 + 2*side + 6*face + 12*primal_ncell(domain->primal);
	  poly_add_triangle( domain_poly(domain,node1),
			     domain_triangle(domain,triangle_index), TRUE );
	}
    }
	  
  free(node_g2l);
  free(f2s);
  
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

  printf("forming surface near tree\n");

  near_tree = (NearStruct *)malloc( surface_ntriangle(domain->surface) * 
				    sizeof(NearStruct));
  NOT_NULL( near_tree, "near_tree NULL");

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

  printf("compute cuts\n");

  max_touched = surface_ntriangle(domain->surface);

  touched = (int *) malloc( max_touched * sizeof(int) );
  NOT_NULL( touched, "touched NULL");

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
	  TRY( cut_establish_between( domain_triangle(domain,triangle_index),
				      surface_triangle(domain->surface,
						       touched[i]) ),
	       "cut establishment failed" );
	}
    }

  free(touched);
  free(near_tree);

  TRY( domain_triangulate(domain), "domain_triangulate" );

  printf("gather surface\n");

  TRY( domain_gather_surf(domain), "domain_gather_surf" );

  printf("determine active subtris\n");

  TRY( domain_determine_active_subtri(domain), 
       "domain_determine_active_subtri" );

  printf("establish dual topology\n");

  TRY( domain_set_dual_topology( domain ), "domain_set_dual_topology" );

  printf("boolean subtract completed\n");

  return (KNIFE_SUCCESS);
}

KNIFE_STATUS domain_triangulate( Domain domain )
{
  int triangle_index;

  printf("triangulate surface\n");

  TRY( surface_triangulate(domain->surface), "surface_triangulate" );

  printf("triangulate volume\n");

  for ( triangle_index = 0;
	triangle_index < domain_ntriangle(domain); 
	triangle_index++)
    TRY( triangle_triangulate_cuts( domain_triangle(domain, triangle_index) ), 
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
    {
      TRY( poly_gather_surf( domain_poly(domain,poly_index) ), "poly_gather_surf" );
      if ( poly_has_surf( domain_poly( domain, poly_index ) ) ) cut_poly++;
    }

  printf("polyhedra %d of %d have been cut\n",cut_poly,domain_npoly(domain));

  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_determine_active_subtri( Domain domain )
{
  int poly_index;

  for ( poly_index = 0;
	poly_index < domain_npoly(domain); 
	poly_index++)
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

  for ( poly_index = 0;
	poly_index < domain_npoly(domain); 
	poly_index++)
    {
      if ( poly_has_surf( domain_poly( domain, poly_index ) ) ) 
	domain_poly(domain,poly_index)->topo = POLY_CUT;
    }

  for (edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      node_index = edge + 
	primal_ntri(domain->primal) + primal_ncell(domain->primal);
      node = domain_node(domain,node_index);
      TRY( primal_edge( domain->primal, edge, edge_nodes), 
	   "dual_topo cut int primal_edge" );
      poly0 = domain_poly(domain,edge_nodes[0]);
      topo0 = poly_topo(poly0);
      poly1 = domain_poly(domain,edge_nodes[1]);
      topo1 = poly_topo(poly1);

      if ( POLY_CUT == topo0 && POLY_INTERIOR == topo1 )
	{
	  TRY( poly_mask_surrounding_node_activity( poly0, node,
						    &active ), "active01");
	  if ( !active ) poly_topo(poly1) = POLY_EXTERIOR;
	}

      if ( POLY_CUT == topo1 && POLY_INTERIOR == topo0 )
	{
	  TRY( poly_mask_surrounding_node_activity( poly1, node,
						    &active ), "active10");
	  if ( !active ) poly_topo(poly0) = POLY_EXTERIOR;
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
	  poly0 = domain_poly(domain,edge_nodes[0]);
	  topo0 = poly_topo(poly0);
	  poly1 = domain_poly(domain,edge_nodes[1]);
	  topo1 = poly_topo(poly1);

	  if ( POLY_EXTERIOR == topo0 && POLY_INTERIOR == topo1 )
	    {
	      requires_another_sweep = TRUE;
	      poly_topo(poly1) = POLY_EXTERIOR;
	    }

	  if ( POLY_EXTERIOR == topo1 && POLY_INTERIOR == topo0 )
	    {
	      requires_another_sweep = TRUE;
	      poly_topo(poly0) = POLY_EXTERIOR;
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
	if ( POLY_INTERIOR == poly_topo( domain_poly( domain, poly_index ) ) )
	  ninterior++;
	if ( POLY_CUT      == poly_topo( domain_poly( domain, poly_index ) ) )
	  ncut++;
	if ( POLY_EXTERIOR == poly_topo( domain_poly( domain, poly_index ) ) )
	  nexterior++;
      }

    printf( "poly: %d interior %d cut %d exterior\n",
	    ninterior, ncut, nexterior);
  }
  
  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_export_fun3d( Domain domain )
{
  int poly_index;
  Poly poly;
  int nnode, ntet, nedge, norig, ncut, nface, ntri;
  double xyz[3], center[3], volume;
  int *node_g2l, *face_g2l, *face_l2g;
  int node, cell, edge, face;
  FILE *f;
  int cell_nodes[4];
  int edge_nodes[2];
  int face_nodes[4];
  int node0, node1;
  int node_index;
  Node edge_node;
  double directed_area[3];

  int max_face_id, face_id;
  int i;

  KnifeBool active, original;

  printf("dump node stuff\n");

  node_g2l = (int *)malloc( primal_nnode(domain->primal)*sizeof(int) );
  for ( node = 0 ; node < primal_nnode(domain->primal) ; node++)
    node_g2l[node] = EMPTY;

  nnode = 0;
  for ( poly_index = 0;
	poly_index < domain_npoly(domain);
	poly_index++)
    {
      poly = domain_poly(domain,poly_index);
      if (poly_active(poly))
	{
	  node_g2l[poly_index] = nnode;
	  nnode++;
	}
    }

  f = fopen("postslice.nodes","w");
  NOT_NULL(f,"nodes file not open");

  fprintf(f,"%d\n",nnode);

  for ( poly_index = 0;
	poly_index < domain_npoly(domain);
	poly_index++)
    {
      if ( EMPTY != node_g2l[poly_index] )
	{
	  poly = domain_poly(domain,poly_index);
	  primal_xyz(domain_primal(domain),poly_index,xyz);
	  if (poly_cut(poly)) 
	    {
	      poly_centroid_volume(poly,xyz,center,&volume);
	      fprintf(f,"%.16e %.16e %.16e\n",
		      center[0],center[1],center[2]);	  
	    }
	  else
	    {
	      fprintf(f,"%.16e %.16e %.16e\n",
		      xyz[0],xyz[1],xyz[2]);	  
	    }
	}
    }

  for ( poly_index = 0;
	poly_index < domain_npoly(domain);
	poly_index++)
    {
      if ( EMPTY != node_g2l[poly_index] )
	{
	  poly = domain_poly(domain,poly_index);
	  primal_xyz(domain_primal(domain),poly_index,xyz);
	  poly_centroid_volume(poly,xyz,center,&volume);
	  fprintf(f,"%.16e\n",volume);	  
	}
    }

  fclose(f);

  printf("dump tet stuff\n");

  ntet = 0;
  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      primal_cell(domain->primal, cell, cell_nodes);
      if ( ( EMPTY != node_g2l[cell_nodes[0]] ) &&
	   ( EMPTY != node_g2l[cell_nodes[1]] ) &&
	   ( EMPTY != node_g2l[cell_nodes[2]] ) &&
	   ( EMPTY != node_g2l[cell_nodes[3]] ) ) ntet++; 
    }

  f = fopen("postslice.tets","w");
  NOT_NULL(f,"tets file not open");

  fprintf(f,"%d\n",ntet);

  for ( cell = 0 ; cell < primal_ncell(domain->primal) ; cell++)
    {
      primal_cell(domain->primal, cell, cell_nodes);
      if ( ( EMPTY != node_g2l[cell_nodes[0]] ) &&
	   ( EMPTY != node_g2l[cell_nodes[1]] ) &&
	   ( EMPTY != node_g2l[cell_nodes[2]] ) &&
	   ( EMPTY != node_g2l[cell_nodes[3]] ) ) 
	fprintf(f,"%d %d %d %d\n",
		1+node_g2l[cell_nodes[0]], 
		1+node_g2l[cell_nodes[1]], 
		1+node_g2l[cell_nodes[2]], 
		1+node_g2l[cell_nodes[3]]);
    }

  fclose(f);

  printf("dump edge stuff\n");

  nedge = 0;
  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) ) nedge++; 
    }

  f = fopen("postslice.edges","w");
  NOT_NULL(f,"tets file not open");

  fprintf(f,"%d\n",nedge);

  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) )
	{
	  node0 = node_g2l[edge_nodes[0]];
	  node1 = node_g2l[edge_nodes[1]];
	  if ( node0<node1)
	    {
	      fprintf(f,"%d %d\n",1+node0,1+node1);
	    }
	  else
	    {
	      fprintf(f,"%d %d\n",1+node1,1+node0);
	    }
	}
    }

  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) )
	{
	  node_index = edge + 
	    primal_ntri(domain->primal) + primal_ncell(domain->primal);
	  edge_node = domain_node(domain,node_index);

	  node0 = node_g2l[edge_nodes[0]];
	  node1 = node_g2l[edge_nodes[1]];
	  if ( node0<node1)
	    {
	      poly = domain_poly(domain, edge_nodes[0]);
	    }
	  else
	    {
	      poly = domain_poly(domain, edge_nodes[1]);
	    }
	  TRY( poly_directed_area_about( poly, edge_node, 
					 directed_area), "directed_area");
	  fprintf(f,"%.16e %.16e %.16e\n",
		  directed_area[0],directed_area[1],directed_area[2]);	  
	}
    }

  norig = 0;
  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) &&
	   !poly_cut(domain_poly(domain,edge_nodes[0])) &&
	   !poly_cut(domain_poly(domain,edge_nodes[1])) ) norig++; 
    }

  fprintf(f,"%d\n",norig);

  nedge = 0;
  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) )
	{
	  if ( !poly_cut(domain_poly(domain,edge_nodes[0])) &&
	       !poly_cut(domain_poly(domain,edge_nodes[1])) ) 
	    fprintf(f,"%d\n",1+nedge);
	  nedge++;
	} 
    }

  fclose(f);

  printf("dump face stuff\n");

  ncut = 0;
  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) )
	{
	  if ( poly_cut(domain_poly(domain,edge_nodes[0])) ||
	       poly_cut(domain_poly(domain,edge_nodes[1])) ) ncut++;
	} 
    }

  f = fopen("postslice.faces","w");
  NOT_NULL(f,"faces file not open");

  fprintf(f,"%d\n",ncut);

  ncut = 0;
  for ( edge = 0 ; edge < primal_nedge(domain->primal) ; edge++)
    {
      primal_edge(domain->primal, edge, edge_nodes);
      if ( ( EMPTY != node_g2l[edge_nodes[0]] ) &&
	   ( EMPTY != node_g2l[edge_nodes[1]] ) )
	if ( poly_cut(domain_poly(domain,edge_nodes[0])) ||
	     poly_cut(domain_poly(domain,edge_nodes[1])) )
	  {
	    node_index = edge + 
	      primal_ntri(domain->primal) + primal_ncell(domain->primal);
	    edge_node = domain_node(domain,node_index);

	    node0 = node_g2l[edge_nodes[0]];
	    node1 = node_g2l[edge_nodes[1]];
	    if ( node0<node1)
	      {
		fprintf(f,"%d %d\n",1+node0,1+node1);
		poly = domain_poly(domain, edge_nodes[0]);
	      }
	    else
	      {
		fprintf(f,"%d %d\n",1+node1,1+node0);
		poly = domain_poly(domain, edge_nodes[1]);
	      }
	    TRY( poly_face_geometry_about( poly, edge_node, f), 
		 "edge face geom");
	} 
    }

  fclose(f);

  printf("dump bound stuff\n");

  TRY( primal_max_face_id(domain->primal, &max_face_id), "max_face_id");;

  f = fopen("postslice.bound","w");
  NOT_NULL(f,"bound file not open");

  fprintf(f,"%d\n",max_face_id);

  face_g2l = (int *)malloc( primal_nnode(domain->primal)*sizeof(int) );

  for ( face_id = 1; face_id <= max_face_id; face_id++)
    {

      for ( node = 0 ; node < primal_nnode(domain->primal) ; node++)
	face_g2l[node] = EMPTY;

      nnode = 0;
      nface = 0;

      for ( face = 0; face < primal_nface(domain->primal); face++)
	{
	  primal_face(domain->primal,face,face_nodes);
	  if ( face_id == face_nodes[3] &&
	       poly_original(domain_poly(domain,face_nodes[0])) &&
	       poly_original(domain_poly(domain,face_nodes[1])) &&
	       poly_original(domain_poly(domain,face_nodes[2])) )
	    {
	      nface++;
	      for ( i = 0; i < 3 ; i++ )
		{
		  if ( EMPTY == face_g2l[face_nodes[i]] )
		    {
		      face_g2l[face_nodes[i]] = nnode;
		      nnode++;
		    }
		}
	    }
	}

      fprintf(f,"%d\n",nnode);

      face_l2g = (int *)malloc( nnode*sizeof(int) );

      nnode = 0;
      for ( node = 0 ; node < primal_nnode(domain->primal) ; node++)
	if ( EMPTY != face_g2l[node] ) 
	  {
	    face_l2g[nnode] = node;
	    face_g2l[node] = nnode;
	    nnode++;
	  }

      for ( node = 0 ; node < nnode ; node++)
	fprintf(f,"%d\n",1+node_g2l[face_l2g[node]]);

      free(face_l2g);

      fprintf(f,"%d\n",nface);
      for ( face = 0; face < primal_nface(domain->primal); face++)
	{
	  primal_face(domain->primal,face,face_nodes);
	  if ( face_id == face_nodes[3] &&
	       poly_original(domain_poly(domain,face_nodes[0])) &&
	       poly_original(domain_poly(domain,face_nodes[1])) &&
	       poly_original(domain_poly(domain,face_nodes[2])) )
	    fprintf(f,"%d %d %d\n",
		    1+face_g2l[face_nodes[0]],
		    1+face_g2l[face_nodes[1]],
		    1+face_g2l[face_nodes[2]] );
	}      
      
    }

  free(face_g2l);

  fclose(f);

  printf("dump surf stuff\n");

  f = fopen("postslice.surf","w");
  NOT_NULL(f,"surf file not open");

  ntri = 0;
  for ( face = 0; face < primal_nface(domain->primal); face++)
    {
      primal_face(domain->primal,face,face_nodes);
      
      active =( poly_active(domain_poly(domain,face_nodes[0])) ||
		poly_active(domain_poly(domain,face_nodes[1])) ||
		poly_active(domain_poly(domain,face_nodes[2])) );
      
      original =( poly_original(domain_poly(domain,face_nodes[0])) &&
		  poly_original(domain_poly(domain,face_nodes[1])) &&
		  poly_original(domain_poly(domain,face_nodes[2])) );
      

      if (active && !original) 
	{
	  if (poly_active(domain_poly(domain,face_nodes[0]))) ntri++;
	  if (poly_active(domain_poly(domain,face_nodes[1]))) ntri++;
	  if (poly_active(domain_poly(domain,face_nodes[2]))) ntri++;
	}
    }

  fprintf(f,"%d\n",ntri);

  for ( face = 0; face < primal_nface(domain->primal); face++)
    {
      primal_face(domain->primal,face,face_nodes);
      
      active = ( poly_active(domain_poly(domain,face_nodes[0])) ||
		 poly_active(domain_poly(domain,face_nodes[1])) ||
		 poly_active(domain_poly(domain,face_nodes[2])) );
      
      original = ( poly_original(domain_poly(domain,face_nodes[0])) &&
		   poly_original(domain_poly(domain,face_nodes[1])) &&
		   poly_original(domain_poly(domain,face_nodes[2])) );
      
      if (active && !original) 
	{
	  if (poly_active(domain_poly(domain,face_nodes[0])))
	    {
	      fprintf(f,"%d\n",1+node_g2l[face_nodes[0]]);	      
	      TRY( poly_boundary_face_geometry(domain_poly(domain,
							   face_nodes[0]),
					       face, f), "poly bound 0");
	    }
	  if (poly_active(domain_poly(domain,face_nodes[1])))
	    {
	      fprintf(f,"%d\n",1+node_g2l[face_nodes[1]]);	      
	      TRY( poly_boundary_face_geometry(domain_poly(domain,
							   face_nodes[1]),
					       face, f), "poly bound 1");
	    }
	  if (poly_active(domain_poly(domain,face_nodes[2])))
	    {
	      fprintf(f,"%d\n",1+node_g2l[face_nodes[2]]);	      
	      TRY( poly_boundary_face_geometry(domain_poly(domain,
							   face_nodes[2]),
					       face, f), "poly bound 2");
	    }
	}
    }
  
  ncut = 0;
  for ( poly_index = 0;
	poly_index < domain_npoly(domain);
	poly_index++)
    if ( poly_cut( domain_poly( domain, poly_index ) ) )
      ncut++;

  fprintf(f,"%d\n",ncut);
  
  for ( poly_index = 0;
	poly_index < domain_npoly(domain);
	poly_index++)
    if ( poly_cut( domain_poly( domain, poly_index ) ) )
      {
	fprintf(f,"%d\n",1+node_g2l[poly_index]);
	TRY( poly_surf_geometry(domain_poly(domain,poly_index), f),"poly surf");
      }

  fclose(f);

  free(node_g2l);

  return KNIFE_SUCCESS;
}

KNIFE_STATUS domain_tecplot( Domain domain, char *filename )
{
  FILE *f;
  int poly;

  if (NULL == filename) 
    {
      printf("tecplot output to domain.t\n");
      f = fopen("domain.t", "w");
    }
  else
    {
      printf("tecplot output to %s\n",filename);
      f = fopen(filename, "w");
    }
  
  fprintf(f,"title=domain_geometry\nvariables=x,y,z\n");
  
  for (poly = 0 ; poly < domain_npoly(domain) ; poly++)
    if ( poly_active( domain_poly(domain,poly) ) )
      TRY( poly_tecplot_zone(domain_poly(domain,poly), f ), 
	   "poly_tecplot_zone");

  fclose(f);

  printf("tecplot output complete\n");
  
  return KNIFE_SUCCESS;
}
