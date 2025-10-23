#include "hash.h"

/* Allocate a new hash.  */
struct hash *
hash_create_size (u_int32_t size,
                  u_int32_t (*hash_key) (), bool (*hash_cmp) ())
{
  struct hash *hash;

  hash = (struct hash *)malloc (sizeof (struct hash));
  memset(hash, 0, sizeof (struct hash));
  hash->index = (struct hash_backet **)malloc (
                         sizeof (struct hash_backet *) * size);
  memset(hash->index, 0, sizeof (struct hash_backet *) * size);
  hash->size = size;
  hash->hash_key = hash_key;
  hash->hash64_key = NULL;
  hash->hash_cmp = hash_cmp;
  hash->count = 0;

  return hash;
}

struct hash *
hash64_create_size (u_int32_t size,
                  u_int64_t (*hash_key) (), bool (*hash_cmp) ())
{
  struct hash *hash;

  hash = malloc (sizeof (struct hash));
  memset(hash, 0, sizeof (struct hash));
  hash->index = (struct hash_backet **)malloc (
                         sizeof (struct hash_backet *) * size);
  memset(hash->index, 0, sizeof (struct hash_backet *) * size);
  hash->size = size;
  hash->hash64_key = hash_key;
  hash->hash_key = NULL;
  hash->hash_cmp = hash_cmp;
  hash->count = 0;

  return hash;
}

/* Allocate a new hash with default hash size.  */
struct hash *
hash_create (u_int32_t (*hash_key) (), bool (*hash_cmp) ())
{
  return hash_create_size (HASHTABSIZE, hash_key, hash_cmp);
}

/* Utility function for hash_get().  When this function is specified
   as alloc_func, return arugment as it is.  This function is used for
   intern already allocated value.  */
void *
hash_alloc_intern (void *arg)
{
  return arg;
}

/* Lookup and return hash backet in hash.  If there is no
   corresponding hash backet and alloc_func is specified, create new
   hash backet.  */
void *
hash_get (struct hash *hash, void *data, void * (*alloc_func) ())
{
  u_int64_t key = 0;
  u_int32_t index = 0;
  void *newdata;
  struct hash_backet *backet;

  if (!data)
  {
    // Data is NULL
    return NULL;
  }

  if (hash->hash_key)
  {
    key = (*hash->hash_key) (data);
  }
  else if (hash->hash64_key)
  {
    key = (*hash->hash64_key) (data);
  }

  index = key % hash->size;

  if (hash->count > 0)
    {
      for (backet = hash->index[index]; backet != NULL; backet = backet->next)
        if (backet->key == key
            && (*hash->hash_cmp) (backet->data, data) == 1)
          return backet->data;
    }

  if (alloc_func)
    {
      newdata = (*alloc_func) (data);
      if (newdata == NULL)
        return NULL;

      backet = (struct hash_backet *)malloc (sizeof (struct hash_backet));
      memset(backet, 0, sizeof (struct hash_backet));
      backet->data = newdata;
      backet->key = key;
      backet->next = hash->index[index];
      hash->index[index] = backet;
      hash->count++;
      return backet->data;
    }
  return NULL;
}

/* Hash lookup.  */
void *
hash_lookup (struct hash *hash, void *data)
{
  return hash_get (hash, data, NULL);
}

/* This function release registered value from specified hash.  When
   release is successfully finished, return the data pointer in the
   hash backet.  */
void *
hash_release (struct hash *hash, void *data)
{
  void *ret;
  u_int64_t key;
  u_int32_t index;
  struct hash_backet *backet;
  struct hash_backet *pp;

  if (!hash || (hash->count == 0) || !data)
  {
    return NULL;
  }

  if (hash->hash_key)
  {
    key = (*hash->hash_key) (data);
  }
  else if (hash->hash64_key)
  {
    key = (*hash->hash64_key) (data);
  }

  index = key % hash->size;

  for (backet = pp = hash->index[index]; backet; backet = backet->next)
    {
      if (backet->key == key
          && (*hash->hash_cmp) (backet->data, data) == 1)
        {
          if (backet == pp)
            {
              hash->index[index] = backet->next;
              if (hash->index[index] && (hash->count == 1))
                {
                  hash->index[index]->data = NULL;
                }
            }
          else
            {
              pp->next = backet->next;
              if (pp->next && (hash->count == 1))
                {
                  pp->next->data = NULL;
                }
            }

          ret = backet->data;
          backet->data = NULL;
          free (backet);
          backet = NULL;
          if (hash->count > 0)
            {
              hash->count--;
            }
          return ret;
        }
      if (backet)
        {
          pp = backet;
        }
      else
        {
          backet = pp;
        }
    }
  return NULL;
}

/* This function release matched data from the hash, additionaly
   clean released data if needed */
