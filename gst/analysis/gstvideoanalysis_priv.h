#ifndef GSTVIDEOANALYSIS_PRIV
#define GSTVIDEOANALYSIS_PRIV

#include <glib.h>
#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"

typedef enum
{
  BLACK,
  LUMA,
  FREEZE,
  DIFF,
  BLOCKY,
  VIDEO_PARAM_NUMBER
} VIDEO_PARAMETER;

struct video_data_ctx
{
  struct flags *errs[VIDEO_PARAM_NUMBER];
  struct point *current[VIDEO_PARAM_NUMBER];
  guint *point_counter[VIDEO_PARAM_NUMBER];
  guint limit;
  /* flags [PARAM_NUMBER] data [PARAM_NUMBER] */
  void *ptr;
};

static inline const char *
video_param_to_string (VIDEO_PARAMETER p)
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

static inline gboolean
video_param_boundary_is_upper (VIDEO_PARAMETER p)
{
  return (p != LUMA) && (p != DIFF);
}

static inline void
video_data_ctx_init (struct video_data_ctx *ctx)
{
  ctx->ptr = NULL;
}

static inline void
video_data_ctx_reset (struct video_data_ctx *ctx, guint32 length)
{
  void *data_ptr;

  if (ctx->ptr != NULL)
    free (ctx->ptr);

  ctx->limit = length;

  ctx->ptr = malloc (sizeof (struct flags) * VIDEO_PARAM_NUMBER
      + (sizeof (struct data) +
          length * sizeof (struct point)) * VIDEO_PARAM_NUMBER);

  for (int i = 0; i < VIDEO_PARAM_NUMBER; i++) {
    ctx->errs[i] = ctx->ptr + i * sizeof (struct flags);
  }

  data_ptr = ctx->ptr + VIDEO_PARAM_NUMBER * sizeof (struct flags);

  for (int i = 0; i < VIDEO_PARAM_NUMBER; i++) {
    ctx->current[i] = ((struct data *) data_ptr)->values;
    ctx->point_counter[i] = &((struct data *) data_ptr)->meaningful;
    ((struct data *) data_ptr)->length = length;
    ((struct data *) data_ptr)->meaningful = 0;
    data_ptr += sizeof (struct data) + sizeof (struct point) * length;
  }
}

static inline void
video_data_ctx_delete (struct video_data_ctx *ctx)
{
  if (ctx->ptr != NULL)
    free (ctx->ptr);

  ctx->ptr = NULL;
}

static inline void
video_data_ctx_add_point (struct video_data_ctx *ctx, VIDEO_PARAMETER p,
    double v, gint64 t)
{
  assert (ctx->ptr != NULL);

  if (G_UNLIKELY (*ctx->point_counter[p] >= ctx->limit))
    return;                     /* TODO assert? */

  ctx->current[p]->time = t;
  ctx->current[p]->data = v;
  ctx->current[p]++;
  (*ctx->point_counter[p])++;
}

/* invalidates the internal pointer */
static inline void *
video_data_ctx_pull_out_data (struct video_data_ctx *ctx, size_t *size)
{
  size_t sz = 0;
  void *data = ctx->ptr;

  assert (ctx->ptr != NULL);

  {
    sz += sizeof (struct flags) * VIDEO_PARAM_NUMBER;

    /* Assume all measurments are of the same size */
    sz +=
        (sizeof (struct data) +
        ctx->limit * sizeof (struct point)) * VIDEO_PARAM_NUMBER;
  }

  ctx->ptr = NULL;

  *size = sz;

  return data;
}

static inline void
video_data_ctx_flags_cmp (struct video_data_ctx *ctx,
    VIDEO_PARAMETER param,
    struct boundary *bounds,
    gboolean upper, float *dur, float dur_d, double val)
{
  gboolean peak = FALSE, cont = FALSE;
  struct flags *flags = ctx->errs[param];

  if (bounds->peak_en)
    peak = upper ? (val > bounds->peak) : (val < bounds->peak);

  if (bounds->cont_en) {
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

/*
void err_reset (Error*, guint);
void err_add_timestamp (Error e [PARAM_NUMBER], gint64 t);
void err_add_params (Error e [PARAM_NUMBER], VideoParams*);
gpointer err_dump (Error e [PARAM_NUMBER]);

/                  flags    boundaries upper_bound? duration duration_d value    /
void err_flags_cmp (Error*, BOUNDARY*, gboolean, float*, float, float);
*/


#endif
