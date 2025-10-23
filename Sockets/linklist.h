#include "lib.h"

typedef unsigned int (*list_del_cb_t) (void *val);
typedef unsigned int (*list_cmp_cb_t) (void *val1, void *val2);

struct listnode
{
  struct listnode *next;
  struct listnode *prev;
  void *data;
};
typedef struct listnode Listnode_T;

struct list
{
  struct listnode *head;
  struct listnode *tail;
  u_int32_t count;
  list_cmp_cb_t cmp;
  list_del_cb_t del;
};
typedef struct list List_T;

#define NEXTNODE(X) ((X) = (X)->next)
#define PREVNODE(X) ((X) = (X)->prev)
#define LISTHEAD(X) ((X)->head)
#define LISTTAIL(X) ((X)->tail)
#define LISTCOUNT(X) (((X) != NULL) ? ((X)->count) : 0)
#define listcount(X) (((X) != NULL) ? ((X)->count) : 0)
#define LIST_ISEMPTY(X) ((X)->head == NULL && (X)->tail == NULL)
#define list_isempty(X) ((X)->head == NULL && (X)->tail == NULL)
#define GETDATA(X) (((X) != NULL) ? (X)->data : NULL)

/* Prototypes. */
struct list *list_new();
struct list *list_create(list_cmp_cb_t cmp_cb, list_del_cb_t del_cb);
struct list *link_list_init(struct list *, list_cmp_cb_t cmp_cb,
                             list_del_cb_t del_cb);
void
list_concat_list (struct list *l, struct list *m);

void list_free (struct list *);

struct listnode *listnode_add (struct list *, void *);
struct listnode *listnode_add_sort (struct list *, void *);
u_int16_t listnode_add_sort_index (struct list *,u_int32_t *);

struct listnode *listnode_add_before (struct list *, struct listnode *,void *);
int listnode_add_sort_nodup (struct list *, void *);
struct listnode *listnode_add_after (struct list *, struct listnode *, void *);
void listnode_delete (struct list *, void *);
void listnode_delete_data (struct list *, void *);
int list_delete_data (struct list *list, void *val);
struct listnode *listnode_lookup (struct list *, void *);
void * list_lookup_data (struct list *list, void *data);
struct listnode* listnode_lookup_data (struct list *list, void *data);
void *listnode_head (struct list *);
void *listnode_tail (struct list *);

void list_delete (struct list *);
void list_delete_all_node (struct list *);
void list_delete_list (struct list *list);

/* For ospfd and ospf6d. */
void list_delete_node (struct list *, struct listnode *);

/* For ospf_spf.c */
void list_add_node_prev (struct list *, struct listnode *, void *);
void list_add_node_next (struct list *, struct listnode *, void *);
void list_add_list (struct list *, struct list *);

/* List iteration macro. */
#define LIST_LOOP(L,V,N) \
  if ((L) && (L)->count) \
    for ((N) = (L)->head; ((N)  && ((L) && (L)->count)); (N) = (N)->next) \
      if (((V) = (N)->data) != NULL)

/* List reverse iteration macro. */
#define LIST_REV_LOOP(L,V,N) \
  if ((L) && (L)->count) \
    for ((N) = (L)->tail; ((N)  && ((L) && (L)->count)); (N) = (N)->prev) \
      if (((V) = (N)->data) != NULL)

/* List reverse iteration macro. */
#define LIST_REV_LOOP_DEL(L,V,N,NN) \
  if ((L) && (L)->count) \
    for ((N) = (L)->tail, NN = ((N)!=NULL) ? (N)->prev : NULL; \
         ((N)  && ((L) && (L)->count)); \
         (N) = (NN), NN = ((N)!=NULL) ? (N)->prev : NULL) \
      if (((V) = (N)->data) != NULL)

/* List iteration macro.
 * It allows to delete N and V in the middle of the loop
 */
#define LIST_LOOP_DEL(L,V,N,NN) \
  if ((L) && (L)->count) \
    for ((N) = (L)->head, NN = ((N)!=NULL) ? (N)->next : NULL;  \
         ((N)  && ((L) && (L)->count));                           \
         (N) = (NN), NN = ((N)!=NULL) ? (N)->next : NULL)       \
      if (((V) = (N)->data) != NULL)

#define LIST_LOOP_NODE(L,V,N,NN) \
  if (L) \
    for (NN = ((N)!=NULL) ? (N)->next : NULL;                   \
         ((N)  && ((L) && (L)->count));                           \
         (N) = (NN), NN = ((N)!=NULL) ? (N)->next : NULL)       \
      if (((V) = (N)->data) != NULL)

/* List node add macro.  */
#define LISTNODE_ADD(L,N) \
  do { \
    (N)->next = NULL; \
    (N)->prev = (L)->tail; \
    if ((L)->head == NULL) \
      (L)->head = (N); \
    else \
      (L)->tail->next = (N); \
    (L)->tail = (N); \
    (L)->count++; \
  } while (0)

#define LISTNODE_ADD_TAIL(L,N) LISTNODE_ADD(L,N)

#define LISTNODE_ADD_HEAD(L,N) \
  do { \
    (N)->next = (L)->head; \
    (N)->prev = NULL; \
    if ((L)->head == NULL) { \
      (L)->head = (N); \
      (L)->tail = (N); \
    } \
    else {	\
      (L)->head->prev = (N); \
      (L)->head = (N); \
    } \
  } while (0)

#define LISTNODE_ADD_TAIL(L,N) LISTNODE_ADD(L,N)

#define LISTNODE_ADD_HEAD(L,N) \
  do { \
    (N)->next = (L)->head; \
    (N)->prev = NULL; \
    if ((L)->head == NULL) { \
      (L)->head = (N); \
      (L)->tail = (N); \
    } \
    else {	\
      (L)->head->prev = (N); \
      (L)->head = (N); \
    } \
  } while (0)

/* List node delete macro.  */
#define LISTNODE_REMOVE(L,N) \
  do { \
    if ((N)->prev) \
      (N)->prev->next = (N)->next; \
    else \
      (L)->head = (N)->next; \
    if ((N)->next) \
      (N)->next->prev = (N)->prev; \
    else \
      (L)->tail = (N)->prev; \
    (L)->count--; \
  } while (0)

