#ifndef GSTAUDIOANALYSIS_PRIV
#define GSTAUDIOANALYSIS_PRIV

#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>
#include <assert.h>

typedef enum
{
  SILENCE_SHORTT,
  SILENCE_MOMENT,
  LOUDNESS_SHORTT,
  LOUDNESS_MOMENT,
  PARAM_NUMBER
} PARAMETER;

#define IS_MOMENT(p) ((p) & 1)

typedef enum
{
  SHORTT,
  MOMENT,
  MEAS_NUMBER,
} MEASURMENT;

struct boundary
{
  gboolean cont_en;
  gfloat cont;
  gboolean peak_en;
  gfloat peak;
  gfloat duration;
};

struct point
{
  gint64 time;
  double data;
};

struct flag
{
  gboolean value;
  gint64 timestamp;
  gint64 span;
};

struct flags
{
  struct flag cont_flag;
  struct flag peak_flag;
};

struct data
{
  guint32 length;
  guint32 meaningful;
  struct point values[];
};

struct data_ctx
{
  struct flags *errs[PARAM_NUMBER];
  struct point *current[MEAS_NUMBER];
  guint *point_counter[MEAS_NUMBER];
  guint limit;
  /* flags [PARAM_NUMBER] data [MEAS_NUMBER] */
  void *ptr;
};

static inline const char *
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

static inline gboolean
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

static inline void
data_ctx_init (struct data_ctx *ctx)
{
  ctx->ptr = NULL;
}

static inline void
data_ctx_reset (struct data_ctx *ctx, guint32 length)
{
  void *data_ptr;

  if (ctx->ptr != NULL)
    free (ctx->ptr);

  ctx->limit = length;

  ctx->ptr = malloc (sizeof (struct flags) * PARAM_NUMBER
      + (sizeof (struct data) + length * sizeof (struct point)) * MEAS_NUMBER);

  for (int i = 0; i < PARAM_NUMBER; i++) {
    ctx->errs[i] = ctx->ptr + i * sizeof (struct flags);
  }

  data_ptr = ctx->ptr + PARAM_NUMBER * sizeof (struct flags);

  for (int i = 0; i < MEAS_NUMBER; i++) {
    ctx->current[i] = ((struct data *) data_ptr)->values;
    ctx->point_counter[i] = &((struct data *) data_ptr)->meaningful;
    ((struct data *) data_ptr)->length = length;
    ((struct data *) data_ptr)->meaningful = 0;
    data_ptr += sizeof (struct data) + sizeof (struct point) * length;
  }
}

static inline void
data_ctx_delete (struct data_ctx *ctx)
{
  if (ctx->ptr != NULL)
    free (ctx->ptr);

  ctx->ptr = NULL;
}

static inline void
data_ctx_add_point (struct data_ctx *ctx, MEASURMENT meas, double v, gint64 t)
{
  assert (ctx->ptr != NULL);

  if (G_UNLIKELY (*ctx->point_counter[meas] >= ctx->limit))
    return;                     /* TODO assert? */

  ctx->current[meas]->time = t;
  ctx->current[meas]->data = v;
  ctx->current[meas]++;
  (*ctx->point_counter[meas])++;
}

/* invalidates the internal pointer */
static inline void *
data_ctx_pull_out_data (struct data_ctx *ctx, size_t *size)
{
  size_t sz = 0;
  void *data = ctx->ptr;

  assert (ctx->ptr != NULL);

  {
    sz += sizeof (struct flags) * PARAM_NUMBER;

    /* Assume all measurments are of the same size */
    sz +=
        (sizeof (struct data) +
        ctx->limit * sizeof (struct point)) * MEAS_NUMBER;
  }

  ctx->ptr = NULL;

  *size = sz;

  return data;
}

static inline void
data_ctx_flags_cmp (struct data_ctx *ctx, PARAMETER param,
    struct boundary *bounds, gboolean upper,
    float *dur, float dur_d, double val)
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

#endif
