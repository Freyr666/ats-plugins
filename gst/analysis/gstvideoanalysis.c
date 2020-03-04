/*
 * TODO copyright
 */

#ifndef LGPL_LIC
#define LIC "Proprietary"
#define URL "http://www.niitv.ru/"
#else
#define LIC "LGPL"
#define URL "https://github.com/Freyr666/ats-analyzer/"
#endif

/**
 * SECTION:element-videoanalysis
 * @title: videoanalysis
 *
 * QoE analysis based on shaders.
 *
 * ## Examples
 * |[
 * gst-launch-1.0 videotestsrc ! glupload ! gstvideoanalysis ! glimagesink
 * ]|
 * FBO (Frame Buffer Object) and GLSL (OpenGL Shading Language) are required.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstvideoanalysis.h"

#define MODULUS(n,m)                            \
  ({                                            \
    __typeof__(n) _n = (n);                     \
    __typeof__(m) _m = (m);                     \
    __typeof__(n) res = _n % _m;                \
    res < 0 ? _m + res : res;                   \
  })

#define GST_CAT_DEFAULT gst_videoanalysis_debug_category
GST_DEBUG_CATEGORY_STATIC (gst_videoanalysis_debug_category);

#define gst_videoanalysis_parent_class parent_class

/* prototypes */

static void gst_videoanalysis_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);

static void gst_videoanalysis_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static void gst_videoanalysis_dispose (GObject * object);

static gboolean gst_videoanalysis_start (GstBaseTransform * trans);

static gboolean gst_videoanalysis_stop (GstBaseTransform * trans);

static gboolean gst_videoanalysis_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);

static GstFlowReturn gst_videoanalysis_transform_ip (GstBaseTransform * filter,
    GstBuffer * inbuf);

static gboolean videoanalysis_apply (GstVideoAnalysis * va, GstGLMemory * mem);

/*
static void gst_videoanalysis_timeout_loop (GstVideoAnalysis * va);
*/
//static gboolean gst_gl_base_filter_find_gl_context (GstGLBaseFilter * filter);

/* signals */
enum
{
  DATA_SIGNAL,
  STREAM_LOST_SIGNAL,
  STREAM_FOUND_SIGNAL,
  LAST_SIGNAL
};

/* args */
enum
{
  PROP_0,
  PROP_TIMEOUT,
  PROP_LATENCY,
  PROP_PERIOD,
  PROP_BLACK_PIXEL_LB,
  PROP_PIXEL_DIFF_LB,
  PROP_BLACK_CONT,
  PROP_BLACK_CONT_EN,
  PROP_BLACK_PEAK,
  PROP_BLACK_PEAK_EN,
  PROP_BLACK_DURATION,
  PROP_LUMA_CONT,
  PROP_LUMA_CONT_EN,
  PROP_LUMA_PEAK,
  PROP_LUMA_PEAK_EN,
  PROP_LUMA_DURATION,
  PROP_FREEZE_CONT,
  PROP_FREEZE_CONT_EN,
  PROP_FREEZE_PEAK,
  PROP_FREEZE_PEAK_EN,
  PROP_FREEZE_DURATION,
  PROP_DIFF_CONT,
  PROP_DIFF_CONT_EN,
  PROP_DIFF_PEAK,
  PROP_DIFF_PEAK_EN,
  PROP_DIFF_DURATION,
  PROP_BLOCKY_CONT,
  PROP_BLOCKY_CONT_EN,
  PROP_BLOCKY_PEAK,
  PROP_BLOCKY_PEAK_EN,
  PROP_BLOCKY_DURATION,
  LAST_PROP
};

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *properties[LAST_PROP] = { NULL, };

/* pad templates */
static const gchar caps_string[] =
    "video/x-raw(memory:GLMemory),format=(string){I420,NV12,NV21,YV12}";

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (GstVideoAnalysis,
    gst_videoanalysis,
    GST_TYPE_GL_BASE_FILTER,
    GST_DEBUG_CATEGORY_INIT (gst_videoanalysis_debug_category,
        "videoanalysis", 0, "debug category for videoanalysis element"));

static inline void
update_all_timestamps (struct video_analysis_state *state, gint64 ts)
{
  for (int n = 0; n < VIDEO_PARAM_NUMBER; n++)
    state->cont_err_past_timestamp[n] = ts;
}

static inline void
update_timestamp (struct video_analysis_state *state, VIDEO_PARAMETER param,
    gint64 ts)
{
  state->cont_err_past_timestamp[param] = ts;
}

static inline gint64
get_timestamp (struct video_analysis_state *state, VIDEO_PARAMETER param)
{
  return state->cont_err_past_timestamp[param];
}

