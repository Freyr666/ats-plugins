#include "error.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>

const char*
param_to_string (PARAMETER p)
{
  switch (p) {
  case BLACK:
    return "black";
    break;
  case LUMA:
    return "luma";
    break;
  case FREEZE:
    return "freeze";
    break;
  case DIFF:
    return "diff";
    break;
  case BLOCKY:
    return "blocky";
    break;
  default:
    return "unknown";
    break;
  }
}

gboolean
param_boundary_is_upper (PARAMETER p)
{
  return (p != LUMA) && (p != DIFF);
}

void
data_ctx_init (struct data_ctx * ctx)
{
  ctx->ptr = NULL;
}

void
data_ctx_reset (struct data_ctx * ctx,
                guint32 length)
{
  void * data_ptr;
  
  if (ctx->ptr != NULL)
    free (ctx->ptr);

  ctx->limit = length;

  ctx->ptr = malloc (sizeof (struct flags) * PARAM_NUMBER
                     + (sizeof (struct data) + length * sizeof (struct point)) * PARAM_NUMBER);

  for (int i = 0; i < PARAM_NUMBER; i++) {
    ctx->errs[i] = ctx->ptr + i * sizeof(struct flags);
  }

  data_ptr = ctx->ptr + PARAM_NUMBER * sizeof(struct flags);
  
  for (int i = 0; i < PARAM_NUMBER; i++) {
    ctx->current[i] = ((struct data*)data_ptr)->values;
    ctx->point_counter[i] = &((struct data*)data_ptr)->meaningful;
    ((struct data*)data_ptr)->length = length;
    ((struct data*)data_ptr)->meaningful = 0;
    data_ptr += sizeof(struct data) + sizeof(struct point) * length;
  }
}

void
data_ctx_delete (struct data_ctx * ctx)
{
  if (ctx->ptr != NULL)
    free (ctx->ptr);

  ctx->ptr = NULL;
}

void
data_ctx_add_point (struct data_ctx * ctx,
                    PARAMETER p,
                    double v,
                    gint64 t)
{
  assert (ctx->ptr != NULL);

  if (G_UNLIKELY (*ctx->point_counter[p] >= ctx->limit))
    return; /* TODO assert? */
    
  ctx->current[p]->time = t;
  ctx->current[p]->data = v;
  ctx->current[p]++;
  (*ctx->point_counter[p])++;
}

void *
data_ctx_pull_out_data (struct data_ctx * ctx,
                        size_t * size)
{
  size_t sz = 0;
  void * data = ctx->ptr;

  assert (ctx->ptr != NULL);

  {
    sz += sizeof (struct flags) * PARAM_NUMBER;

    /* Assume all measurments are of the same size */
    sz += (sizeof (struct data) + ctx->limit * sizeof (struct point)) * PARAM_NUMBER;
  }
  
  ctx->ptr = NULL;

  *size = sz;
  
  return data;
}

void
data_ctx_flags_cmp (struct data_ctx * ctx,
                    PARAMETER param,
                    struct boundary * bounds,
                    gboolean upper,
                    float * dur,
                    float dur_d,
                    double val)
{
  gboolean peak = FALSE, cont = FALSE;
  struct flags * flags = ctx->errs [param];
  
  if (bounds->peak_en)
    peak = upper ? (val > bounds->peak) : (val < bounds->peak);

  if (bounds->cont_en)
    {
      gboolean b = upper ? (val > bounds->cont) : (val < bounds->cont);
      if (b) {
        *dur += dur_d;
      } else {
        *dur = 0.0;
      }
      cont = *dur > bounds->duration;
    }
  if (peak)
    flags->peak_flag.value = peak;
  if (cont)
    flags->cont_flag.value = cont;
}
