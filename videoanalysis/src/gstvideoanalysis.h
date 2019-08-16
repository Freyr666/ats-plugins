/*
 * TODO copyright
 */

#ifndef _GSTVIDEOANALYSIS_
#define _GSTVIDEOANALYSIS_

#include <gst/gl/gl.h>
#include <gst/gl/gstglbasefilter.h>
#include <GL/gl.h>
#include <stdatomic.h>

#include "error.h"

#define MAX_LATENCY 24

G_BEGIN_DECLS

GType gst_videoanalysis_get_type (void);
#define GST_TYPE_VIDEOANALYSIS                  \
  (gst_videoanalysis_get_type())
#define GST_VIDEOANALYSIS(obj)                                          \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysis))
#define GST_VIDEOANALYSIS_CLASS(klass)                                  \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysisClass))
#define GST_IS_VIDEOANALYSIS(obj)                               \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEOANALYSIS))
#define GST_IS_VIDEOANALYSIS_CLASS(obj)                         \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEOANALYSIS))
#define GST_VIDEOANALYSIS_GET_CLASS(obj)                                \
  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_VIDEOANALYSIS,GstVideoAnalysisClass))

typedef struct _GstVideoAnalysis GstVideoAnalysis;
typedef struct _GstVideoAnalysisClass GstVideoAnalysisClass;

struct state {
  gfloat        cont_err_duration [PARAM_NUMBER];
  gint64        cont_err_past_timestamp [PARAM_NUMBER];
};

struct _GstVideoAnalysis
{
  GstGLBaseFilter    parent;

  /* Stream-loss detection task */
  atomic_bool        got_frame;
  atomic_bool        task_should_run;
  GRecMutex          task_lock;

  GstTask          * timeout_task;

  /* GL-related stuff */
  guint              buffer_ptr;
  GLuint             buffer [MAX_LATENCY];
  gboolean           gl_settings_unchecked;

  GstGLShader      * shader;
  GstGLShader      * shader_block;
  //GstGLShader *      shader_accum;
  /* Textures */
  GstGLMemory      * tex;
  GstBuffer        * prev_buffer;
  GstGLMemory      * prev_tex;

  /* VideoInfo */
  GstVideoInfo       in_info;
  GstVideoInfo       out_info;

  /* Video params evaluation */
  gint64             time_now_us;
  struct state       error_state;
  struct data_ctx    errors;
  GstClockTime       next_data_message_ts;
  gint64             frame_duration_us;
  double             frame_duration_double;
  guint64            frames_prealloc_per_s;
  
  float       fps_period;
  guint       frames_in_sec;

  /* Interm values */
  struct accumulator *acc_buffer;
        
  /* Parameters */
  guint              latency;
  guint              timeout;
  guint              period;
  guint              black_pixel_lb;
  guint              pixel_diff_lb;
  struct boundary    params_boundary [PARAM_NUMBER];
};

struct _GstVideoAnalysisClass
{
  GstGLBaseFilterClass parent_class;

  void (*data_signal) (GstVideoFilter *filter, GstBuffer* d);
  void (*stream_lost_signal) (GstVideoFilter *filter);
  void (*stream_found_signal) (GstVideoFilter *filter);
};

static inline void
update_all_timestamps(struct state * state, gint64 ts)
{
  for (int n = 0; n < PARAM_NUMBER; n++)
    state->cont_err_past_timestamp[n] = ts;
}

static inline void
update_timestamp(struct state * state, PARAMETER param, gint64 ts)
{
  state->cont_err_past_timestamp[param] = ts;
}

static inline gint64
get_timestamp(struct state * state, PARAMETER param)
{
  return state->cont_err_past_timestamp[param];
}

G_END_DECLS

#endif /* _GSTVIDEOANALYSIS_ */
