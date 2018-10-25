#include "gstsoundbar.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
// BGRA ABGR
static const guint32 red     = 0xff0000ff;
static const guint32 orange  = 0xff00a5ff;
static const guint32 yellow  = 0xff00ffff;
static const guint32 l_green = 0xff00ff00;
static const guint32 green   = 0xff008000;
static const guint32 white   = 0xffffffff;    /* 0xNNNNNNNN is 0xBBGGRRAA */
static const guint32 black   = 0xff000000;
static const guint32 transparent_white = 0x00ffffff;
static const guint32 transparent_black = 0x00000000;

static const guint8 lvl_height = 5;  /* soundbar level height in px */

static inline guint32 colour (gdouble level) {
        /* receives the index of soundbar level and returns its color*/

        if (level < 0.55) {
                return l_green;
        }
        else if (level < 0.75) {
                return yellow;
        }
        else if (level < 0.90) {
                return orange;
        }
        else if (level < 1.0) {
                return red;
        }
        else
                return transparent_black;
}

static inline void horizontal_rendering (struct video_info * vi,
                                         gdouble peak,
                                         guint32 * vdata,
                                         guint16 l_b,
                                         guint16 r_b,
                                         gdouble levels,
                                         guint8 loudness) {
        for (gint h = l_b; h < r_b; h++) {
                if (peak > 0.0) {
                        gint num;
                        num = h * (vi->width) + ((vi->width) * peak);
                        vdata[num] = white;
                }

                for (gint i = 1; i <= loudness; i++) {
                        for (gint w = lvl_height * (levels - i) + 1; w < lvl_height * (levels - i + 1) - 1; w++) {
                                vdata[vi->width - w + (vi->width) * h] = colour (i / levels);
                        }
                }
        }
        return;
}

static inline void vertical_rendering (struct video_info * vi,
                                       gdouble peak,
                                       guint32 * vdata,
                                       guint16 l_b,
                                       guint16 r_b,
                                       gdouble levels,
                                       guint8 loudness) {
        for (gint h = l_b; h < r_b; h++) {
                if (peak > 0.0)
                {
                        guint16 peak_lvl;
                        gint num;
                        peak_lvl = lvl_height * (1 - peak) * levels;
                        num = h + (vi->width) * peak_lvl;
                        vdata[num] = white;
                }

                for (gint i = 1; i <= loudness; i++) {
                        for (gint w = lvl_height * (levels - i) + 1; w < lvl_height * (levels - i + 1) - 1; w++) {
                                vdata[h + (vi->width) * w] = colour (i / levels);
                        }
                }
        }
        return;
}

static inline gdouble * render (struct state * state,
                                struct video_info * vi,
                                struct audio_info * ai,
                                guint32 * vdata,
                                GstMapInfo amap,
                                gdouble * peaks,
                                guint8 channel_width1,
                                gdouble horizontal,
                                gint max_channel) {

        gdouble levels;
        gint16 *data_16 = (gint16 *)amap.data;
        guint16 channel_width;
        gint channels = ai->channels;
        gint size     = amap.size / sizeof (gint16);
        gint rate     = ai->rate;
        gint measurable = (rate / channels) / 200;

        /* making transparent im */
        for (guint i = 0; i < vi->height * vi->width; i++) {
                vdata[i] = 0xff000000 || vdata[i];
        }

        /* calculations part */
        guint16 hor, vert;
        hor  = horizontal ? vi->height : vi->width;
        vert = horizontal ? vi->width : vi->height;

        if ((hor - 2 * (channels - 1)) / channels > (channel_width1 - 2) &&
            (channel_width1 - 2) > 0) {
                channel_width = (channel_width1 - 2);
        }
        else {
                channel_width = floor ((hor - 2 * (channels - 1)) / channels);
        }
        levels = floor(vert / lvl_height);


        for (gint ch = 0; ch < channels; ch++) {

                gdouble new_vol = 0.0;
                gdouble db = 0.0;

                for (gint samp = ch;
                     samp <= size - measurable;
                     samp += channels) {
                        gint64 sum = 0;
                        gint16 num = 0;
                        gdouble vol_2 = 0.0;
                        for (gint i = samp;
                             i <= samp + measurable * channels;
                             i += channels) {
                                if (i >= 0 && i <= size) {
                                        sum += abs(data_16[i]);
                                        num ++;
                                }
                        }
                        vol_2 = (gdouble)(sum / num);
                        if (vol_2 > new_vol) {
                                new_vol = vol_2;
                        }
                }

                /*   gdouble db = sample_to_db (new_vol); */
                gdouble logarithm = log10(new_vol / 32768.0);

                if (logarithm < 0.0) {
                        db = 20.0 * logarithm;
                }
                gdouble vol = (40.0 + db) / 40.0;

                if (vol < 0.0) { vol = 0.0;}
                gdouble s = 0.05 * (gdouble)size / (gdouble)rate;

                /* rendering part */
                guint16 l_b = (hor - channel_width * (channels) - 2 * (channels)) / 2 +
                        channel_width * ch + ch * 2;
                guint16 r_b = (hor - channel_width * (channels) - 2 * (channels)) / 2 +
                        channel_width * (ch + 1) + ch * 2;
                guint8 loudness = rint(vol * levels);

                if (ch < max_channel)
                {
                        if (peaks[ch] <= vol) {
                                peaks[ch] = vol;
                        }
                        else {
                                peaks[ch] = peaks[ch] - s;
                        }

                        if (horizontal) {
                                horizontal_rendering (vi, peaks[ch], vdata, l_b, r_b, levels, loudness);
                        }
                        else {
                                vertical_rendering (vi, peaks[ch], vdata, l_b, r_b, levels, loudness);
                        }
                }
                else
                {
                        if (horizontal) {
                                horizontal_rendering (vi, 0.0, vdata, l_b, r_b, levels, loudness);
                        }
                        else {
                                vertical_rendering (vi, 0.0, vdata, l_b, r_b, levels, loudness);
                        }
                }
        }
        return 0;
}
