/* gstaudioanalysis.c
 *
 * Copyright (C) 2016 freyr <sky_rider_93@mail.ru> 
 *
 * This file is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version. 
 *
 * This file is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef LGPL_LIC
#define LIC "Proprietary"
#define URL "http://www.niitv.ru/"
#else
#define LIC "LGPL"
#define URL "https://github.com/Freyr666/ats-analyzer/"
#endif

/**
 * SECTION:element-gstaudioanalysis
 *
 * The audioanalysis element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! audioanalysis ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>
#include <unistd.h>
#include "ebur128.h"
#include "gstaudioanalysis.h"

#define EVALS_IN_SECOND 11 /* There are 10 100ms periods in a second + 10% error */

#define DIFF(x,y)((x > y)?(x-y):(y-x))

GST_DEBUG_CATEGORY_STATIC (gst_audioanalysis_debug_category);
#define GST_CAT_DEFAULT gst_audioanalysis_debug_category

#define gst_audioanalysis_parent_class parent_class

/* prototypes */

static void gst_audioanalysis_set_property (GObject * object,
                                            guint property_id,
                                            const GValue * value,
                                            GParamSpec * pspec);

static void gst_audioanalysis_get_property (GObject * object,
                                            guint property_id,
                                            GValue * value,
                                            GParamSpec * pspec);

static void gst_audioanalysis_finalize (GObject * object);

static GstStateChangeReturn gst_audioanalysis_change_state (GstElement * element,
                                                            GstStateChange transition);

static gboolean gst_audioanalysis_setup (GstAudioFilter * filter,
                                         const GstAudioInfo * info);

static GstFlowReturn gst_audioanalysis_transform_ip (GstBaseTransform * trans,
                                                     GstBuffer * buf);

static void gst_audioanalysis_timeout_loop (GstAudioAnalysis * audioanalysis);

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
    PROP_PERIOD,
    /* PROP_LOSS,
     * PROP_ADV_DIFF,
     * PROP_ADV_BUF,
     */
    PROP_SILENCE_CONT,
    PROP_SILENCE_CONT_EN,
    PROP_SILENCE_PEAK,
    PROP_SILENCE_PEAK_EN,
    PROP_SILENCE_DURATION,
    PROP_LOUDNESS_CONT,
    PROP_LOUDNESS_CONT_EN,
    PROP_LOUDNESS_PEAK,
    PROP_LOUDNESS_PEAK_EN,
    PROP_LOUDNESS_DURATION,
    LAST_PROP
  };

static guint      signals[LAST_SIGNAL]   = { 0 };
static GParamSpec *properties[LAST_PROP] = { NULL, };

/* pad templates */

static GstStaticPadTemplate gst_audioanalysis_src_template =
  GST_STATIC_PAD_TEMPLATE ("src",
                           GST_PAD_SRC,
                           GST_PAD_ALWAYS,
                           GST_STATIC_CAPS ("audio/x-raw,format=S16LE,rate=[1,max],"
                                            "channels=[1,max],layout=interleaved"));

static GstStaticPadTemplate gst_audioanalysis_sink_template =
  GST_STATIC_PAD_TEMPLATE ("sink",
                           GST_PAD_SINK,
                           GST_PAD_ALWAYS,
                           GST_STATIC_CAPS ("audio/x-raw,format=S16LE,rate=[1,max],"
                                            "channels=[1,max],layout=interleaved"));


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstAudioAnalysis,
			 gst_audioanalysis,
			 GST_TYPE_AUDIO_FILTER,
			 GST_DEBUG_CATEGORY_INIT (gst_audioanalysis_debug_category,
						  "audioanalysis", 0,
						  "debug category for audioanalysis element"));

