//
//  prefix_or_ls_table.h
//  Interview programming
//
//  Created by chetan ganesh  on 17/11/24.
//
//
//  prefix_or_ls_table.c
//  Interview programming
//
//  Created by chetan ganesh  on 17/11/24.
//

//#include "prefix_or_ls_table.h"
//
//  ptree.c
//
//
//  Created by chetan ganesh  on 16/11/24.
//

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#incldue <assert.h>

typedef unsigned char u_char;
typedef unsigned int u_int32_t;

#define LS_PREFIX_MIN_SIZE 4 /* Key Size */
#define DEFAULT_SLOT_SIZE 1 /* how many infos required for each rn */

static const u_int8_t maskbit[] =
{0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

//#define NULL ((void *)0)

/* LS Prefix struture */
struct ls_prefix
{
    u_char family;
    u_char prefixlen;
    u_char prefixsize;
    u_char pad;
    u_char prefix[LS_PREFIX_MIN_SIZE];
};/* struct ls_prefix */

/* Each routing entry */
struct ls_node
{
    /* Donot move the first 2 pointers are used by the memory manager aswell */
    struct ls_node *link[2];
#define l_left link[0]
#define l_right link[1]
    
    /* Pointer to the variable length prefix of the radix */
    struct ls_prefix *p;
    
    /* Tree link */
    struct ls_table *table;
    struct ls_node *parent;
    
    /*Lock of this radix */
    unsigned int lock;
    
    /* Routing information */
    void *vinfo[1];
#define vinfo0 vinfo[0]
#define vinfo1 vinfo[1]
#define vinfo2 vinfo[2]
    
    /* Convenient macro */
#define ri_cur vinfo[0]
#define ri_new vinfo[1]
#define ri_lsas vinfo[2]
};/* struct ls_node */


/* Route table top struture */
struct ls_table
{
  /* Description for this table */
    char *desc;
    
  /* Top node of the route table */
    struct ls_node *top;
    
    /* Prefix sizeof the depth */
    u_char prefixsize;
    
    /* Number of info slots */
    u_char slotsize;
    
    /* Counter of vinfo */
    u_int32_t count[1];
}; /* struct ls_table */

#define LS_PREFIX_SIZE(P) \
  (sizeof(struct ls_prefix) + (P)->prefixsize - LS_PREFIX_MIN_SIZE)

#define LS_TABLE_SIZE(S) \
 (sizeof(struct ls_table) + (((S) - 1) * sizeof (u_int32_t)))

#define LS_NODE_SIZE(T) \
  (sizeof(struct ls_node) + ((T)->slotsize - 1) * sizeof (void *))


/* Initialize ls_table */
struct ls_table *
ls_table_init(u_char prefixsize, u_char slotsize);
/* ls_prefix free */
void
ls_prefix_free(struct ls_prefix *p);
/* Free route node */
void
ls_node_free(struct ls_node *node);

/* Lock node */
struct ls_node*
ls_lock_node (struct ls_node *node);

/* Delete node from the routing table */
void
ls_node_delete (struct ls_node *node);

/* Unlock the node */
void
ls_unlock_node(struct ls_node *node);

int
ls_prefix_match(struct ls_prefix *n, struct ls_prefix *p);


struct ls_prefix *
ls_prefix_new (size_t size);




void
ls_prefix_copy(struct ls_prefix *dest, struct ls_prefix *src);



/* Allocate new route node with allocating new ls_prefix */
struct ls_node *
ls_node_set (struct ls_table *table, struct ls_prefix *prefix);

struct ls_node *
ls_node_next (struct ls_node *node);


/* Common prefix route generation */
struct ls_prefix *
ls_route_common (struct ls_prefix *n, struct ls_prefix *p);


int
ls_node_info_exist (struct ls_node *node);

    
/* lookup same prefix node. Return NULL when we can't find the route */
struct ls_node*
ls_node_lookup(struct ls_table *table, struct ls_prefix *p);


struct ls_node *
ls_table_top(struct ls_table *table);


struct ls_node *
ls_route_next(struct ls_node *node);


struct ls_node *
ls_node_lookup_first(struct ls_table *table);


struct ls_node*
ls_node_create (struct ls_table *table);


struct ls_node*
ls_node_get (struct ls_table *table, struct ls_prefix *p);


struct ls_node *
ls_route_next_until(struct ls_node *node, struct ls_node *limit);


void *
ls_node_info_set (struct ls_node *node , int index, void *info);


void *
ls_node_info_usnet(struct ls_node *node, int index);


struct ls_node *
ls_node_info_lookup(struct ls_table *table, struct ls_prefix *p, int index);


int
str2prefix_ipv4(const char *str, struct ls_prefix *p);


























