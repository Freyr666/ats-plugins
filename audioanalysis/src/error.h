#ifndef ERROR_H
#define ERROR_H

#include <gst/gst.h>
#include <glib.h>

typedef enum {
  SILENCE_SHORTT,
  SILENCE_MOMENT,
  LOUDNESS_SHORTT,
  LOUDNESS_MOMENT,
  PARAM_NUMBER
} PARAMETER;

#define IS_MOMENT(p) ((p) & 1)

typedef enum {
  SHORTT,
  MOMENT,
  MEAS_NUMBER,
} MEASURMENT;

struct boundary {
  gboolean cont_en;
  gfloat   cont;
  gboolean peak_en;
  gfloat   peak;
  gfloat   duration;
};

const char* param_to_string (PARAMETER);

gboolean param_boundary_is_upper (PARAMETER p);

struct point {
  gint64 time;
  double data;
};

struct flag {
  gboolean value;
  gint64   timestamp;
  gint64   span;
};

struct flags {
  struct flag cont_flag;
  struct flag peak_flag;
};

struct data {
  guint32      length;
  guint32      meaningful;
  struct point values [];
};

struct data_ctx {
  struct flags * errs [PARAM_NUMBER];
  struct point * current [MEAS_NUMBER];
  guint * point_counter [MEAS_NUMBER];
  guint limit;
  /* flags [PARAM_NUMBER] data [MEAS_NUMBER] */
  void * ptr;
};

void data_ctx_init (struct data_ctx * ctx);

void data_ctx_reset (struct data_ctx * ctx,
                     guint32 length);

void data_ctx_delete (struct data_ctx * ctx);

void data_ctx_add_point (struct data_ctx * ctx,
                         MEASURMENT meas,
                         double v,
                         gint64 t);

/* invalidates the internal pointer */
void * data_ctx_pull_out_data (struct data_ctx * ctx, size_t * size);

void data_ctx_flags_cmp (struct data_ctx * ctx,
                         PARAMETER param,
                         struct boundary * bounds,
                         gboolean upper,
                         float * dur,
                         float dur_d,
                         double val);

#endif /* ERROR_H */