void
hash_iterate_release (struct hash *hash,
                      bool (*func) (struct hash_backet *, void *),
                      void *arg,
                      void (*free_func) (void *))
{
  void *ret;
  u_int32_t index;
  struct hash_backet *backet;
  struct hash_backet *pp;

  if (!hash || (hash->count == 0))
  {
    return;
  }

  for (index = 0; (hash->count > 0) && (index < hash->size); index++)
    {
      for (backet = pp = hash->index[index]; (hash->count > 0) && backet;
           backet = backet->next)
        {
          if ((*func) (backet, arg) == 1)
            {
              if (backet == pp)
                {
                  hash->index[index] = backet->next;
                  pp = backet->next;
                  if (hash->index[index] && (hash->count == 1))
                    {
                      hash->index[index]->data = NULL;
                    }
                }
              else
                {
                  pp->next = backet->next;
                  if (pp->next && (hash->count == 1))
                    {
                      pp->next->data = NULL;
                    }
                }

              ret = backet->data;
              backet->data = NULL;
              free (backet);
              backet = NULL;
              if (hash->count > 0)
                {
                  hash->count--;
                }

              if (free_func)
                (*free_func) (ret);
            }

          if (backet)
            {
              pp = backet;
            }
          else if (pp)
            {
              backet = pp;
            }
          else
            {
              /* no more backets left in this index */
              break;
            }
        }
    }
}

/* Iterator function for hash.  */
void
hash_iterate (struct hash *hash,
              void (*func) (struct hash_backet *, void *), void *arg)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg);
        nodes_to_visit--;
      }
}

/* Iterator function for hash with 2 args  */
void
hash_iterate2 (struct hash *hash,
               void (*func) (struct hash_backet *, void *, void *),
               void *arg1, void *arg2)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg1, arg2);
        nodes_to_visit--;
      }
}

/* Iterator function for hash with 3 args  */
void
hash_iterate3 (struct hash *hash,
               void (*func) (struct hash_backet *, void *, void *, void *),
               void *arg1, void *arg2, void *arg3)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg1, arg2, arg3);
        nodes_to_visit--;
      }
}

/* Iterator function for hash with 4 args  */
void
hash_iterate4 (struct hash *hash,
       void (*func) (struct hash_backet *, void *, void *, void *, void *),
       void *arg1, void *arg2, void *arg3, void *arg4)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4);
        nodes_to_visit--;
      }
}

/* Iterator function for hash with 5 args  */
void
hash_iterate5 (struct hash *hash,
       void (*func) (struct hash_backet *,void *,void *,void *,void *,void*),
       void *arg1, void *arg2, void *arg3, void *arg4, void *arg5)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4, arg5);
        nodes_to_visit--;
      }
}

/* Iterator function for hash with 6 args  */
void
hash_iterate6 (struct hash *hash,
       void (*func) (struct hash_backet *,void *,void *,void *,void *,void*, void*),
       void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4, arg5,arg6);
        nodes_to_visit--;
      }
}

/* Iterator function for hash with 7 args  */
void
hash_iterate7 (struct hash *hash,
               void (*func) (struct hash_backet *, void *, void *, void *,
               void *, void *, void *, void *),
               void *arg1, void *arg2, void *arg3, void *arg4, void *arg5,
               void *arg6, void *arg7)
{
  struct hash_backet *hb;
  struct hash_backet *hb_next;
  int i;
  int nodes_to_visit = hash->count;

  if (hash == NULL || func == NULL)
    return;

  for (i = 0; (i < hash->size) && (nodes_to_visit > 0); i++)
    for (hb = hash->index[i]; hb; hb = hb_next)
      {
        hb_next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        nodes_to_visit--;
      }
}

/* Clean up hash.  */
void
hash_clean (struct hash *hash, void (*free_func) (void *))
{
  int i;
  struct hash_backet *hb;
  struct hash_backet *next;

  for (i = 0; (i < hash->size) && hash->count; i++)
    {
      for (hb = hash->index[i]; hb; hb = next)
        {
          next = hb->next;

          if (free_func)
            (*free_func) (hb->data);

          free (hb);
          hash->count--;
        }
      hash->index[i] = NULL;
    }
}

/* Free hash memory.  You may call hash_clean before call this
   function.  */
void
hash_free (struct hash *hash)
{
  free(hash->index);
  free(hash);
}

/* Iterator function for hash entry delete.  */
void
hash_iterate_delete (struct hash *hash,
                     void (*func) (struct hash_backet *, void *), void *arg)
{
  struct hash_backet *next = NULL;
  struct hash_backet *hb = NULL;
  int i;

  for (i = 0; (i < hash->size) && hash->count; i++)
    for (hb = hash->index[i]; hb; hb = next)
      {
        next = hb->next;
        (*func) (hb, arg);
      }
}

/* Iterator function for hash entry delete with 2 args  */
void
hash_iterate_delete2 (struct hash *hash,
                      void (*func) (struct hash_backet *, void *, void *),
                      void *arg1, void *arg2)
{
  struct hash_backet *next = NULL;
  struct hash_backet *hb = NULL;
  int i;

  for (i = 0; (i < hash->size) && hash->count; i++)
    for (hb = hash->index[i]; hb; hb = next)
      {
        next = hb->next;
        (*func) (hb, arg1, arg2);
      }
}

