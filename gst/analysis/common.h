#ifndef COMMON_H
#define COMMON_H

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

#endif
