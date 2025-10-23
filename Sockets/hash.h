#include "lib.h"
/* Default hash table size.  */
#define HASHTABSIZE     1024

struct hash_backet
{
  /* Linked list.  */
  struct hash_backet *next;

  /* Data.  */
  void *data;

  /* Hash key. */
  u_int64_t key;
};
typedef struct hash_backet Hash_backet_T;

struct hash
{
  /* Hash backet. */
  struct hash_backet **index;

  /* Hash table size. */
  u_int32_t size;

  /* Key make function. */
  u_int32_t (*hash_key) ();

  /* Key make function. */
  u_int64_t (*hash64_key) ();

  /* Data compare function. */
  bool (*hash_cmp) ();

  /* Backet alloc. */
  u_int32_t count;
};
typedef struct hash Hash_T;

/* Use GOTO to break the HASH_ITERATE */
#define HASH_ITERATE(hash, backet, i)              \
  if (hash)                                   \
  for (i = 0; i < hash->size; i++)            \
    for (backet = hash->index[i]; backet; backet = backet->next)

struct hash *hash_create (u_int32_t (*) (), bool (*) ());
struct hash *hash_create_size (u_int32_t,
                               u_int32_t (*) (), bool (*) ());
struct hash *hash64_create_size (u_int32_t,
                               u_int64_t (*) (), bool (*) ());
void *hash_get (struct hash *, void *, void * (*) ());
void *hash_alloc_intern (void *);
void *hash_lookup (struct hash *, void *);
void *hash_release (struct hash *, void *);
void
hash_iterate_release (struct hash *hash,
                      bool (*func) (struct hash_backet *, void *),
                      void *arg,
                      void (*free_func) (void *));

void hash_iterate (struct hash *,
                   void (*) (struct hash_backet *, void *), void *);

void hash_iterate2 (struct hash *,
                    void (*) (struct hash_backet *, void *, void *),
                    void *, void *);

void hash_iterate3 (struct hash *,
                    void (*) (struct hash_backet *, void *, void *, void *),
                    void *, void *, void *);

void hash_iterate4 (struct hash *hash,
       void (*func) (struct hash_backet *, void *, void *, void *, void *),
       void *arg1, void *arg2, void *arg3, void *arg4);

void hash_iterate5 (struct hash *hash,
       void (*func) (struct hash_backet *,void *,void *,void *,void *,void *),
       void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);

void hash_iterate6 (struct hash *hash,
       void (*func) (struct hash_backet *,void *,void *,void *,void *,void *, void *),
       void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);


void hash_iterate7 (struct hash *hash,
                    void (*func) (struct hash_backet *, void *, void *, void *,
                    void *,void *, void *, void *),
                    void *arg1, void *arg2, void *arg3, void *arg4, void *arg5,
                    void *arg6, void *arg7);

void hash_clean (struct hash *, void (*) (void *));
void hash_free (struct hash *);

void hash_iterate_delete (struct hash *,
                          void (*) (struct hash_backet *, void *), void *);

void hash_iterate_delete2 (struct hash *,
                           void (*) (struct hash_backet *, void *, void *),
                           void *, void *);
void hash_iterate_delete3 (struct hash *hash,
                      void (*) (struct hash_backet *, void *, void *, void *),
                           void *, void *, void *);

void hash_iterate_delete4 (struct hash *hash,
                      void (*) (struct hash_backet *, void *, void *, void *, void *),
                           void *, void *, void *, void*);
void
hash_iterate_delete5 (struct hash *hash,
                      void (*) (struct hash_backet *, void *, void *, void *, void *, void *),
                      void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);
void
hash_iterate_delete6 (struct hash *hash,
                      void (*) (struct hash_backet *, void *, void *, void *, void *, void *, void *),
                      void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);

void *
hash_get_first_data (struct hash *hash);

void *
hash_get_next_data (struct hash *hash, void *data);

void *hash_set (struct hash *, void *, void *);

u_int32_t hash_key_make (char *);

