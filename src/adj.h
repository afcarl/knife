
/* adjacency tool for assoicated objects */

/* $Id$ */

/* Michael A. Park (Mike Park)
 * Computational AeroSciences Branch
 * NASA Langley Research Center
 * Hampton, VA 23681
 * Phone:(757) 864-6604
 * Email: Mike.Park@NASA.Gov
 */

#ifndef ADJ_H
#define ADJ_H

#include "knife_definitions.h"

BEGIN_C_DECLORATION

typedef struct Adj Adj;
typedef struct AdjItem AdjItem;
typedef AdjItem * AdjIterator;

struct AdjItem {
  int item;
  AdjItem *next;
};

struct Adj {
  int nnode, nadj, chunk_size;
  AdjItem *node2item;
  AdjItem **first;
  AdjItem *current;
  AdjItem *blank;
};

Adj *adj_create( int nnode, int nadj, int chunkSize );
void adj_free( Adj *adj );

#define adj_nnode(adj) (adj->nnode)
#define adj_nadj(adj) (adj->nadj)
#define adj_chunk_size(adj) (adj->chunk_size)

Adj *adj_resize( Adj *adj, int nnode );

Adj *adj_register( Adj *adj, int node, int item );
Adj *adj_remove( Adj *adj, int node, int item );

#define adj_valid(iterator) (iterator!=NULL)
#define adj_more(iterator) ((iterator!=NULL)&&(iterator->next != NULL))
#define adj_first(adj,node) \
((node < 0 || node >= adj->nnode)?NULL:adj->first[node])

#define adj_item(iterator) (iterator==NULL?EMPTY:iterator->item)
#define adj_next(iterator) (iterator==NULL?NULL:iterator->next)

KnifeBool adj_exists( Adj *adj, int node, int item );
int adj_degree( Adj *adj, int node );

END_C_DECLORATION

#endif /* ADJ_H */
