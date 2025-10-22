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
#include "prefix_or_ls_table.h"

/* Initialize ls_table */
struct ls_table *
ls_table_init(u_char prefixsize, u_char slotsize)
{
    struct ls_table *rt;
    
    rt = (struct ls_table *)malloc(LS_TABLE_SIZE(slotsize));
    if (rt == NULL)
        return NULL;
    
    memset (rt, 0, LS_TABLE_SIZE(slotsize));
    rt->prefixsize = prefixsize;
    rt->slotsize = slotsize;
    
    return rt;
}

/* ls_prefix free */
void
ls_prefix_free(struct ls_prefix *p)
{
    free(p);
}

/* Free route node */
void
ls_node_free(struct ls_node *node)
{
    ls_prefix_free(node->p);
    free(node);
}

/* Lock node */
struct ls_node*
ls_lock_node (struct ls_node *node)
{
    node->lock++;
    return node;
}

/* Delete node from the routing table */
void
ls_node_delete (struct ls_node *node)
{
    struct ls_node *child = NULL;
    struct ls_node *parent = NULL;
    
    //assert(node->lock == 0);
    //assert(node->vinfo0 == NULL);
    
    if (node->l_left && node->l_right)
        return;
    
    if (node->l_left) {
        child = node->l_left;
    } else {
        child = node->l_right;
    }
    
    parent = node->parent;
    
    if(child)
        child->parent = parent;
    
    if (parent)
    {
        if (parent->l_left == node)
            parent->l_left = child;
        else
            parent->l_right = child;
    }
    else
        node->table->top = child;
    
    ls_node_free(node);
    
    /* If parent node is stub then delte it also */
    if (parent && parent->lock == 0)
        ls_node_delete(parent);
}

/* Unlock the node */
void
ls_unlock_node(struct ls_node *node)
{
    node->lock--;
    if (node->lock == 0)
        ls_node_delete(node);
}


int
ls_prefix_match(struct ls_prefix *n, struct ls_prefix *p)
{
    int offset;
    int shift;
    
    /* Set both prefix's head pointer */
    u_char *np = n->prefix;
    u_char *pp = p->prefix;
    
    
    if (n->prefixlen > p->prefixlen)
        return 0;
    
    offset = n->prefixlen / 8;
    shift = n->prefixlen % 8;
    
    if (shift)
        if (maskbit[shift] & (np[offset] ^ pp[offset]))
            return 0;
    
    while(offset--)
        if(np[offset] != pp[offset])
            return 0;
    
    return 1;
}

struct ls_prefix *
ls_prefix_new (size_t size)
{
    struct ls_prefix *p = NULL;
    
    p = (struct ls_prefix *) malloc (sizeof (struct ls_prefix) + size - LS_PREFIX_MIN_SIZE);
    
    memset(p, 0, (sizeof (struct ls_prefix) + size - LS_PREFIX_MIN_SIZE));
    if (p == NULL)
        return NULL;
    
    p->prefixsize = size;
    
    return p;
}

static int
check_bit (u_char *prefix, u_char prefixlen)
{
    int offset;
    int shift;
    u_char *p = (u_char *)prefix;
    
    //assert(prefixlen <= 128);
    
    offset =  prefixlen / 8;
    shift = 7 - (prefixlen % 8);
    
    return (p[offset] >> shift & 1);
}

void
ls_prefix_copy(struct ls_prefix *dest, struct ls_prefix *src)
{
    memcpy(dest, src, LS_PREFIX_SIZE(src));
}


/* Allocate new route node with allocating new ls_prefix */
struct ls_node *
ls_node_set (struct ls_table *table, struct ls_prefix *prefix)
{
    struct ls_node *node;
    
    node = (struct ls_node*)malloc(LS_NODE_SIZE(table));
    memset(node, 0, LS_NODE_SIZE(table));
    if (node == NULL)
        return NULL;
    
    node->p = ls_prefix_new(prefix->prefixsize);
    if (node->p == NULL)
    {
        free(node);
        return NULL;
    }
    
    ls_prefix_copy(node->p, prefix);
    node->table = table;
    
    return node;
}



static void
set_link (struct ls_node *node, struct ls_node *new)
{
    int bit;
    bit = check_bit(new->p->prefix, node->p->prefixlen);
    
    //assert(bit == 0 || bit ==1);
    
    node->link[bit] = new;
    new->parent = node;
}


