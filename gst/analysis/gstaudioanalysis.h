/* gstaudioanalysis.h
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

#ifndef _GST_AUDIOANALYSIS_H_
#define _GST_AUDIOANALYSIS_H_

#include <time.h>
#include <stdatomic.h>
#include <unistd.h>
#include <gst/audio/gstaudiofilter.h>
#include <ebur128.h>

#include "gstaudioanalysis_priv.h"

#define OBSERVATION_TIME 100000000

G_BEGIN_DECLS
#define GST_TYPE_AUDIOANALYSIS                  \
  (gst_audioanalysis_get_type())
#define GST_AUDIOANALYSIS(obj)                                          \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIOANALYSIS,GstAudioAnalysis))
#define GST_AUDIOANALYSIS_CLASS(klass)                                  \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AUDIOANALYSIS,GstAudioAnalysisClass))
#define GST_IS_AUDIOANALYSIS(obj)                               \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIOANALYSIS))
#define GST_IS_AUDIOANALYSIS_CLASS(obj)                         \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AUDIOANALYSIS))
#define GST_AUDIOANALYSIS_GET_CLASS(obj)                                \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_AUDIOANALYSIS,GstAudioAnalysisClass))
typedef struct _GstAudioAnalysis GstAudioAnalysis;
typedef struct _GstAudioAnalysisClass GstAudioAnalysisClass;

struct audio_analysis_state
{
  gfloat cont_err_duration[AUDIO_PARAM_NUMBER];
  gint64 cont_err_past_timestamp[AUDIO_PARAM_NUMBER];
};

struct _GstAudioAnalysis
{
  GstAudioFilter parent;

  /* Stream-loss detection task */
  atomic_bool got_frame;
  atomic_bool task_should_run;
  GRecMutex task_lock;

  GstTask *timeout_task;

  /* Loudness evaluation */
  gint64 time_now_us;
  struct audio_analysis_state error_state;
  struct audio_data_ctx errors;
  GstClockTime next_evaluation_ts;
  GstClockTime next_data_message_ts;

  ebur128_state *lufs_state;
  /* Global loudless
   * ebur128_state   *glob_state;
   * gboolean        glob_ad_flag;
   * time_t          glob_start;
   */

  /* Parameters */
  guint period;                 /* Seconds between data_signal events */
  guint timeout;                /* Seconds before stream_lost_signal */
  struct boundary params_boundary[AUDIO_PARAM_NUMBER];  /* Boundary values fo error detection */
  /* float    loss;
   * gfloat   adv_diff;
   * gint     adv_buf;
   */

};

struct _GstAudioAnalysisClass
{
  GstAudioFilterClass parent_class;

  void (*data_signal) (GstAudioFilter * filter, GstBuffer * d);
  void (*stream_lost_signal) (GstAudioFilter * filter);
  void (*stream_found_signal) (GstAudioFilter * filter);
};

GType gst_audioanalysis_get_type (void);

G_END_DECLS
#endif