static void
gst_videoanalysis_class_init (GstVideoAnalysisClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);
  GstGLBaseFilterClass *base_filter = GST_GL_BASE_FILTER_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("sink",
          GST_PAD_SINK, GST_PAD_ALWAYS, gst_caps_from_string (caps_string)));
  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("src",
          GST_PAD_SRC, GST_PAD_ALWAYS, gst_caps_from_string (caps_string)));

  gst_element_class_set_static_metadata (element_class,
      "Gstreamer element for video analysis",
      "Video data analysis",
      "filter for video analysis", "freyr <sky_rider_93@mail.ru>");

  gobject_class->set_property = gst_videoanalysis_set_property;
  gobject_class->get_property = gst_videoanalysis_get_property;
  gobject_class->dispose = gst_videoanalysis_dispose;

  base_transform_class->passthrough_on_same_caps = TRUE;
  base_transform_class->transform_ip_on_passthrough = TRUE;
  base_transform_class->transform_ip = gst_videoanalysis_transform_ip;
  base_transform_class->start = gst_videoanalysis_start;
  base_transform_class->stop = gst_videoanalysis_stop;
  base_transform_class->set_caps = gst_videoanalysis_set_caps;
  base_filter->supported_gl_api = GST_GL_API_OPENGL3;

  signals[DATA_SIGNAL] =
      g_signal_new ("data", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstVideoAnalysisClass, data_signal), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, GST_TYPE_BUFFER);
  signals[STREAM_LOST_SIGNAL] =
      g_signal_new ("stream-lost", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstVideoAnalysisClass, stream_lost_signal), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 0);
  signals[STREAM_FOUND_SIGNAL] =
      g_signal_new ("stream-found", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstVideoAnalysisClass,
          stream_found_signal), NULL, NULL, g_cclosure_marshal_generic,
      G_TYPE_NONE, 0);

  properties[PROP_TIMEOUT] =
      g_param_spec_uint ("timeout", "Stream-lost event timeout",
      "Seconds needed for stream-lost being emitted.",
      1, G_MAXUINT, 10, G_PARAM_READWRITE);
  properties[PROP_LATENCY] =
      g_param_spec_uint ("latency", "Latency",
      "Measurment latency (frame), bigger latency may reduce GPU stalling. 1 - instant measurment, 2 - 1-frame latency, etc.",
      1, MAX_LATENCY, 3, G_PARAM_READWRITE);
  properties[PROP_PERIOD] =
      g_param_spec_uint ("period", "Period",
      "Measuring period", 1, 60, 1, G_PARAM_READWRITE);
  properties[PROP_BLACK_PIXEL_LB] =
      g_param_spec_uint ("black_pixel_lb", "Black pixel lb",
      "Black pixel value lower boundary", 0, 256, 16, G_PARAM_READWRITE);
  properties[PROP_PIXEL_DIFF_LB] =
      g_param_spec_uint ("pixel_diff_lb", "Freeze pixel lb",
      "Freeze pixel value lower boundary", 0, 256, 0, G_PARAM_READWRITE);
  properties[PROP_BLACK_CONT] =
      g_param_spec_float ("black_cont", "Black cont err boundary",
      "Black cont err meas", 0., 100.0, 90.0, G_PARAM_READWRITE);
  properties[PROP_BLACK_CONT_EN] =
      g_param_spec_boolean ("black_cont_en", "Black cont err enabled",
      "Enable black cont err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_BLACK_PEAK] =
      g_param_spec_float ("black_peak", "Black peak err boundary",
      "Black peak err meas", 0., 100.0, 100.0, G_PARAM_READWRITE);
  properties[PROP_BLACK_PEAK_EN] =
      g_param_spec_boolean ("black_peak_en", "Black peak err enabled",
      "Enable black peak err meas", FALSE, G_PARAM_READWRITE);
  properties[PROP_BLACK_DURATION] =
      g_param_spec_float ("black_duration", "Black duration boundary",
      "Black err duration", 0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties[PROP_LUMA_CONT] =
      g_param_spec_float ("luma_cont", "Luma cont err boundary",
      "Luma cont err meas", 0., 256.0, 20.0, G_PARAM_READWRITE);
  properties[PROP_LUMA_CONT_EN] =
      g_param_spec_boolean ("luma_cont_en", "Luma cont err enabled",
      "Enable luma cont err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_LUMA_PEAK] =
      g_param_spec_float ("luma_peak", "Luma peak err boundary",
      "Luma peak err meas", 0., 256.0, 17.0, G_PARAM_READWRITE);
  properties[PROP_LUMA_PEAK_EN] =
      g_param_spec_boolean ("luma_peak_en", "Luma peak err enabled",
      "Enable luma peak err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_LUMA_DURATION] =
      g_param_spec_float ("luma_duration", "Luma duration boundary",
      "Luma err duration", 0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties[PROP_FREEZE_CONT] =
      g_param_spec_float ("freeze_cont", "Freeze cont err boundary",
      "Freeze cont err meas", 0., 100.0, 90., G_PARAM_READWRITE);
  properties[PROP_FREEZE_CONT_EN] =
      g_param_spec_boolean ("freeze_cont_en", "Freeze cont err enabled",
      "Enable freeze cont err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_FREEZE_PEAK] =
      g_param_spec_float ("freeze_peak", "Freeze peak err boundary",
      "Freeze peak err meas", 0., 100.0, 100.0, G_PARAM_READWRITE);
  properties[PROP_FREEZE_PEAK_EN] =
      g_param_spec_boolean ("freeze_peak_en", "Freeze peak err enabled",
      "Enable freeze peak err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_FREEZE_DURATION] =
      g_param_spec_float ("freeze_duration", "Freeze duration boundary",
      "Freeze err duration", 0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties[PROP_DIFF_CONT] =
      g_param_spec_float ("diff_cont", "Diff cont err boundary",
      "Diff cont err meas", 0., 256.0, 0.05, G_PARAM_READWRITE);
  properties[PROP_DIFF_CONT_EN] =
      g_param_spec_boolean ("diff_cont_en", "Diff cont err enabled",
      "Enable diff cont err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_DIFF_PEAK] =
      g_param_spec_float ("diff_peak", "Diff peak err boundary",
      "Diff peak err meas", 0., 265.0, 0.01, G_PARAM_READWRITE);
  properties[PROP_DIFF_PEAK_EN] =
      g_param_spec_boolean ("diff_peak_en", "Diff peak err enabled",
      "Enable diff peak err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_DIFF_DURATION] =
      g_param_spec_float ("diff_duration", "Diff duration boundary",
      "Diff err duration", 0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties[PROP_BLOCKY_CONT] =
      g_param_spec_float ("blocky_cont", "Blocky cont err boundary",
      "Blocky cont err meas", 0., 100.0, 10., G_PARAM_READWRITE);
  properties[PROP_BLOCKY_CONT_EN] =
      g_param_spec_boolean ("blocky_cont_en", "Blocky cont err enabled",
      "Enable blocky cont err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_BLOCKY_PEAK] =
      g_param_spec_float ("blocky_peak", "Blocky peak err boundary",
      "Blocky peak err meas", 0., 100.0, 15., G_PARAM_READWRITE);
  properties[PROP_BLOCKY_PEAK_EN] =
      g_param_spec_boolean ("blocky_peak_en", "Blocky peak err enabled",
      "Enable blocky peak err meas", TRUE, G_PARAM_READWRITE);
  properties[PROP_BLOCKY_DURATION] =
      g_param_spec_float ("blocky_duration", "Blocky duration boundary",
      "Blocky err duration", 0., G_MAXFLOAT, 3., G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, LAST_PROP, properties);
}

static void
gst_videoanalysis_init (GstVideoAnalysis * videoanalysis)
{
  videoanalysis->timeout = 10;
  videoanalysis->period = 1;
  videoanalysis->black_pixel_lb = 16;
  videoanalysis->pixel_diff_lb = 0;

  /* TODO cleanup this mess */
  for (guint i = 0; i < VIDEO_PARAM_NUMBER; i++) {
    videoanalysis->params_boundary[i].cont_en = TRUE;   /* TODO FALSE */
    videoanalysis->params_boundary[i].peak_en = TRUE;   /* TODO FALSE */
  }

  videoanalysis->params_boundary[BLACK].cont = 90.00;
  videoanalysis->params_boundary[BLACK].peak = 100.00;
  videoanalysis->params_boundary[BLACK].duration = 2.00;

  videoanalysis->params_boundary[LUMA].cont = 20.00;
  videoanalysis->params_boundary[LUMA].peak = 17.00;
  videoanalysis->params_boundary[LUMA].duration = 2.00;

  videoanalysis->params_boundary[FREEZE].cont = 90.00;
  videoanalysis->params_boundary[FREEZE].peak = 100.00;
  videoanalysis->params_boundary[FREEZE].duration = 2.00;

  videoanalysis->params_boundary[DIFF].cont_en = FALSE; /* TODO FALSE */
  videoanalysis->params_boundary[DIFF].peak_en = FALSE; /* TODO FALSE */

  videoanalysis->params_boundary[DIFF].cont = 0.05;
  videoanalysis->params_boundary[DIFF].peak = 0.02;
  videoanalysis->params_boundary[DIFF].duration = 2.00;

  videoanalysis->params_boundary[BLOCKY].cont = 10.00;
  videoanalysis->params_boundary[BLOCKY].peak = 15.00;
  videoanalysis->params_boundary[BLOCKY].duration = 3.00;

  /* private */
  videoanalysis->latency = 3;
  videoanalysis->buffer_ptr = 0;
  videoanalysis->acc_buffer = NULL;
  videoanalysis->shader = NULL;
  videoanalysis->shader_block = NULL;
  videoanalysis->tex = NULL;
  videoanalysis->prev_buffer = NULL;
  videoanalysis->prev_tex = NULL;
  videoanalysis->gl_settings_unchecked = TRUE;

  for (int i = 0; i < MAX_LATENCY; i++) {
    videoanalysis->buffer[i] = 0;
  }

  video_data_ctx_init (&videoanalysis->errors);
  /*
     g_rec_mutex_init (&videoanalysis->task_lock);
     videoanalysis->timeout_task =
     gst_task_new ((GstTaskFunction) gst_videoanalysis_timeout_loop,
     videoanalysis, NULL);
     gst_task_set_lock (videoanalysis->timeout_task, &videoanalysis->task_lock);
   */
}

static void
gst_videoanalysis_dispose (GObject * object)
{
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

  video_data_ctx_delete (&videoanalysis->errors);

  //gst_object_unref (videoanalysis->timeout_task);
  gst_object_replace (&videoanalysis->shader, NULL);
  gst_object_replace (&videoanalysis->shader_block, NULL);
  gst_buffer_replace (&videoanalysis->prev_buffer, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_videoanalysis_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

  GST_DEBUG_OBJECT (videoanalysis, "set_property");

  switch (property_id) {
    case PROP_TIMEOUT:
      videoanalysis->timeout = g_value_get_uint (value);
      break;
    case PROP_LATENCY:
      videoanalysis->latency = g_value_get_uint (value);
      break;
    case PROP_PERIOD:
      videoanalysis->period = g_value_get_uint (value);
      break;
    case PROP_BLACK_PIXEL_LB:
      videoanalysis->black_pixel_lb = g_value_get_uint (value);
      break;
    case PROP_PIXEL_DIFF_LB:
      videoanalysis->pixel_diff_lb = g_value_get_uint (value);
      break;
    case PROP_BLACK_CONT:
      videoanalysis->params_boundary[BLACK].cont = g_value_get_float (value);
      break;
    case PROP_BLACK_CONT_EN:
      videoanalysis->params_boundary[BLACK].cont_en =
          g_value_get_boolean (value);
      break;
    case PROP_BLACK_PEAK:
      videoanalysis->params_boundary[BLACK].peak = g_value_get_float (value);
      break;
    case PROP_BLACK_PEAK_EN:
      videoanalysis->params_boundary[BLACK].peak_en =
          g_value_get_boolean (value);
      break;
    case PROP_BLACK_DURATION:
      videoanalysis->params_boundary[BLACK].duration =
          g_value_get_float (value);
      break;
    case PROP_LUMA_CONT:
      videoanalysis->params_boundary[LUMA].cont = g_value_get_float (value);
      break;
    case PROP_LUMA_CONT_EN:
      videoanalysis->params_boundary[LUMA].cont_en =
          g_value_get_boolean (value);
      break;
    case PROP_LUMA_PEAK:
      videoanalysis->params_boundary[LUMA].peak = g_value_get_float (value);
      break;
    case PROP_LUMA_PEAK_EN:
      videoanalysis->params_boundary[LUMA].peak_en =
          g_value_get_boolean (value);
      break;
    case PROP_LUMA_DURATION:
      videoanalysis->params_boundary[LUMA].duration = g_value_get_float (value);
      break;
    case PROP_FREEZE_CONT:
      videoanalysis->params_boundary[FREEZE].cont = g_value_get_float (value);
      break;
    case PROP_FREEZE_CONT_EN:
      videoanalysis->params_boundary[FREEZE].cont_en =
          g_value_get_boolean (value);
      break;
    case PROP_FREEZE_PEAK:
      videoanalysis->params_boundary[FREEZE].peak = g_value_get_float (value);
      break;
    case PROP_FREEZE_PEAK_EN:
      videoanalysis->params_boundary[FREEZE].peak_en =
          g_value_get_boolean (value);
      break;
    case PROP_FREEZE_DURATION:
      videoanalysis->params_boundary[FREEZE].duration =
          g_value_get_float (value);
      break;
    case PROP_DIFF_CONT:
      videoanalysis->params_boundary[DIFF].cont = g_value_get_float (value);
      break;
    case PROP_DIFF_CONT_EN:
      videoanalysis->params_boundary[DIFF].cont_en =
          g_value_get_boolean (value);
      break;
    case PROP_DIFF_PEAK:
      videoanalysis->params_boundary[DIFF].peak = g_value_get_float (value);
      break;
    case PROP_DIFF_PEAK_EN:
      videoanalysis->params_boundary[DIFF].peak_en =
          g_value_get_boolean (value);
      break;
    case PROP_DIFF_DURATION:
      videoanalysis->params_boundary[DIFF].duration = g_value_get_float (value);
      break;
    case PROP_BLOCKY_CONT:
      videoanalysis->params_boundary[BLOCKY].cont = g_value_get_float (value);
      break;
    case PROP_BLOCKY_CONT_EN:
      videoanalysis->params_boundary[BLOCKY].cont_en =
          g_value_get_boolean (value);
      break;
    case PROP_BLOCKY_PEAK:
      videoanalysis->params_boundary[BLOCKY].peak = g_value_get_float (value);
      break;
    case PROP_BLOCKY_PEAK_EN:
      videoanalysis->params_boundary[BLOCKY].peak_en =
          g_value_get_boolean (value);
      break;
    case PROP_BLOCKY_DURATION:
      videoanalysis->params_boundary[BLOCKY].duration =
          g_value_get_float (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_videoanalysis_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

  GST_DEBUG_OBJECT (videoanalysis, "get_property");

  switch (property_id) {
    case PROP_TIMEOUT:
      g_value_set_uint (value, videoanalysis->timeout);
      break;
    case PROP_LATENCY:
      g_value_set_uint (value, videoanalysis->latency);
      break;
    case PROP_PERIOD:
      g_value_set_uint (value, videoanalysis->period);
      break;
    case PROP_BLACK_PIXEL_LB:
      g_value_set_uint (value, videoanalysis->black_pixel_lb);
      break;
    case PROP_PIXEL_DIFF_LB:
      g_value_set_uint (value, videoanalysis->pixel_diff_lb);
      break;
    case PROP_BLACK_CONT:
      g_value_set_float (value, videoanalysis->params_boundary[BLACK].cont);
      break;
    case PROP_BLACK_CONT_EN:
      g_value_set_boolean (value,
          videoanalysis->params_boundary[BLACK].cont_en);
      break;
    case PROP_BLACK_PEAK:
      g_value_set_float (value, videoanalysis->params_boundary[BLACK].peak);
      break;
    case PROP_BLACK_PEAK_EN:
      g_value_set_boolean (value,
          videoanalysis->params_boundary[BLACK].peak_en);
      break;
    case PROP_BLACK_DURATION:
      g_value_set_float (value, videoanalysis->params_boundary[BLACK].duration);
      break;
    case PROP_LUMA_CONT:
      g_value_set_float (value, videoanalysis->params_boundary[LUMA].cont);
      break;
    case PROP_LUMA_CONT_EN:
      g_value_set_boolean (value, videoanalysis->params_boundary[LUMA].cont_en);
      break;
    case PROP_LUMA_PEAK:
      g_value_set_float (value, videoanalysis->params_boundary[LUMA].peak);
      break;
    case PROP_LUMA_PEAK_EN:
      g_value_set_boolean (value, videoanalysis->params_boundary[LUMA].peak_en);
      break;
    case PROP_LUMA_DURATION:
      g_value_set_float (value, videoanalysis->params_boundary[LUMA].duration);
      break;
    case PROP_FREEZE_CONT:
      g_value_set_float (value, videoanalysis->params_boundary[FREEZE].cont);
      break;
    case PROP_FREEZE_CONT_EN:
      g_value_set_boolean (value,
          videoanalysis->params_boundary[FREEZE].cont_en);
      break;
    case PROP_FREEZE_PEAK:
      g_value_set_float (value, videoanalysis->params_boundary[FREEZE].peak);
      break;
    case PROP_FREEZE_PEAK_EN:
      g_value_set_boolean (value,
          videoanalysis->params_boundary[FREEZE].peak_en);
      break;
    case PROP_FREEZE_DURATION:
      g_value_set_float (value,
          videoanalysis->params_boundary[FREEZE].duration);
      break;
    case PROP_DIFF_CONT:
      g_value_set_float (value, videoanalysis->params_boundary[DIFF].cont);
      break;
    case PROP_DIFF_CONT_EN:
      g_value_set_boolean (value, videoanalysis->params_boundary[DIFF].cont_en);
      break;
    case PROP_DIFF_PEAK:
      g_value_set_float (value, videoanalysis->params_boundary[DIFF].peak);
      break;
    case PROP_DIFF_PEAK_EN:
      g_value_set_boolean (value, videoanalysis->params_boundary[DIFF].peak_en);
      break;
    case PROP_DIFF_DURATION:
      g_value_set_float (value, videoanalysis->params_boundary[DIFF].duration);
      break;
    case PROP_BLOCKY_CONT:
      g_value_set_float (value, videoanalysis->params_boundary[BLOCKY].cont);
      break;
    case PROP_BLOCKY_CONT_EN:
      g_value_set_boolean (value,
          videoanalysis->params_boundary[BLOCKY].cont_en);
      break;
    case PROP_BLOCKY_PEAK:
      g_value_set_float (value, videoanalysis->params_boundary[BLOCKY].peak);
      break;
    case PROP_BLOCKY_PEAK_EN:
      g_value_set_boolean (value,
          videoanalysis->params_boundary[BLOCKY].peak_en);
      break;
    case PROP_BLOCKY_DURATION:
      g_value_set_float (value,
          videoanalysis->params_boundary[BLOCKY].duration);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

struct accumulator
{
  GLfloat frozen;
  GLfloat black;
  GLfloat bright;
  GLfloat diff;
  GLfloat noize;
  GLint visible;
};

static const char *shader_source =
    "#version 430\n"
    "#extension GL_ARB_compute_shader : enable\n"
    "#extension GL_ARB_shader_storage_buffer_object : enable\n" "\n"
    //"precision highp float;\n"
    "\n"
    "#define WHT_LVL 210\n"
    "// 210 //0.90196078\n"
    "#define BLK_LVL 40\n"
    "// 40 //0.15625\n"
    "#define WHT_DIFF 6\n"
    "// 6 //0.0234375\n"
    "#define GRH_DIFF 2\n"
    "// 2 //0.0078125\n"
    "#define KNORM 4\n"
    "#define L_DIFF 5\n"
    "\n"
    "#define BLOCK_SIZE 8\n"
    "\n"
    "struct Noize {\n"
    "        float frozen;\n"
    "        float black;\n"
    "        float bright;\n"
    "        float diff;\n"
    "        float noize;\n"
    "        int   visible;\n"
    "};\n"
    "\n"
    "layout (r8) uniform image2D tex;\n"
    "layout (r8) uniform image2D tex_prev;\n"
    "uniform int width;\n"
    "uniform int height;\n"
    "uniform int stride;\n"
    "uniform int black_bound;\n"
    "uniform int freez_bound;\n"
    "\n"
    "layout (std430, binding=10) buffer Interm {\n"
    "         Noize noize_data [];\n"
    "};\n"
    "\n"
    "layout (local_size_x = BLOCK_SIZE, local_size_y = BLOCK_SIZE, local_size_z = 1) in; \n"
    "\n"
    "        shared int noize;"
    "        shared int bright;"
    "        shared int diff_counter;"
    "        shared int black;"
    "        shared int frozen;"
    "\n"
    "int abs_int(int x) { return x * sign(x); } \n"
    "\n"
    "void main() {\n"
    "        uint  block_pos = (gl_WorkGroupID.y * (width / BLOCK_SIZE)) + gl_WorkGroupID.x;\n"
    "        ivec2 pix_pos   = ivec2(gl_GlobalInvocationID.xy);\n"
    "\n"
    "        /* init shared*/\n"
    "        if (gl_LocalInvocationID.xy == ivec2(0,0)) {\n"
    "             noize = 0;\n"
    "             bright = 0;\n"
    "             diff_counter = 0;\n"
    "             black = 0;\n"
    "             frozen = 0;\n"
    "        }\n"
    "        memoryBarrierShared();"
    "        barrier();"
    "\n"
    "        int   pix       = int(imageLoad(tex, pix_pos).r * 255.0);\n"
    "        int   pix_right = int(imageLoad(tex, ivec2(pix_pos.x + 1, pix_pos.y)).r * 255.0);\n"
    "        int   pix_down  = int(imageLoad(tex, ivec2(pix_pos.x, pix_pos.y + 1)).r * 255.0);\n"
    "        /* Noize */\n"
    "        int   lvl = WHT_DIFF;\n"
    "        if ((pix < WHT_LVL) && (pix > BLK_LVL)) {\n"
    "              lvl = GRH_DIFF;\n"
    "        }\n"
    "        if (abs(pix - pix_right) > lvl) {\n"
    "            atomicAdd(noize, 1);\n"
    "        }\n"
    "        if (abs(pix - pix_down) > lvl) {\n"
    "            atomicAdd(noize, 1);\n"
    "        }\n"
    "        /* Brightness */\n"
    "        atomicAdd(bright, pix);\n"
    "        /* Black */\n"
    "        if (pix <= black_bound) {\n"
    "            atomicAdd(black, 1);\n"
    "        }\n"
    "        /* Diff */\n"
    "        int   pix_prev = int(imageLoad(tex_prev, pix_pos).r * 255.0);\n"
    "        int   diff_pix = abs_int(pix - pix_prev);\n"
    "        atomicAdd(diff_counter, diff_pix);\n" "        /* Frozen */\n"
    //       TODO consider diff_pid = int(abs(imageLoad(tex_prev, pix_pos).r - imageLoad(tex, pix_pos).r) * 255.0)
    //       TODO <=
    "        if (diff_pix < freez_bound) {\n"
    "            atomicAdd(frozen, 1);\n"
    "        }\n"
    "        memoryBarrierShared();"
    "        barrier();"
    "        /* Store results */\n"
    "        if (gl_LocalInvocationID.xy == ivec2(0,0)) {\n"
    "            noize_data[block_pos].noize  = float(noize) / (8.0 * 8.0 * 2.0);\n"
    "            noize_data[block_pos].black  = float(black);\n"
    "            noize_data[block_pos].frozen = float(frozen);\n"
    "            noize_data[block_pos].bright = float(bright) / 256.0;\n"
    "            noize_data[block_pos].diff   = float(diff_counter);\n"
    "            noize_data[block_pos].visible = 0;\n" "        }\n" "}\n";

static const char *shader_source_block =
    "#version 430\n"
    "#extension GL_ARB_compute_shader : enable\n"
    "#extension GL_ARB_shader_image_load_store : enable\n"
    "#extension GL_ARB_shader_storage_buffer_object : enable\n"
    "\n"
    "//precision lowp float;\n"
    "\n"
    "#define WHT_LVL 0.90196078\n"
    "// 210\n"
    "#define BLK_LVL 0.15625\n"
    "// 40\n"
    "#define WHT_DIFF 0.0234375\n"
    "// 6\n"
    "#define GRH_DIFF 0.0078125\n"
    "// 2\n"
    "#define KNORM 4.0\n"
    "#define L_DIFF 5\n"
    "\n"
    "#define BLOCK_SIZE 8\n"
    "\n"
    "struct Noize {\n"
    "        float frozen;\n"
    "        float black;\n"
    "        float bright;\n"
    "        float diff;\n"
    "        float noize;\n"
    "        int   visible;\n"
    "};\n"
    "\n"
    "layout (r8, binding = 0) uniform image2D tex;\n"
    "\n"
    "uniform int width;\n"
    "uniform int height;\n"
    "\n"
    "layout (std430, binding=10) buffer Interm {\n"
    "         Noize noize_data [];\n"
    "};\n"
    "\n"
    "//const uint wht_coef[20] = {8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 11, 11, 12, 14, 17, 27};\n"
    "//const uint ght_coef[20] = {4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 10, 13, 23};\n"
    "//const uint wht_coef[20] = {10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 14, 16, 19, 29};	       \n"
    "//const uint ght_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
    "const uint wht_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
    "const uint ght_coef[20] = {2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 8, 11, 21};\n"
    "\n"
    "/* x = Edge pixel offset, y = Edge ID */\n"
    "layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;\n"
    "\n"
    "float get_coef(float noize, uint array[20]) {\n"
    "        uint ret_val;                                     \n"
    "        if((noize>100) || (noize<0))\n"
    "                ret_val = 0;                              \n"
    "        else                                              \n"
    "                ret_val = array[uint(noize/5)];                 \n"
    "        return float(ret_val/255.0);\n"
    "}\n"
    "\n"
    "int abs_int(int x) { return x * sign(x); } \n"
    "\n"
    "void main() {\n"
    "        uint block_pos = (gl_WorkGroupID.y * (width / BLOCK_SIZE)) + gl_WorkGroupID.x;\n"
    "        /* l d r u */\n"
    "        uint pixel_off = gl_LocalInvocationID.x;\n"
    "        uint edge = gl_LocalInvocationID.y;\n"
    "        /* l (-1, 0) d (0, -1) r (1, 0) u (0, 1) */ \n"
    "        ivec2 edge_off = ivec2 ( mod(edge + 1, 2) * int(edge - 1),\n"
    "                                 mod(edge, 2) * int(edge - 2));\n"
    "\n"
    "        /* Would not compute blocks near the borders */\n"
    "        if (gl_WorkGroupID.x == 0\n"
    "            || gl_WorkGroupID.x >= (width / BLOCK_SIZE)\n"
    "            || gl_WorkGroupID.y == 0\n"
    "            || gl_WorkGroupID.y >= (height / BLOCK_SIZE))\n"
    "                return;\n"
    "\n"
    "        shared int loc_counter;\n"
    "        shared int vis[4];\n"
    "        /* Noize coeffs */\n"
    "        uint pos = block_pos + edge_off.x + (edge_off.y * width / BLOCK_SIZE);\n"
    "        float noize = 100.0 * max(noize_data[block_pos].noize, noize_data[pos].noize);\n"
    "        float white = get_coef(noize, wht_coef);\n"
    "        float grey  = get_coef(noize, ght_coef);\n"
    "\n"
    "        if (gl_LocalInvocationID.x == 0) {\n"
    "\n"
    "                loc_counter = 0;\n"
    "                atomicExchange(vis[edge], 0);\n"
    "\n"
    "        }\n"
    "\n"
    "        ivec2 zero = ivec2(gl_WorkGroupID.x * BLOCK_SIZE,\n"
    "                           gl_WorkGroupID.y * BLOCK_SIZE);\n"
    "        float pixel = imageLoad(tex, ivec2(zero.x + abs_int(edge_off.y) * pixel_off +\n"
    "                                           (edge_off.x == 1 ? (BLOCK_SIZE-1) : 0),\n"
    "                                           zero.y + abs_int(edge_off.x) * pixel_off +\n"
    "                                           (edge_off.y == 1 ? (BLOCK_SIZE-1) : 0))).r;\n"
    "        float prev  = imageLoad(tex, ivec2(zero.x + abs_int(edge_off.y) * pixel_off + \n"
    "                                           (edge_off.x == 1 ? (BLOCK_SIZE-2) : 0) + \n"
    "                                           (edge_off.x == -1 ? 1 : 0), \n"
    "                                           zero.y + abs_int(edge_off.x) * pixel_off + \n"
    "                                           (edge_off.y == 1 ? (BLOCK_SIZE-2) : 0) + \n"
    "                                           (edge_off.y == 1 ? 1 : 0))).r;\n"
    "        float next  = imageLoad(tex, ivec2(zero.x + abs_int(edge_off.y) * pixel_off + \n"
    "                                           (edge_off.x == 1 ? BLOCK_SIZE : 0) + \n"
    "                                           (edge_off.x == -1 ? -1 : 0),\n"
    "                                           zero.y + abs_int(edge_off.x) * pixel_off + \n"
    "                                           (edge_off.y == 1 ? BLOCK_SIZE : 0) + \n"
    "                                           (edge_off.y == 1 ? -1 : 0))).r;\n"
    "        float next_next  = imageLoad(tex, ivec2(zero.x + abs_int(edge_off.y) * pixel_off + \n"
    "                                                (edge_off.x == 1 ? (BLOCK_SIZE+1) : 0) + \n"
    "                                                (edge_off.x == -1 ? -2 : 0), \n"
    "                                                zero.y + abs_int(edge_off.x) * pixel_off + \n"
    "                                                (edge_off.y == 1 ? (BLOCK_SIZE+1) : 0) + \n"
    "                                                (edge_off.y == 1 ? -2 : 0))).r;\n"
    "        float coef = ((pixel < WHT_LVL) && (pixel > BLK_LVL)) ? grey : white;\n"
    "        float denom = round( (abs(prev-pixel) + abs(next-next_next) ) / KNORM);\n"
    "        denom = (denom == 0.0) ? 1.0 : denom;\n"
    "        float norm = abs(next-pixel) / denom;\n"
    "        \n"
    "        if (norm > coef)\n"
    "                atomicAdd(vis[edge], 1);\n"
    "\n"
    "        /* counting visible blocks */\n"
    "\n"
    "        if (gl_LocalInvocationID.x == 0) {\n"
    "                if (vis[edge] > L_DIFF)\n"
    "                        atomicAdd(loc_counter, 1);\n"
    "        }\n"
    "\n"
    "        barrier();\n"
    "\n"
    "        if (gl_LocalInvocationID.xy == ivec2(0,0)) {\n"
    "                if (loc_counter >= 2)\n"
    "                        noize_data[block_pos].visible = 1;\n"
    "        }\n" "}\n";

static void
shader_create (GstGLContext * context, GstVideoAnalysis * va)
{
  GError *error;
  if (!(va->shader =
          gst_gl_shader_new_link_with_stages (context,
              &error,
              gst_glsl_stage_new_with_string (context,
                  GL_COMPUTE_SHADER,
                  GST_GLSL_VERSION_450,
                  GST_GLSL_PROFILE_CORE, shader_source), NULL))) {
    GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
        ("Failed to initialize shader"), (NULL));
  }
  if (!(va->shader_block =
          gst_gl_shader_new_link_with_stages (context,
              &error,
              gst_glsl_stage_new_with_string (context,
                  GL_COMPUTE_SHADER,
                  GST_GLSL_VERSION_450,
                  GST_GLSL_PROFILE_CORE, shader_source_block), NULL))) {
    GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
        ("Failed to initialize shader block"), (NULL));
  }

  for (int i = 0; i < va->latency; i++) {
    if (va->buffer[i]) {
      glDeleteBuffers (1, &va->buffer[i]);
      va->buffer[i] = 0;
    }
    glGenBuffers (1, &va->buffer[i]);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, va->buffer[i]);
    glBufferData (GL_SHADER_STORAGE_BUFFER, (va->in_info.width / 8)
        * (va->in_info.height / 8)
        * sizeof (struct accumulator), NULL, GL_DYNAMIC_COPY);
  }
}

static gboolean
gst_videoanalysis_start (GstBaseTransform * trans)
{
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);

  videoanalysis->time_now_us = g_get_real_time ();
  update_all_timestamps (&videoanalysis->error_state,
      videoanalysis->time_now_us);
  for (int i = 0; i < VIDEO_PARAM_NUMBER; i++)
    videoanalysis->error_state.cont_err_duration[i] = 0.;

  videoanalysis->next_data_message_ts = 0;
  //atomic_store (&videoanalysis->got_frame, FALSE);
  //atomic_store (&videoanalysis->task_should_run, TRUE);

  //gst_task_start (videoanalysis->timeout_task);

  return GST_BASE_TRANSFORM_CLASS (parent_class)->start (trans);
}

static gboolean
gst_videoanalysis_stop (GstBaseTransform * trans)
{
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);

  videoanalysis->tex = NULL;
  videoanalysis->prev_tex = NULL;

  gst_buffer_replace (&videoanalysis->prev_buffer, NULL);
  gst_object_replace (&videoanalysis->shader, NULL);
  gst_object_replace (&videoanalysis->shader_block, NULL);

  //atomic_store (&videoanalysis->task_should_run, FALSE);

  //gst_task_join (videoanalysis->timeout_task);

  return GST_BASE_TRANSFORM_CLASS (parent_class)->stop (trans);
}

static gboolean
gst_videoanalysis_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps)
{
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);

  if (!gst_video_info_from_caps (&videoanalysis->in_info, incaps))
    goto wrong_caps;
  if (!gst_video_info_from_caps (&videoanalysis->out_info, outcaps))
    goto wrong_caps;

  videoanalysis->frame_duration_double =
      (double) videoanalysis->in_info.fps_d /
      (double) videoanalysis->in_info.fps_n;

  videoanalysis->frame_duration_us =
      gst_util_uint64_scale (GST_TIME_AS_USECONDS (GST_SECOND),
      videoanalysis->in_info.fps_d, videoanalysis->in_info.fps_n);

  /* Preallocate number of frames per second + 10% */
  videoanalysis->frames_prealloc_per_s =
      gst_util_uint64_scale (1,
      videoanalysis->in_info.fps_n * 110, videoanalysis->in_info.fps_d * 100);

  video_data_ctx_reset (&videoanalysis->errors,
      (videoanalysis->period * videoanalysis->frames_prealloc_per_s));

  if (videoanalysis->acc_buffer)
    free (videoanalysis->acc_buffer);

  videoanalysis->acc_buffer =
      (struct accumulator *) malloc ((videoanalysis->in_info.width / 8)
      * (videoanalysis->in_info.height / 8)
      * sizeof (struct accumulator));

  return GST_BASE_TRANSFORM_CLASS (parent_class)->set_caps (trans, incaps,
      outcaps);

  /* ERRORS */
wrong_caps:
  GST_WARNING ("Wrong caps - could not understand input or output caps");
  return FALSE;
}

static void
_update_flags_and_timestamps (struct video_data_ctx *ctx,
    struct video_analysis_state *state, gint64 new_ts)
{
  struct flag *current_flag;

  for (int p = 0; p < VIDEO_PARAM_NUMBER; p++) {
    /* Cont flag */
    current_flag = &ctx->errs[p]->cont_flag;

    if (current_flag->value) {
      current_flag->timestamp = get_timestamp (state, p);
      /* TODO check if this is actually true */
      current_flag->span = new_ts - current_flag->timestamp;
    } else
      update_timestamp (state, p, new_ts);

    /* Peak flag */
    current_flag = &ctx->errs[p]->peak_flag;

    if (current_flag->value) {
      current_flag->timestamp = new_ts;
      /* TODO check if this is actually true */
      current_flag->span = GST_SECOND;
    }
  }
}

static void
_set_flags (struct video_data_ctx *ctx,
    struct boundary bounds[VIDEO_PARAM_NUMBER],
    struct video_analysis_state *state, double frame_duration,
    double values[VIDEO_PARAM_NUMBER])
{
  for (int p = 0; p < VIDEO_PARAM_NUMBER; p++) {
    video_data_ctx_flags_cmp (ctx,
        p,
        &bounds[p],
        video_param_boundary_is_upper (p),
        &(state->cont_err_duration[p]), frame_duration, values[p]);
  }
}

static GstFlowReturn
gst_videoanalysis_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstMemory *tex;
  GstVideoFrame gl_frame;
  GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);
  int height = videoanalysis->in_info.height;
  int width = videoanalysis->in_info.width;
  double values[VIDEO_PARAM_NUMBER] = { 0 };

  if (G_UNLIKELY (!gst_pad_is_linked (GST_BASE_TRANSFORM_SRC_PAD (trans))))
    return GST_FLOW_OK;

  if (G_UNLIKELY (videoanalysis->shader == NULL))
    gst_gl_context_thread_add (GST_GL_BASE_FILTER (trans)->context,
        (GstGLContextThreadFunc) shader_create, videoanalysis);

  /* Initialize clocks if needed */
  if (G_UNLIKELY (videoanalysis->next_data_message_ts == 0))
    videoanalysis->next_data_message_ts =
        GST_BUFFER_TIMESTAMP (buf) + (GST_SECOND * videoanalysis->period);

  if (!gst_video_frame_map (&gl_frame,
          &videoanalysis->in_info, buf, GST_MAP_READ | GST_MAP_GL))
    goto inbuf_error;

  /* map[0] corresponds to the Y component of Yuv */
  tex = gl_frame.map[0].memory;

  if (!gst_is_gl_memory (tex)) {
    GST_ERROR_OBJECT (videoanalysis, "Input memory must be GstGLMemory");
    goto unmap_error;
  }

  /* Frame is fine, so inform the timeout_loop task */
  atomic_store (&videoanalysis->got_frame, TRUE);

  /* Evaluate the intermidiate parameters via shader */
  videoanalysis_apply (videoanalysis, GST_GL_MEMORY_CAST (tex));

  /* Merge intermediate values */
  for (int i = 0; i < (width * height / 64); i++) {
    values[FREEZE] += videoanalysis->acc_buffer[i].frozen;
    values[BLACK] += videoanalysis->acc_buffer[i].black;
    values[DIFF] += videoanalysis->acc_buffer[i].diff;
    values[LUMA] += videoanalysis->acc_buffer[i].bright;
    values[BLOCKY] += (float) videoanalysis->acc_buffer[i].visible;
  }

  values[FREEZE] = 100.0 * values[FREEZE] / (width * height);
  values[LUMA] = 255.0 * values[LUMA] / (width * height);
  values[DIFF] = values[DIFF] / (width * height);
  values[BLACK] = 100.0 * values[BLACK] / (width * height);
  values[BLOCKY] = 100.0 * values[BLOCKY] / (width * height / 64);

  //g_print ("Shader Results: [block: %f; luma: %f; black: %f; diff: %f; freeze: %f]\n",
  //          values[BLOCKY], values[LUMA], values[BLACK], values[DIFF], values[FREEZE]);
  //g_print ("Frame: %d Limit: %d\n", videoanalysis->frame, videoanalysis->frame_limit);

  videoanalysis->time_now_us += videoanalysis->frame_duration_us;

  /* errors */
  _set_flags (&videoanalysis->errors,
      videoanalysis->params_boundary,
      &videoanalysis->error_state,
      videoanalysis->frame_duration_double, values);

  for (int p = 0; p < VIDEO_PARAM_NUMBER; p++)
    video_data_ctx_add_point (&videoanalysis->errors,
        p, values[p], videoanalysis->time_now_us);

  /* Send data message if needed */
  if (GST_BUFFER_TIMESTAMP (buf)
      >= videoanalysis->next_data_message_ts) {
    size_t data_size;

    videoanalysis->time_now_us = g_get_real_time ();

    /* Update clock */
    videoanalysis->next_data_message_ts =
        GST_BUFFER_TIMESTAMP (buf) + (GST_SECOND * videoanalysis->period);

    _update_flags_and_timestamps (&videoanalysis->errors,
        &videoanalysis->error_state, videoanalysis->time_now_us);

    gpointer d =
        video_data_ctx_pull_out_data (&videoanalysis->errors, &data_size);

    video_data_ctx_reset (&videoanalysis->errors,
        (videoanalysis->period * videoanalysis->frames_prealloc_per_s));

    GstBuffer *data = gst_buffer_new_wrapped (d, data_size);
    g_signal_emit (videoanalysis, signals[DATA_SIGNAL], 0, data);

    gst_buffer_unref (data);
  }

  gst_video_frame_unmap (&gl_frame);

  /* Cleanup */
  gst_buffer_replace (&videoanalysis->prev_buffer, buf);

  return GST_FLOW_OK;

unmap_error:
  gst_video_frame_unmap (&gl_frame);
inbuf_error:
  return GST_FLOW_ERROR;
}

static void
analyse (GstGLContext * context, GstVideoAnalysis * va)
{
  const GstGLFuncs *gl = context->gl_vtable;
  int width = va->in_info.width;
  int height = va->in_info.height;
  int stride = va->in_info.stride[0];
  struct accumulator *data = NULL;

  glGetError ();

  if (G_LIKELY (va->prev_tex)) {
    gl->ActiveTexture (GL_TEXTURE1);
    gl->BindTexture (GL_TEXTURE_2D,
        gst_gl_memory_get_texture_id (va->prev_tex));
    glBindImageTexture (1, gst_gl_memory_get_texture_id (va->prev_tex), 0,
        GL_FALSE, 0, GL_READ_ONLY, GL_R8);
  }

  gl->ActiveTexture (GL_TEXTURE0);
  gl->BindTexture (GL_TEXTURE_2D, gst_gl_memory_get_texture_id (va->tex));
  glBindImageTexture (0, gst_gl_memory_get_texture_id (va->tex),
      0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

  glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 10, va->buffer[va->buffer_ptr]);

  gst_gl_shader_use (va->shader);

  GLuint prev_ind = va->prev_tex != 0 ? 1 : 0;
  gst_gl_shader_set_uniform_1i (va->shader, "tex", 0);
  gst_gl_shader_set_uniform_1i (va->shader, "tex_prev", prev_ind);
  gst_gl_shader_set_uniform_1i (va->shader, "width", width);
  gst_gl_shader_set_uniform_1i (va->shader, "height", height);
  gst_gl_shader_set_uniform_1i (va->shader, "stride", stride);
  gst_gl_shader_set_uniform_1i (va->shader, "black_bound", va->black_pixel_lb);
  gst_gl_shader_set_uniform_1i (va->shader, "freez_bound", va->pixel_diff_lb);

  glDispatchCompute (width / 8, height / 8, 1);

  glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

  gst_gl_shader_use (va->shader_block);

  gst_gl_shader_set_uniform_1i (va->shader_block, "tex", 0);
  gst_gl_shader_set_uniform_1i (va->shader_block, "width", width);
  gst_gl_shader_set_uniform_1i (va->shader_block, "height", height);

  glDispatchCompute (width / 8, height / 8, 1);

  glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

  /* Get prev results */

  guint prev = MODULUS (((int) va->buffer_ptr - (int) va->latency + 1),
      (int) va->latency);

  glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, va->buffer[prev]);
  //glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, va->buffer[prev],
  //                  0, (width / 8) * (height / 8) * sizeof(struct accumulator));
  data = glMapBufferRange (GL_SHADER_STORAGE_BUFFER,
      0, (width / 8) * (height / 8) * sizeof (struct accumulator),
      GL_MAP_READ_BIT);

  memcpy (va->acc_buffer, data,
      (width / 8) * (height / 8) * sizeof (struct accumulator));

  /* Cleanup */
  glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);

  glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);
  glBindTexture (GL_TEXTURE_2D, 0);

  va->buffer_ptr = MODULUS ((va->buffer_ptr + 1), va->latency);

  va->prev_tex = va->tex;
}

static void
_check_defaults_ (GstGLContext * context, GstVideoAnalysis * va)
{
  int size, type;

  glGetInternalformativ (GL_TEXTURE_2D, GL_RED, GL_INTERNALFORMAT_RED_SIZE, 1,
      &size);
  glGetInternalformativ (GL_TEXTURE_2D, GL_RED, GL_INTERNALFORMAT_RED_TYPE, 1,
      &type);

  if (size == 8 && type == GL_UNSIGNED_NORMALIZED) {
    va->gl_settings_unchecked = FALSE;
  } else {
    GST_ERROR ("Platform default red representation is not GL_R8");
    exit (-1);
  }
}

static gboolean
videoanalysis_apply (GstVideoAnalysis * va, GstGLMemory * tex)
{
  GstGLContext *context = GST_GL_BASE_FILTER (va)->context;

  if (G_UNLIKELY (va->shader == NULL)) {
    GST_WARNING ("GL shader is not ready for analysis");
    return TRUE;
  }

  /* Ensure that GL platform defaults meet the expectations */
  if (G_UNLIKELY (va->gl_settings_unchecked)) {
    gst_gl_context_thread_add (context,
        (GstGLContextThreadFunc) _check_defaults_, va);
  }
  /* Check texture format */
  if (G_UNLIKELY (gst_gl_memory_get_texture_format (tex) != GST_GL_RED)) {
    GST_ERROR ("GL texture format should be GL_RED");
    exit (-1);
  }

  va->tex = tex;

  /* Compile shader */
  //if (G_UNLIKELY (!va->shader) )
  //        gst_gl_context_thread_add(context, (GstGLContextThreadFunc) shader_create, va);

  /* Analyze */
  gst_gl_context_thread_add (context, (GstGLContextThreadFunc) analyse, va);

  return TRUE;
}

/* static void */
/* gst_videoanalysis_timeout_loop (GstVideoAnalysis * videoanalysis) */
/* { */
/*   gint countdown = videoanalysis->timeout; */
/*   static gboolean stream_is_lost = FALSE;       /\* TODO maybe this should be true *\/ */

/*   while (countdown--) { */
/*     /\* In case we need kill the task *\/ */
/*     if (G_UNLIKELY (!atomic_load (&videoanalysis->task_should_run))) */
/*       return; */

/*     if (G_LIKELY (atomic_load (&videoanalysis->got_frame))) { */
/*       atomic_store (&videoanalysis->got_frame, FALSE); */
/*       countdown = videoanalysis->timeout; */

/*       if (stream_is_lost) { */
/*         stream_is_lost = FALSE; */
/*         g_signal_emit (videoanalysis, signals[STREAM_FOUND_SIGNAL], 0); */
/*       } */
/*     } */

/*     sleep (1); */
/*   } */

/*   /\* No frame appeared before countdown exp *\/ */
/*   if (!stream_is_lost) { */
/*     stream_is_lost = TRUE; */
/*     g_signal_emit (videoanalysis, signals[STREAM_LOST_SIGNAL], 0); */
/*   } */
/* } */