struct ls_node *
ls_node_next (struct ls_node *node)
{
    struct ls_node *next;
    struct ls_node *start;
    
    /* node may be deleted from ls_unlock_node so we have to preserve
     next nodes's pointer */
    
    if (node->l_left)
    {
        next = node->l_left;
        ls_lock_node (next);
        ls_unlock_node (node);
        return next;
    }
    
    if (node->l_right)
    {
        next = node->l_right;
        ls_lock_node(next);
        ls_unlock_node(node);
        return next;
    }
    
    start = node;
    
    while(node->parent)
    {
        if (node->parent->l_left == node && node->parent->l_right)
        {
            next =  node->parent->l_right;
            ls_lock_node(next);
            ls_unlock_node(start);
            return next;
        }
        node = node->parent;
    }
    
    ls_unlock_node(start);
    return NULL;
}

/* Common prefix route generation */
struct ls_prefix *
ls_route_common (struct ls_prefix *n, struct ls_prefix *p)
{
    int i;
    u_char diff;
    u_char mask;
    
    struct ls_prefix *new;
    
    u_char *np = n->prefix;
    u_char *pp = p->prefix;
    u_char *newp;
    
    
    new = ls_prefix_new(n->prefixsize);
    if (new == NULL)
        return NULL;
    
    newp = new->prefix;
    
    for (i = 0; i < p->prefixlen / 8; i++)
    {
        if (np[i] == pp[i])
            newp[i] = np[i];
        else
            break;
    }
    
    new->prefixlen = i * 8;
    
    if(new->prefixlen != p->prefixlen)
    {
        diff = np[i] ^ pp[i];
        mask = 0x80;
        while (new->prefixlen < p->prefixlen && !(mask & diff))
        {
            mask >>=1;
            new->prefixlen++;
        }
        newp[i] = np[i] & maskbit[new->prefixlen % 8];
    }
    return new;
}

int
ls_node_info_exist (struct ls_node *node)
{
    int index;
    
    for(index = 0; index < node->table->slotsize; index++)
        if(node->vinfo[index])
            return 1;
    
    return 0;
}
    
/* lookup same prefix node. Return NULL when we can't find the route */
struct ls_node*
ls_node_lookup(struct ls_table *table, struct ls_prefix *p)
{
    if(table == NULL)
        return NULL;
    
   struct ls_node *node = NULL;
    
   while(node && node->p->prefixlen <= p->prefixlen &&
          ls_prefix_match(node->p, p))
    {
        if(node->p->prefixlen == p->prefixlen)
        {
            if(ls_node_info_exist(node))
                return ls_lock_node (node);
        }
        node = node->link[check_bit(p->prefix, node->p->prefixlen)];
    }
    return node;
}

struct ls_node *
ls_table_top(struct ls_table *table)
{
    /* if there s no node in the routing table return NULL */
    if (table == NULL || table->top == NULL)
        return NULL;
    
    ls_lock_node(table->top);
    return table->top;
}

struct ls_node *
ls_route_next(struct ls_node *node)
{
    struct ls_node *next;
    struct ls_node *start;
    
    /* Node mat=y be deleted from ls_unlock_node so we have to preserve next
     nodes pointer */
    
    if (node->l_left)
    {
        next = node->l_left;
        ls_lock_node(next);
        ls_unlock_node(node);
        return next;
    }
    
    if (node->l_right)
    {
        next = node->l_right;
        ls_lock_node(next);
        ls_unlock_node(node);
        return next;
    }
    
    start =  node;
    while(node->parent)
    {
        if (node->parent->l_left == node && node->parent->l_right)
        {
            next = node->parent->l_right;
            ls_lock_node(next);
            ls_unlock_node(start);
            return next;
        }
        node = node->parent;
    }
    ls_unlock_node(start);
    return NULL;
}

struct ls_node *
ls_node_lookup_first(struct ls_table *table)
{
    struct ls_node *node;
    
    for (node = ls_table_top(table); node; node = ls_route_next(node))
        if(node->vinfo0)
            return node;
    
    return NULL;
}