/* Iterator function for hash entry delete with 3 args  */
void
hash_iterate_delete3 (struct hash *hash,
                      void (*func) (struct hash_backet *, \
                                    void *, void *, void *),
                      void *arg1, void *arg2, void *arg3)
{
  struct hash_backet *next = NULL;
  struct hash_backet *hb = NULL;
  int i;

  for (i = 0; (i < hash->size) && hash->count; i++)
    for (hb = hash->index[i]; hb; hb = next)
      {
        next = hb->next;
        (*func) (hb, arg1, arg2, arg3);
      }
}

/* Iterator function for hash entry delete with 4 args  */
void
hash_iterate_delete4 (struct hash *hash,
                      void (*func) (struct hash_backet *, \
                                    void *, void *, void *, void *),
                      void *arg1, void *arg2, void *arg3, void *arg4)
{
  struct hash_backet *next = NULL;
  struct hash_backet *hb = NULL;
  int i;

  for (i = 0; (i < hash->size) && hash->count; i++)
    for (hb = hash->index[i]; hb; hb = next)
      {
        next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4);
      }
}

/* Iterator function for hash entry delete with 5 args  */
void
hash_iterate_delete5 (struct hash *hash,
                      void (*func) (struct hash_backet *, \
                                    void *, void *, void *, void *, void *),
                      void *arg1, void *arg2, void *arg3, void *arg4, void *arg5)
{
  struct hash_backet *next = NULL;
  struct hash_backet *hb = NULL;
  int i;

  for (i = 0; (i < hash->size) && hash->count; i++)
    for (hb = hash->index[i]; hb; hb = next)
      {
        next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4, arg5);
      }
}

/* Iterator function for hash entry delete with 6 args  */
void
hash_iterate_delete6 (struct hash *hash,
                      void (*func) (struct hash_backet *, \
                                    void *, void *, void *, void *, void *, void *),
                      void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6)
{
  struct hash_backet *next = NULL;
  struct hash_backet *hb = NULL;
  int i;

  for (i = 0; (i < hash->size) && hash->count; i++)
    for (hb = hash->index[i]; hb; hb = next)
      {
        next = hb->next;
        (*func) (hb, arg1, arg2, arg3, arg4, arg5, arg6);
      }
}

/* function input hash table and output is first data node */
void *
hash_get_first_data (struct hash *hash)
{
  u_int32_t index;
  struct hash_backet *backet;

  if (!hash || !hash->count)
    {
      return NULL;
    }

  for (index = 0; index < hash->size; index++)
    {
      for (backet = hash->index[index]; backet != NULL; backet = backet->next)
        {
          return backet->data;
        }
    }
  return NULL;
}

/* This function takes data as input and output is next data node */
void *
hash_get_next_data (struct hash *hash, void *data)
{
  u_int64_t key = 0;
  u_int32_t index;
  bool go_next = 0;
  struct hash_backet *backet;

  if (!hash || !data)
    {
      return NULL;
    }

  if (hash->hash_key)
  {
    key = (*hash->hash_key) (data);
  }
  else if (hash->hash64_key)
  {
    key = (*hash->hash64_key) (data);
  }

  index = key % hash->size;

  for (; index < hash->size; index++)
    {
      for (backet = hash->index[index]; backet != NULL; backet = backet->next)
        {
          if (go_next == 1)
            {
              return backet->data;
            }

          /* Ignore first, search for next one */
          if (backet->key == key
              && ((*hash->hash_cmp) (backet->data, data) == 1))
            {
              go_next = 1;
            }
        }
    }
  return NULL;
}

/* This function takes key and bucket data as input */
void *
hash_set (struct hash *hash, void *data, void *newdata)
{
  u_int64_t key;
  u_int32_t index;
  struct hash_backet *backet;

  if (! data || ! newdata)
    return NULL;

  if (hash->hash_key)
  {
    key = (*hash->hash_key) (data);
  }
  else if (hash->hash64_key)
  {
    key = (*hash->hash64_key) (data);
  }

  index = key % hash->size;

  for (backet = hash->index[index]; backet != NULL; backet = backet->next)
    if (backet->key == key
        && (*hash->hash_cmp) (backet->data, data) == 1)
      return backet->data;


   backet = (struct hash_backet *)malloc (sizeof (struct hash_backet));
   if (! backet)
     return NULL;

   memset(backet, 0, sizeof (struct hash_backet));
   backet->data = newdata;
   backet->key = key;
   backet->next = hash->index[index];
   hash->index[index] = backet;
   hash->count++;
   return backet->data;
}

u_int32_t
hash_key_make (char *name)
{
  int i, len;
  u_int32_t key;

  if (name)
    {
      key = 0;
      len = strlen (name);
      for (i = 1; i <= len; i++)
        key += (name[i] * i);

      return key;
    }
  return 0;
}

