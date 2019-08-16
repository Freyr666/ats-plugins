#ifndef ERROR_H
#define ERROR_H

#include <gst/gst.h>
#include <glib.h>

typedef enum {
  BLACK,
  LUMA,
  FREEZE,
  DIFF,
  BLOCKY,
  PARAM_NUMBER
} PARAMETER;

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
  struct point * current [PARAM_NUMBER];
  guint * point_counter [PARAM_NUMBER];
  guint limit;
  /* flags [PARAM_NUMBER] data [PARAM_NUMBER] */
  void * ptr;
};

void data_ctx_init (struct data_ctx * ctx);

void data_ctx_reset (struct data_ctx * ctx,
                     guint32 length);

void data_ctx_delete (struct data_ctx * ctx);

void data_ctx_add_point (struct data_ctx * ctx,
                         PARAMETER meas,
                         double v,
                         gint64 t);

/* invalidates the internal pointer */
void * data_ctx_pull_out_data (struct data_ctx * ctx,
                               size_t * size);

void data_ctx_flags_cmp (struct data_ctx * ctx,
                         PARAMETER param,
                         struct boundary * bounds,
                         gboolean upper,
                         float * dur,
                         float dur_d,
                         double val);

/*
void err_reset (Error*, guint);
void err_add_timestamp (Error e [PARAM_NUMBER], gint64 t);
void err_add_params (Error e [PARAM_NUMBER], VideoParams*);
gpointer err_dump (Error e [PARAM_NUMBER]);

/                  flags    boundaries upper_bound? duration duration_d value    /
void err_flags_cmp (Error*, BOUNDARY*, gboolean, float*, float, float);
*/
#endif /* BOUNDARY_H */
