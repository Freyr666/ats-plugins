/* GStreamer
 * Copyright (C) <2018> Freyr <sky_rider_93@mail.ru>
 *
 * gstsoundbar.c: simple soundbar
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#define MAX_CHANNEL_N 24

#ifndef LGPL_LIC
#define LIC "Proprietary"
#define URL "http://www.niitv.ru/"
#else
#define LIC "LGPL"
#define URL "https://github.com/Freyr666/ats-analyzer/"
#endif

/**
 * SECTION:element-soundbar
 * @title: soundbar
 *
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc ! audioconvert ! soundbar ! ximagesink
 * ]|
 *
 */

#include <stdlib.h>
#include "gstsoundbar.h"
#include "render.h"

static GstStaticPadTemplate gst_soundbar_src_template =
        GST_STATIC_PAD_TEMPLATE ("src",
                                 GST_PAD_SRC,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGBA"))
                );

static GstStaticPadTemplate gst_soundbar_sink_template =
        GST_STATIC_PAD_TEMPLATE ("sink",
                                 GST_PAD_SINK,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS ("audio/x-raw, "
                                                  "format = (string) " GST_AUDIO_NE (S16) ", "
                                                  "layout = (string) interleaved, "
                                                  "rate = (int) [ 8000, 96000 ]")
                                         );

enum {
        PROP_0,
};

static void gst_soundbar_finalize (GObject * object);
static gboolean gst_soundbar_setup (GstAudioVisualizer * scope);
static gboolean gst_soundbar_render (GstAudioVisualizer * base,
                                     GstBuffer * audio,
                                     GstVideoFrame * video);

#define gst_soundbar_parent_class parent_class
G_DEFINE_TYPE (GstSoundbar, gst_soundbar, GST_TYPE_AUDIO_VISUALIZER);

static void
gst_soundbar_class_init (GstSoundbarClass * g_class) {
        
        GObjectClass *gobject_class = (GObjectClass *) g_class;
        GstElementClass *gstelement_class = (GstElementClass *) g_class;
        GstAudioVisualizerClass *scope_class = (GstAudioVisualizerClass *) g_class;

        gobject_class->finalize = gst_soundbar_finalize;

        scope_class->setup = GST_DEBUG_FUNCPTR (gst_soundbar_setup);
        scope_class->render = GST_DEBUG_FUNCPTR (gst_soundbar_render);

        gst_element_class_set_static_metadata (gstelement_class,
                                               "Soundbar", "Visualization", "Simple soundbar",
                                               "Freyr <sky_rider_93@mail.ru>");

        gst_element_class_add_static_pad_template (gstelement_class,
                                                   &gst_soundbar_src_template);
        gst_element_class_add_static_pad_template (gstelement_class,
                                                   &gst_soundbar_sink_template);
}

static void
gst_soundbar_init (GstSoundbar * scope) {
        g_object_set (G_OBJECT(scope), "shader", 0, NULL);

        /* TODO object init */
}

static void
gst_soundbar_finalize (GObject * object) {

        GstSoundbar *scope = GST_SOUNDBAR (object);

        /* TODO finalize */

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_soundbar_setup (GstAudioVisualizer * bscope) {

        GstSoundbar *scope = GST_SOUNDBAR (bscope);

        /* TODO reset on format change */

        return TRUE;
}

 static gdouble peaks[MAX_CHANNEL_N] = {0.0}; 

static gboolean
gst_soundbar_render (GstAudioVisualizer * base, GstBuffer * audio,
                     GstVideoFrame * video) {

        GstSoundbar *scope = GST_SOUNDBAR (base);
        GstMapInfo amap;
        guint num_samples;
        gint channels = GST_AUDIO_INFO_CHANNELS (&base->ainfo);
        gint rate     = GST_AUDIO_INFO_RATE (&base->ainfo);
        gint fps      = GST_VIDEO_INFO_FPS_N (&base->vinfo);
        struct video_info vi;
        struct audio_info ai;

        gst_buffer_map (audio, &amap, GST_MAP_READ);

        num_samples = amap.size / (channels * sizeof (gint16));
        ai = (struct audio_info) { .samples = num_samples, .channels = channels, .rate = rate };
        vi = (struct video_info) { .width  = GST_VIDEO_INFO_WIDTH (&base->vinfo),
                                   .height = GST_VIDEO_INFO_HEIGHT (&base->vinfo),
                                   .fps    = fps
        };
        guint8 channel_width = 10;
        gboolean horizontal  = FALSE;

        render (&scope->state, &vi, &ai,
                (guint32 *) GST_VIDEO_FRAME_PLANE_DATA (video, 0),
                amap,
                peaks,
                channel_width,
                horizontal,
                MAX_CHANNEL_N);
        gst_buffer_unmap (audio, &amap);

        return TRUE;
}

GST_DEBUG_CATEGORY_STATIC (soundbar_debug);
#define GST_CAT_DEFAULT soundbar_debug

gboolean
gst_soundbar_plugin_init (GstPlugin * plugin) {

  GST_DEBUG_CATEGORY_INIT (soundbar_debug, "soundbar", 0, "soundbar");

  return gst_element_register (plugin, "soundbar", GST_RANK_NONE,
                               GST_TYPE_SOUNDBAR);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.1.9"
#endif
#ifndef PACKAGE
#define PACKAGE "soundbar"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "soundbar"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN URL
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   soundbar,
		   "Package for the imaging of a sound level",
		   gst_soundbar_plugin_init, VERSION, LIC, PACKAGE_NAME, GST_PACKAGE_ORIGIN)

