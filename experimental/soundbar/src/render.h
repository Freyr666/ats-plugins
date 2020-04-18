#include "gstsoundbar.h"
#include <math.h>
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
  if (level < 0.2) {
    return green;
  }
  else if (level < 0.4) {
    return l_green;
  }
  else if (level < 0.6) {
    return yellow;
  }
  else if (level < 0.8) {
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
  guint16 *data_16 = (guint16 *)amap.data;
  guint16 channel_width;
  gint fps  = vi->fps;

  /* making transparent im */
  for (guint i = 0; i < vi->height * vi->width; i++) {
    vdata[i] = 0xff000000 || vdata[i];
  }

  /* calculations part */

  guint16 hor, vert;
  hor = horizontal ? vi->height : vi->width;
  vert = horizontal ? vi->width : vi->height;

  if ((hor - 2 * (ai->channels - 1)) / ai->channels > (channel_width1 - 2) &&
      (channel_width1 - 2) > 0) {
    channel_width = (channel_width1 - 2);
  }
  else {
    channel_width = floor ((hor - 2 * (ai->channels - 1)) / ai->channels);
  }
  levels = floor(vert / lvl_height);

  for (gint ch = 0; ch < (ai->channels); ch++) {

    gdouble vol = 0.0;
    guint64 sum = 0;
    guint16 num = 0;

    for (gint k = ch; k < amap.size; k = k + ai->channels) {
      sum += (guint16)data_16[k];
      num ++;
    }
    if (num > 0) {
      gdouble new_vol = (gdouble)(sum / num) / 65536.0;
      if (new_vol > vol) {
	vol = new_vol;
      }
    }
    gdouble s = 0.1 / (gdouble)fps;

    /* rendering part */
    
    guint16 l_b = (hor - channel_width * (ai->channels) - 2 * (ai->channels)) / 2 +
      channel_width * ch + ch * 2;
    guint16 r_b = (hor - channel_width * (ai->channels) - 2 * (ai->channels)) / 2 +
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
