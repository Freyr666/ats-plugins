/* GStreamer
 * Copyright (C) <2018> Freyr <sky_rider_93@mail.ru>
 *
 * gstsoundbar.h: simple soundbar
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

#ifndef __GST_SOUNDBAR_H__
#define __GST_SOUNDBAR_H__

#include "gst/pbutils/gstaudiovisualizer.h"

G_BEGIN_DECLS
#define GST_TYPE_SOUNDBAR            (gst_soundbar_get_type())
#define GST_SOUNDBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SOUNDBAR,GstSoundbar))
#define GST_SOUNDBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SOUNDBAR,GstSoundbarClass))
#define GST_IS_SOUNDBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SOUNDBAR))
#define GST_IS_SOUNDBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SOUNDBAR))

typedef struct _GstSoundbarClass GstSoundbarClass;
typedef struct _GstSoundbar GstSoundbar;

struct state {

};

struct audio_info {
        int samples;
        int channels;
        int rate;
};

struct video_info {
        int width;
        int height;
        int fps;
};

struct _GstSoundbar {
        GstAudioVisualizer parent;
  
        /* < private > */
        struct state state;

        /* filter specific data */
};

struct _GstSoundbarClass {
        GstAudioVisualizerClass parent_class;
};

GType gst_soundbar_get_type (void);
gboolean gst_soundbar_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_SOUNDBAR_H__ */
