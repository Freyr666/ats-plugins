/*
 * TODO copyright
 */

#ifndef _GSTVIDEOANALYSIS_
#define _GSTVIDEOANALYSIS_

#include <gst/gl/gl.h>
#include <gst/gl/gstglbasefilter.h>
#include <gst/gl/gstglfuncs.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>

#include "gstvideoanalysis_priv.h"

#define MAX_LATENCY 24

G_BEGIN_DECLS GType gst_videoanalysis_get_type (void);
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

struct video_analysis_state
{
  gfloat cont_err_duration[VIDEO_PARAM_NUMBER];
  gint64 cont_err_past_timestamp[VIDEO_PARAM_NUMBER];
};

struct _GstVideoAnalysis
{
  GstGLBaseFilter parent;

  /* Stream-loss detection task */
  atomic_bool got_frame;
  atomic_bool task_should_run;
  GRecMutex task_lock;

  GstTask *timeout_task;

  /* GL-related stuff */
  guint buffer_ptr;
  GLuint buffer[MAX_LATENCY];
  gboolean gl_settings_unchecked;

  GstGLShader *shader;
  GstGLShader *shader_block;
  //GstGLShader *      shader_accum;
  /* Textures */
  GstGLMemory *tex;
  GstBuffer *prev_buffer;
  GstGLMemory *prev_tex;

  /* VideoInfo */
  GstVideoInfo in_info;
  GstVideoInfo out_info;

  /* Video params evaluation */
  gint64 time_now_us;
  struct video_analysis_state error_state;
  struct video_data_ctx errors;
  GstClockTime next_data_message_ts;
  gint64 frame_duration_us;
  double frame_duration_double;
  guint64 frames_prealloc_per_s;

  float fps_period;
  guint frames_in_sec;

  /* Interm values */
  struct accumulator *acc_buffer;

  /* Parameters */
  guint latency;
  guint timeout;
  guint period;
  guint black_pixel_lb;
  guint pixel_diff_lb;
  struct boundary params_boundary[VIDEO_PARAM_NUMBER];
};

struct _GstVideoAnalysisClass
{
  GstGLBaseFilterClass parent_class;

  void (*data_signal) (GstVideoFilter * filter, GstBuffer * d);
  void (*stream_lost_signal) (GstVideoFilter * filter);
  void (*stream_found_signal) (GstVideoFilter * filter);
};

G_END_DECLS
#endif /* _GSTVIDEOANALYSIS_ */