static void
gst_audioanalysis_class_init (GstAudioAnalysisClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass); 
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstAudioFilterClass *audio_filter_class = GST_AUDIO_FILTER_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                      gst_static_pad_template_get (&gst_audioanalysis_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                      gst_static_pad_template_get (&gst_audioanalysis_sink_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
                                         "Gstreamer element for audio analysis",
                                         "Audio data analysis",
                                         "filter for audio analysis",
                                         "freyr <sky_rider_93@mail.ru>");

  gobject_class->set_property = gst_audioanalysis_set_property;
  gobject_class->get_property = gst_audioanalysis_get_property;
  gobject_class->finalize = gst_audioanalysis_finalize;
  element_class->change_state = gst_audioanalysis_change_state;
  audio_filter_class->setup = GST_DEBUG_FUNCPTR (gst_audioanalysis_setup);
  base_transform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_audioanalysis_transform_ip);

  signals[DATA_SIGNAL] =
    g_signal_new("data", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(GstAudioAnalysisClass, data_signal), NULL, NULL,
                 g_cclosure_marshal_generic, G_TYPE_NONE,
                 1, GST_TYPE_BUFFER);
  signals[STREAM_LOST_SIGNAL] =
    g_signal_new("stream-lost", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(GstAudioAnalysisClass, stream_lost_signal), NULL, NULL,
                 g_cclosure_marshal_generic, G_TYPE_NONE, 0);
  signals[STREAM_FOUND_SIGNAL] =
    g_signal_new("stream-found", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(GstAudioAnalysisClass, stream_found_signal), NULL, NULL,
                 g_cclosure_marshal_generic, G_TYPE_NONE, 0);

  properties [PROP_TIMEOUT] =
    g_param_spec_uint("timeout", "Stream-lost event timeout",
                      "Seconds needed for stream-lost being emitted.",
                      1, G_MAXUINT, 10, G_PARAM_READWRITE);
  properties [PROP_PERIOD] =
    g_param_spec_uint("period", "Period",
                      "Measuring period",
                      1, 60, 1, G_PARAM_READWRITE);
  /*  properties [PROP_PROGRAM] =
    g_param_spec_int("program", "Program",
                     "Program",
                     0, G_MAXINT, 2010, G_PARAM_READWRITE);
  */    
  /* properties [PROP_LOSS] =
    g_param_spec_float("loss", "Loss",
                       "Audio loss",
                       0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
  properties [PROP_ADV_DIFF] =
    g_param_spec_float("adv_diff", "Adv sound level diff",
                       "Adv sound level diff",
                       0., 1., 0.5, G_PARAM_READWRITE);
  properties [PROP_ADV_BUF] =
    g_param_spec_int("adv_buff", "Size of adv buffer",
                     "Size of adv buffer",
                     0, 1000, 100, G_PARAM_READWRITE);
  */
  properties [PROP_SILENCE_CONT] =
    g_param_spec_float("silence_cont", "Silence cont err boundary",
                       "Silence cont err meas",
                       -70.0, -5.0, -35.0, G_PARAM_READWRITE);
  properties [PROP_SILENCE_CONT_EN] =
    g_param_spec_boolean("silence_cont_en", "Silence cont err enabled",
                         "Enable silence cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_SILENCE_PEAK] =
    g_param_spec_float("silence_peak", "Silence peak err boundary",
                       "Silence peak err meas",
                       -70.0, -5.0, -45.0, G_PARAM_READWRITE);
  properties [PROP_SILENCE_PEAK_EN] =
    g_param_spec_boolean("silence_peak_en", "Silence peak err enabled",
                         "Enable silence peak err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_SILENCE_DURATION] =
    g_param_spec_float("silence_duration", "Silence duration boundary",
                       "Silence err duration",
                       0., G_MAXFLOAT, 10., G_PARAM_READWRITE);
  properties [PROP_LOUDNESS_CONT] =
    g_param_spec_float("loudness_cont", "Loudness cont err boundary",
                       "Loudness cont err meas",
                       -70.0, -5.0, -21.9, G_PARAM_READWRITE);
  properties [PROP_LOUDNESS_CONT_EN] =
    g_param_spec_boolean("loudness_cont_en", "Loudness cont err enabled",
                         "Enable loudness cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_LOUDNESS_PEAK] =
    g_param_spec_float("loudness_peak", "Loudness peak err boundary",
                       "Loudness peak err meas",
                       -70.0, -5.0, -15.0, G_PARAM_READWRITE);
  properties [PROP_LOUDNESS_PEAK_EN] =
    g_param_spec_boolean("loudness_peak_en", "Loudness peak err enabled",
                         "Enable loudness peak err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_LOUDNESS_DURATION] =
    g_param_spec_float("loudness_duration", "Loudness duration boundary",
                       "Loudness err duration",
                       0., G_MAXFLOAT, 2.0, G_PARAM_READWRITE);
  
  g_object_class_install_properties(gobject_class, LAST_PROP, properties);
}

static void
gst_audioanalysis_init (GstAudioAnalysis *audioanalysis)
{  
  audioanalysis->period = 1;
  audioanalysis->timeout = 10;
  
  /* TODO cleanup this mess */
  for (int i = 0; i < PARAM_NUMBER; i++) {
    audioanalysis->params_boundary[i].cont_en = TRUE;
    audioanalysis->params_boundary[i].peak_en = TRUE;
  }
        
  audioanalysis->params_boundary[SILENCE_SHORTT].cont = -35.0;
  audioanalysis->params_boundary[SILENCE_SHORTT].peak = -45.0;
  audioanalysis->params_boundary[SILENCE_SHORTT].duration = 10.;
        
  audioanalysis->params_boundary[SILENCE_MOMENT].cont = -45.0;
  audioanalysis->params_boundary[SILENCE_MOMENT].peak = -35.0;
  audioanalysis->params_boundary[SILENCE_MOMENT].duration = 10.;
        
  audioanalysis->params_boundary[LOUDNESS_SHORTT].cont = -21.9;
  audioanalysis->params_boundary[LOUDNESS_SHORTT].peak = -15.0;
  audioanalysis->params_boundary[LOUDNESS_SHORTT].duration = 2.;
        
  audioanalysis->params_boundary[LOUDNESS_MOMENT].cont = -21.9;
  audioanalysis->params_boundary[LOUDNESS_MOMENT].peak = -15.0;
  audioanalysis->params_boundary[LOUDNESS_MOMENT].duration = 2.;
        
  /* private */
  data_ctx_init (&audioanalysis->errors);
  
  for (int i = 0; i < PARAM_NUMBER; i++) {
    audioanalysis->error_state.cont_err_duration[i] = 0.;
  }

  /* init in (setup) */
  audioanalysis->lufs_state = NULL;
  
  audioanalysis->timeout_task = gst_task_new ((GstTaskFunction) gst_audioanalysis_timeout_loop,
                                              audioanalysis,
                                              NULL);
  g_rec_mutex_init (&audioanalysis->task_lock);
  gst_task_set_lock (audioanalysis->timeout_task, &audioanalysis->task_lock);
}

void
gst_audioanalysis_finalize (GObject * object)
{
  GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);

  GST_DEBUG_OBJECT (audioanalysis, "finalize");

  if (audioanalysis->lufs_state != NULL)
    ebur128_destroy(&audioanalysis->lufs_state);

  data_ctx_delete (&audioanalysis->errors);

  gst_object_unref (audioanalysis->timeout_task);
  //g_rec_mutex_clear (&audioanalysis->task_lock);

  G_OBJECT_CLASS (gst_audioanalysis_parent_class)->finalize (object);
}

void
gst_audioanalysis_set_property (GObject * object,
				guint property_id,
				const GValue * value,
				GParamSpec * pspec)
{
  GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);

  GST_DEBUG_OBJECT (audioanalysis, "set_property");

  switch (property_id) {
  case PROP_TIMEOUT:
    audioanalysis->timeout = g_value_get_uint(value);
    break;
  case PROP_PERIOD:
    audioanalysis->period = g_value_get_uint(value);
    break;
  case PROP_SILENCE_CONT:
    audioanalysis->params_boundary[SILENCE_SHORTT].cont = g_value_get_float(value);
    audioanalysis->params_boundary[SILENCE_MOMENT].cont = g_value_get_float(value);
    break;
  case PROP_SILENCE_CONT_EN:
    audioanalysis->params_boundary[SILENCE_SHORTT].cont_en = g_value_get_boolean(value);
    audioanalysis->params_boundary[SILENCE_MOMENT].cont_en = g_value_get_boolean(value);
    break;
  case PROP_SILENCE_PEAK:
    audioanalysis->params_boundary[SILENCE_SHORTT].peak = g_value_get_float(value);
    audioanalysis->params_boundary[SILENCE_MOMENT].peak = g_value_get_float(value);
    break;
  case PROP_SILENCE_PEAK_EN:
    audioanalysis->params_boundary[SILENCE_SHORTT].peak_en = g_value_get_boolean(value);
    audioanalysis->params_boundary[SILENCE_MOMENT].peak_en = g_value_get_boolean(value);
    break;
  case PROP_SILENCE_DURATION:
    audioanalysis->params_boundary[SILENCE_SHORTT].duration = g_value_get_float(value);
    audioanalysis->params_boundary[SILENCE_MOMENT].duration = g_value_get_float(value);
    break;
  case PROP_LOUDNESS_CONT:
    audioanalysis->params_boundary[LOUDNESS_SHORTT].cont = g_value_get_float(value);
    audioanalysis->params_boundary[LOUDNESS_MOMENT].cont = g_value_get_float(value);
    break;
  case PROP_LOUDNESS_CONT_EN:
    audioanalysis->params_boundary[LOUDNESS_SHORTT].cont_en = g_value_get_boolean(value);
    audioanalysis->params_boundary[LOUDNESS_MOMENT].cont_en = g_value_get_boolean(value);
    break;
  case PROP_LOUDNESS_PEAK:
    audioanalysis->params_boundary[LOUDNESS_SHORTT].peak = g_value_get_float(value);
    audioanalysis->params_boundary[LOUDNESS_MOMENT].peak = g_value_get_float(value);
    break;
  case PROP_LOUDNESS_PEAK_EN:
    audioanalysis->params_boundary[LOUDNESS_SHORTT].peak_en = g_value_get_boolean(value);
    audioanalysis->params_boundary[LOUDNESS_MOMENT].peak_en = g_value_get_boolean(value);
    break;
  case PROP_LOUDNESS_DURATION:
    audioanalysis->params_boundary[LOUDNESS_SHORTT].duration = g_value_get_float(value);
    audioanalysis->params_boundary[LOUDNESS_MOMENT].duration = g_value_get_float(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

void
gst_audioanalysis_get_property (GObject * object,
				guint property_id,
				GValue * value,
				GParamSpec * pspec)
{
  GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);

  GST_DEBUG_OBJECT (audioanalysis, "get_property");

  switch (property_id) {
  case PROP_TIMEOUT: 
    g_value_set_uint(value, audioanalysis->timeout);
    break;
  case PROP_PERIOD:
    g_value_set_uint(value, audioanalysis->period);
    break;
  case PROP_SILENCE_CONT:
    g_value_set_float(value, audioanalysis->params_boundary[SILENCE_SHORTT].cont);
    break;
  case PROP_SILENCE_CONT_EN:
    g_value_set_boolean(value, audioanalysis->params_boundary[SILENCE_SHORTT].cont_en);
    break;
  case PROP_SILENCE_PEAK:
    g_value_set_float(value, audioanalysis->params_boundary[SILENCE_SHORTT].peak);
    break;
  case PROP_SILENCE_PEAK_EN:
    g_value_set_boolean(value, audioanalysis->params_boundary[SILENCE_SHORTT].peak_en);
    break;
  case PROP_SILENCE_DURATION:
    g_value_set_float(value, audioanalysis->params_boundary[SILENCE_SHORTT].duration);
    break;
  case PROP_LOUDNESS_CONT:
    g_value_set_float(value, audioanalysis->params_boundary[LOUDNESS_SHORTT].cont);
    break;
  case PROP_LOUDNESS_CONT_EN:
    g_value_set_boolean(value, audioanalysis->params_boundary[LOUDNESS_SHORTT].cont_en);
    break;
  case PROP_LOUDNESS_PEAK:
    g_value_set_float(value, audioanalysis->params_boundary[LOUDNESS_SHORTT].peak);
    break;
  case PROP_LOUDNESS_PEAK_EN:
    g_value_set_boolean(value, audioanalysis->params_boundary[LOUDNESS_SHORTT].peak_en);
    break;
  case PROP_LOUDNESS_DURATION:
    g_value_set_float(value, audioanalysis->params_boundary[LOUDNESS_SHORTT].duration);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static GstStateChangeReturn
gst_audioanalysis_change_state (GstElement * element,
                                GstStateChange transition)
{
  GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (element);

  switch (transition) {
    /* Initialize task and clocks */
  case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    {
      audioanalysis->time_now = g_get_real_time ();
      update_all_timestamps (&audioanalysis->error_state, audioanalysis->time_now);
      for (int i = 0; i < PARAM_NUMBER; i++)
        audioanalysis->error_state.cont_err_duration[i] = 0.;
      
      audioanalysis->next_evaluation_ts = 0;
      audioanalysis->next_data_message_ts = 0;

      atomic_store(&audioanalysis->got_frame, FALSE);
      atomic_store(&audioanalysis->task_should_run, TRUE);
      
      gst_task_start (audioanalysis->timeout_task);
    }
    break;
  case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    {
      atomic_store(&audioanalysis->task_should_run, FALSE);
      
      gst_task_join (audioanalysis->timeout_task);
    }
    break;
  default:
    break;
  }

  return
    GST_ELEMENT_CLASS (gst_audioanalysis_parent_class)->change_state (element,
                                                                      transition);
}

static gboolean
gst_audioanalysis_setup (GstAudioFilter * filter,
			 const GstAudioInfo * info)
{
  GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (filter);

  GST_DEBUG_OBJECT (audioanalysis, "setup");

  if (audioanalysis->lufs_state != NULL)
    ebur128_destroy(&audioanalysis->lufs_state);
  
  audioanalysis->lufs_state = ebur128_init(info->channels,
                                           (unsigned long)info->rate,
                                           EBUR128_MODE_S | EBUR128_MODE_M);
  /*
  audioanalysis->glob_state = ebur128_init(info->channels,
                                           (unsigned long)info->rate,
                                           EBUR128_MODE_I);
  */
  guint32 per = EVALS_IN_SECOND * audioanalysis->period;
  data_ctx_reset (&audioanalysis->errors, per);

  return TRUE;
}

#define SECOND 1000000

static void
_update_flags_and_timestamps (struct data_ctx * ctx,
                              struct state * state,
                              gint64 new_ts)
{
  struct flag * current_flag;
  
  for (int p = 0; p < PARAM_NUMBER; p++)
    {
      current_flag = &ctx->errs[p]->cont_flag;

      if (current_flag->value)
        {
          current_flag->timestamp = get_timestamp (state, p);
          /* TODO check if this is actually true */
          current_flag->span = new_ts - current_flag->timestamp;
        }
      else
        update_timestamp (state, p, new_ts);
    }
}

static void
_set_flags (struct data_ctx * ctx,
            struct boundary bounds [PARAM_NUMBER],
            struct state * state,
            double moment,
            double shortt)
{
  for (int p = 0; p < PARAM_NUMBER; p++)
    {
      data_ctx_flags_cmp (ctx,
                          p,
                          &bounds[p],
                          param_boundary_is_upper (p),
                          &(state->cont_err_duration[p]),
                          0.1, /* 100ms */
                          IS_MOMENT(p) ? moment : shortt);
    }
}

/* transform */
static GstFlowReturn
gst_audioanalysis_transform_ip (GstBaseTransform * trans,
				GstBuffer * buf)
{
  GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (trans);
  
  GstMapInfo map;
  static gint64 evaluation_counter = 0;
  guint num_frames;

  GST_DEBUG_OBJECT (audioanalysis, "transform_ip");

  /* Initialize clocks if needed */
  if (G_UNLIKELY (audioanalysis->next_evaluation_ts == 0
                  || audioanalysis->next_data_message_ts == 0))
    {
      audioanalysis->next_evaluation_ts =
        GST_BUFFER_TIMESTAMP (buf) + (GST_MSECOND * 90); /* TODO: There should be 100ms
                                                          * periods but we will allow
                                                          * a 10% overlap for the sake
                                                          * of dealing with big samples
                                                          * which rarely fit in 100ms window
                                                          *
                                                          * TODO: use sample rate to infer
                                                          * the error
                                                          */
      audioanalysis->next_data_message_ts =
        GST_BUFFER_TIMESTAMP (buf) + (GST_SECOND * audioanalysis->period);
    }

  gst_buffer_map(buf, &map, GST_MAP_READ);
  num_frames =
    map.size / (GST_AUDIO_FILTER_BPS(audioanalysis)
                * GST_AUDIO_FILTER_CHANNELS(audioanalysis));

  ebur128_add_frames_short(audioanalysis->lufs_state, (short*)map.data, num_frames);

  gst_buffer_unmap(buf, &map);

  /* send data for the momentary and short term states
   * after a second of observations
   */
  if (GST_BUFFER_TIMESTAMP (buf)
      >= audioanalysis->next_data_message_ts)
    {
      size_t data_size;
      
      evaluation_counter = 0;

      audioanalysis->time_now = g_get_real_time ();

      /* Update clock */
      audioanalysis->next_data_message_ts =
        GST_BUFFER_TIMESTAMP (buf) + GST_SECOND;
      
      _update_flags_and_timestamps (&audioanalysis->errors,
                                    &audioanalysis->error_state,
                                    audioanalysis->time_now);
                
      gpointer d = data_ctx_pull_out_data (&audioanalysis->errors, &data_size);

      data_ctx_reset (&audioanalysis->errors, EVALS_IN_SECOND * audioanalysis->period);

      GstBuffer* data = gst_buffer_new_wrapped (d, data_size);
      g_signal_emit(audioanalysis, signals[DATA_SIGNAL], 0, data);

      gst_buffer_unref (data);
  }

  /* eval loudness for the 100ms interval */
  if (GST_BUFFER_TIMESTAMP (buf)
      >= audioanalysis->next_evaluation_ts)
    {
      double moment, shortt;

      evaluation_counter++;

      audioanalysis->time_now +=
        90 * GST_MSECOND
        + (audioanalysis->next_evaluation_ts - GST_BUFFER_TIMESTAMP (buf));

      /* Update clock */
      audioanalysis->next_evaluation_ts =
        GST_BUFFER_TIMESTAMP (buf) + (GST_MSECOND * 90);

      /* Tell the timeout task that we've got some data 
       * We are doing it once in 100ms instead of each
       * sample for the sake of performance
       */
      atomic_store(&audioanalysis->got_frame, TRUE);

      /* Check if the amount of evaluation points
       * hasn't surpassed the preallocated limit
       */
      if (G_UNLIKELY (evaluation_counter > audioanalysis->errors.limit))
        goto exit;
    
      ebur128_loudness_momentary(audioanalysis->lufs_state, &moment);
      ebur128_loudness_shortterm(audioanalysis->lufs_state, &shortt);

    /* errors */
      _set_flags (&audioanalysis->errors,
                  audioanalysis->params_boundary,
                  &audioanalysis->error_state,
                  moment,
                  shortt);
    
      data_ctx_add_point (&audioanalysis->errors,
                          SHORTT,
                          shortt,
                          audioanalysis->time_now);

      data_ctx_add_point (&audioanalysis->errors,
                          MOMENT,
                          moment,
                          audioanalysis->time_now);
                
    }

 exit:
  return GST_FLOW_OK;
}

static void
gst_audioanalysis_timeout_loop (GstAudioAnalysis * audioanalysis)
{
  gint countdown = audioanalysis->timeout;
  static gboolean stream_is_lost = FALSE; /* TODO maybe this should be true */
  
  while (countdown--)
    {
      /* In case we need kill the task */
      if (G_UNLIKELY (! atomic_load (&audioanalysis->task_should_run)))
        return;

      if (G_LIKELY (atomic_load (&audioanalysis->got_frame)))
        {
          atomic_store (&audioanalysis->got_frame, FALSE);
          countdown = audioanalysis->timeout;
          
          if (stream_is_lost)
            {
              stream_is_lost = FALSE;
              g_signal_emit(audioanalysis, signals[STREAM_FOUND_SIGNAL], 0);
            }
        }

      sleep (1);
    }

  /* No frame appeared before countdown exp */
  if ( !stream_is_lost )
    {
      stream_is_lost = TRUE;
      g_signal_emit(audioanalysis, signals[STREAM_LOST_SIGNAL], 0);
    }
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin,
                               "audioanalysis",
                               GST_RANK_NONE,
                               GST_TYPE_AUDIOANALYSIS);
}

/*
static gboolean
gst_filter_sink_ad_event (GstBaseTransform * base,
			  GstEvent * event)
{
  GstAudioAnalysis       *filter;
  const GstStructure     *st; 
  guint                  pid;
  guint                  ad;
  
  filter = GST_AUDIOANALYSIS(base);
  
  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_DOWNSTREAM) { 

    st = gst_event_get_structure(event);

    if (gst_structure_has_name(st, "ad")) {
      
      pid = g_value_get_uint(gst_structure_get_value(st, "pid"));
      ad  = g_value_get_uint(gst_structure_get_value(st, "isad"));
      
      if ((guint)filter->program == pid) {

        gst_audioanalysis_eval_global(base, ad);
	
        gst_event_unref(event);
        event = NULL;
      }
    }
  }
  / pass event on /
  if (event)
    return GST_BASE_TRANSFORM_CLASS
      (gst_audioanalysis_parent_class)->sink_event (base, event);
  else 
    return TRUE;  
}

static inline void
gst_audioanalysis_eval_global (GstBaseTransform * trans,
			       guint ad_flag)
{

    GstAudioAnalysis *audioanalysis;
    double           result, diff_time;
    time_t           now;
    gchar            string[50];

    audioanalysis = GST_AUDIOANALYSIS (trans);
    now = time(NULL);

    // if measurements have already begun
    if (audioanalysis->glob_ad_flag) {

    ebur128_loudness_global(audioanalysis->glob_state, &result);
    ebur128_clear_block_list(audioanalysis->glob_state);

    diff_time = difftime(audioanalysis->glob_start, now);

    g_snprintf(string, 39, "c%d:%d:%d:*:%.2f:%.2f:%d",
    audioanalysis->stream_id,
    audioanalysis->program,
    audioanalysis->pid,
    result, diff_time, ad_flag);

    gst_audioanalysis_send_string(string, audioanalysis);
    } else {
    audioanalysis->glob_ad_flag = TRUE;
    }

    audioanalysis->glob_start = now;

}
*/


  /* add frames to an ad state */
  /* if (audioanalysis->glob_ad_flag) {

     now = time(NULL);

     ebur128_add_frames_short(audioanalysis->glob_state, (short*)map.data, num_frames);

     // interval exceeded specified timeout
     if (DIFF(now, audioanalysis->glob_start) >= audioanalysis->ad_timeout) {
     ebur128_clear_block_list(audioanalysis->glob_state);
     audioanalysis->glob_ad_flag = FALSE;
     }
     }*/

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.1.9"
#endif
#ifndef PACKAGE
#define PACKAGE "audioanalysis"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "audioanalysis_package"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN URL
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   audioanalysis,
		   "Package for audio data analysis",
		   plugin_init, VERSION, LIC, PACKAGE_NAME, GST_PACKAGE_ORIGIN)

