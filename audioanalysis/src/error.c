#include "error.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>

const char*
param_to_string (PARAMETER p)
{
        switch (p) {
        case SILENCE_MOMENT:
        case SILENCE_SHORTT:
                return "silence";
                break;
        case LOUDNESS_MOMENT:
        case LOUDNESS_SHORTT:
                return "loudness";
                break;
        default:
                return "unknown";
                break;
        }
}

gboolean
param_boundary_is_upper (PARAMETER p)
{
        switch (p) {
        case SILENCE_MOMENT:
        case SILENCE_SHORTT:
                return FALSE;
                break;
        default:
                return TRUE;
                break;
        }
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
  
  if (ctx->ptr)
    free (ctx->ptr);

  ctx->ptr = malloc (sizeof (struct flags) * PARAM_NUMBER
                     + (sizeof (struct data) + length * sizeof (struct point)) * MEAS_NUMBER);

  for (int i = 0; i < PARAM_NUMBER; i++) {
    ctx->errs[i] = &((struct flags*)ctx->ptr[i]);
  }

  data_ptr = &((struct flags*)ctx->ptr[PARAM_NUMBER]);
  
  for (int i = 0; i < MEAS_NUMBER; i++) {
    ctx->current[i] = (struct data*)data_ptr->points;
    data_ptr += sizeof(struct data) + sizeof(struct point) * length;
  }
}

void
data_ctx_add_point (struct data_ctx * ctx,
                    MEASURMENT meas,
                    double v,
                    gint64 t)
{
  assert (ctx->ptr != NULL);
  
  ctx->point[meas]->time = t;
  ctx->point[meas]->data = v;
  ctx->point[meas]++;
}

void *
data_ctx_pull_out_data (struct data_ctx * ctx,
                        size_t * size)
{
  size_t sz = 0;
  guint32 len;
  void * data = ctx->ptr;
  void * data_tmp_ptr;

  assert (ctx->ptr != NULL);

  {
    sz += sizeof (struct flags) * PARAM_NUMBER;

    data_tmp_ptr = &((struct flags*)ctx->ptr[PARAM_NUMBER]);

    len = (struct flags*)data_tmp_ptr->length;

    /* Assume all measurments are of the same size */
    sz += (sizeof (struct data) + length * sizeof (struct point)) * MEAS_NUMBER;
  }
  
  ctx->ptr = NULL;

  *size = sz;
  
  return data;
}

/* Unfinished */
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
  struct flags * flags = ctx->flags [param];
  
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
    err->peak_flag = peak;
  if (cont)
    err->cont_flag = cont;
}