struct ls_node*
ls_node_create (struct ls_table *table)
{
    struct ls_node *new;
    
    new = (struct ls_node*)malloc(LS_NODE_SIZE(table));
    if(new == NULL)
        return NULL;
    
    return new;
}

struct ls_node*
ls_node_get (struct ls_table *table, struct ls_prefix *p)
{
    struct ls_node *new;
    struct ls_node *node;
    struct ls_node *match;
    
    match = NULL;
    
    if(table == NULL)
        return NULL;
    
    node = table->top;
    while(node && node->p->prefixlen <= p->prefixlen &&
          ls_prefix_match(node->p, p))
    {
        if (node->p->prefixlen == p->prefixlen)
        {
            ls_lock_node(node);
            return node;
        }
        
        match = node;
        node = node->link[check_bit(p->prefix, node->p->prefixlen)];
    }
    
    if (node == NULL)
    {
        new = ls_node_set(table, p);
        if(new == NULL)
            return NULL;
        
        if (match) {
            set_link(match, new);
        } else {
            table->top = new;
        }
    }
    else
    {
        new = ls_node_create(table);
        if (new == NULL) {
            return NULL;
        }
        
        new->p = ls_route_common(node->p, p);
        if (new->p == NULL) {
            free(new);
            return NULL;
        }
        new->p->family = p->family;
        new->table = table;
        set_link(new, node);
        
        if (match)
            set_link(match, new);
        else
            table->top = new;
        
        if (new->p->prefixlen != p->prefixlen)
        {
            match = new;
            new = ls_node_get(table, p);
            if (new == NULL)
                return NULL;
            
            set_link(match, new);
        }
    }
    ls_lock_node(new);
    return new;
}

struct ls_node *
ls_route_next_until(struct ls_node *node, struct ls_node *limit)
{
    struct ls_node *next;
    struct ls_node *start;
    
    /* Node mat=y be deleted from ls_unlock_node so we have to preserve next
     nodes pointer */
    
    if (node->l_left)
    {
        next = node->l_left;
        ls_lock_node(next);
        ls_unlock_node(node);
        return next;
    }
    
    if (node->l_right)
    {
        next = node->l_right;
        ls_lock_node(next);
        ls_unlock_node(node);
        return next;
    }
    
    start =  node;
    while(node->parent && node != limit)
    {
        if (node->parent->l_left == node && node->parent->l_right)
        {
            next = node->parent->l_right;
            ls_lock_node(next);
            ls_unlock_node(start);
            return next;
        }
        node = node->parent;
    }
    ls_unlock_node(start);
    return NULL;
}

void *
ls_node_info_set (struct ls_node *node , int index, void *info)
{
    void *old =  node->vinfo[index];
    
    if(old == NULL)
    {
        ls_lock_node(node);
        node->table->count[index]++;
    }
    
    node->vinfo[index] = info;
    
    if (info == NULL)
    {
        node->table->count[index]--;
        ls_unlock_node(node);
    }
    
    return old;
}

void *
ls_node_info_usnet(struct ls_node *node, int index)
{
    void *old = node->vinfo[index];
    
    if(node->vinfo[index] != NULL)
    {
        node->vinfo[index] = NULL;
        node->table->count[index]--;
        ls_unlock_node(node);
    }
    
    return old;
}

struct ls_node *
ls_node_info_lookup(struct ls_table *table, struct ls_prefix *p, int index)
{
    struct ls_node *node = NULL;
    //assert(table->slotsize > index);
    
    node = ls_node_lookup(table, p);
    if(node)
    {
        if (node->vinfo[index])
            return node;
        
        ls_unlock_node(node);
    }
    
    return NULL;
}

int
str2prefix_ipv4(const char *str, struct ls_prefix *p)
{
    char *pnt = NULL;
    char *cp = NULL;
    unsigned int alloc = 0;
    signed int plen;
    
    
    pnt = strchr(str, '/');
    alloc  = (unsigned int)(pnt - str) + 1;
    cp = (char*)malloc(alloc);
    strncpy(cp, str, (pnt -str));
    inet_pton(AF_INET, cp, &p->prefix);
    free(cp);
    
    plen = (unsigned char)strtoul(++pnt, NULL, 10);
    
    p->family = AF_INET;
    p->prefixlen = plen;
    p->prefixsize = plen / 8;
    
    return 0;
}






















