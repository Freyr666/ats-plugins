#ifndef __SUPL_ARRAY__
#define __SUPL_ARRAY__

#include <malloc.h>

#define MAKE_ARRAY(TYP)                                                 \
                                                                        \
  struct array_##TYP {                                                  \
    size_t capacity;                                                    \
    size_t len;                                                         \
    TYP * data;                                                         \
  };                                                                    \
                                                                        \
  void                                                                  \
  array_##TYP##_init (struct array_##TYP * a,                           \
                      size_t cap)                                       \
  {                                                                     \
    a->capacity = cap;                                                  \
    a->len = 0;                                                         \
    a->data = (TYP*) malloc (cap * sizeof(TYP));                        \
  }                                                                     \
                                                                        \
  void                                                                  \
  array_##TYP##_free (struct array_##TYP * a)                           \
  {                                                                     \
    a->capacity = 0;                                                    \
    a->len = 0;                                                         \
    if (a->data)                                                        \
      free (a->data);                                                   \
  }                                                                     \
                                                                        \
  void                                                                  \
  array_##TYP##_reset (struct array_##TYP * a)                          \
  {                                                                     \
    a->len = 0;                                                         \
  }                                                                     \
                                                                        \
  void                                                                  \
  array_##TYP##_append (struct array_##TYP * a,                         \
                        TYP v)                                          \
  {                                                                     \
    if (a->len >= a->capacity)                                          \
      a->data = (TYP*) realloc (a->data, (a->capacity + 16) * sizeof(TYP)); \
                                                                        \
    a->data[a->len++] = v;                                              \
  }                                                                     \
                                                                        \
  TYP                                                                   \
  array_##TYP##_get_unsafe (struct array_##TYP * a,                     \
                            size_t ind)                                 \
  {                                                                     \
    return a->data[ind];                                                \
  }                                                                    

#endif /* __SUPL_ARRAY__ */
