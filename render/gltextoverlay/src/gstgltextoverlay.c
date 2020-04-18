/*
 * GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 * Copyright (C) 2018 NIITV. Ivan Fedotov<ivanfedotovmail@yandex.ru>
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

/**
 * SECTION:element-gltextoverlay
 * @title: gltextoverlay
 *
 * The gltextoverlay element is used to produce test video texture.
 * The video test produced can be controlled with the "pattern"
 * property.
 *
 * ## Example launch line
 *
 * |[
 * gst-launch-1.0 -v gltextoverlay pattern=smpte ! glimagesink
 * ]|
 * Shows original SMPTE color bars in a window.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gst/gl/gstglfuncs.h"
#include "gst/gst-i18n-plugin.h"

#include "gstgltextoverlay.h"
//#include "gltextoverlay.h"

#define USE_PEER_BUFFERALLOC
#define SUPPORTED_GL_APIS (GST_GL_API_OPENGL | GST_GL_API_OPENGL3 | GST_GL_API_GLES2)

GST_DEBUG_CATEGORY_STATIC (gl_text_overlay_debug);
#define GST_CAT_DEFAULT gl_text_overlay_debug

enum
{
  PROP_0,
  PROP_TIMESTAMP_OFFSET,
  PROP_IS_LIVE,
  PROP_FONT_CAPTION,
  PROP_FONT_STYLE,
  PROP_BG_COLOR_ARGB,
  PROP_TEXT_COLOR_ARGB,
  PROP_TEXT_X,
  PROP_TEXT_Y,
  PROP_TEXT_SIZE_MULT,
  PROP_TEXT,
  PROP_ALIGN_X,
  PROP_ALIGN_Y
      /* FILL ME */
};

/* *INDENT-OFF* */
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
        "format = (string) RGBA, "
        "width = " GST_VIDEO_SIZE_RANGE ", "
        "height = " GST_VIDEO_SIZE_RANGE ", "
        "framerate = " GST_VIDEO_FPS_RANGE ","
        "texture-target = (string) 2D")
    );
/* *INDENT-ON* */

#define gst_gl_text_overlay_parent_class parent_class
G_DEFINE_TYPE (GstGLTextOverlay, gst_gl_text_overlay, GST_TYPE_PUSH_SRC);

static void gst_gl_text_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_gl_text_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_gl_text_overlay_setcaps (GstBaseSrc * bsrc, GstCaps * caps);
static GstCaps *gst_gl_text_overlay_fixate (GstBaseSrc * bsrc, GstCaps * caps);

static gboolean gst_gl_text_overlay_is_seekable (GstBaseSrc * psrc);
static gboolean gst_gl_text_overlay_do_seek (GstBaseSrc * bsrc,
    GstSegment * segment);
static gboolean gst_gl_text_overlay_query (GstBaseSrc * bsrc, GstQuery * query);
static void gst_gl_text_overlay_set_context (GstElement * element,
    GstContext * context);
static GstStateChangeReturn gst_gl_text_overlay_change_state (GstElement * element,
    GstStateChange transition);

static void gst_gl_text_overlay_get_times (GstBaseSrc * basesrc,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static GstFlowReturn gst_gl_text_overlay_fill (GstPushSrc * psrc,
    GstBuffer * buffer);
static gboolean gst_gl_text_overlay_start (GstBaseSrc * basesrc);
static gboolean gst_gl_text_overlay_stop (GstBaseSrc * basesrc);
static gboolean gst_gl_text_overlay_decide_allocation (GstBaseSrc * basesrc,
    GstQuery * query);

static gboolean gst_gl_text_overlay_init_shader (GstGLTextOverlay * gltextoverlay);

static void gst_gl_text_overlay_dispose (GObject * object);

#define gst_gl_text_overlay_parent_class parent_class

static GstCaps *
gst_gl_text_overlay_fixate (GstBaseSrc * bsrc, GstCaps * caps)
{
  GstStructure *structure;

  GST_DEBUG ("fixate");

  caps = gst_caps_make_writable (caps);

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "width", 320);
  gst_structure_fixate_field_nearest_int (structure, "height", 240);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate", 30, 1);

  caps = GST_BASE_SRC_CLASS (parent_class)->fixate (bsrc, caps);

  return caps;
}

static void
gst_gl_text_overlay_close_gldraw (GstGLContext * context, GstGLTextOverlay * src){
  if(src->gl_drawing.gl_drawing_created==TRUE){
    gldraw_close (src->context, &src->gl_drawing);
    src->gl_drawing.gl_drawing_created=FALSE;
  }
}

static void
gst_gl_text_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (object);

  switch (prop_id) {
    case PROP_TIMESTAMP_OFFSET:
      src->timestamp_offset = g_value_get_int64 (value);
      break;
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (src), g_value_get_boolean (value));
      break;
    case PROP_FONT_CAPTION:
      if(strcmp(g_value_get_string(value),"")!=0){
        gldraw_set_font_caption(&src->gl_drawing,g_value_get_string(value));
        if(src->context){
          gst_gl_context_thread_add (src->context,
            (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
        }
      }
      break;
    case PROP_TEXT:
      gldraw_set_text(&src->gl_drawing,g_value_get_string(value));
      if(src->context){
        gst_gl_context_thread_add (src->context,
          (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
      }
      break;
    case PROP_FONT_STYLE:
      if(strcmp(g_value_get_string(value),"")!=0){
        gldraw_set_font_style(&src->gl_drawing,g_value_get_string(value));
        if(src->context){
            gst_gl_context_thread_add (src->context,
              (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
        }
      }
      break;
    case PROP_BG_COLOR_ARGB:
        setBGColor(&src->gl_drawing, g_value_get_uint(value));
        if(src->context){
            gst_gl_context_thread_add (src->context,
              (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
        }
      break;
    case PROP_TEXT_COLOR_ARGB:
        setTextColor(&src->gl_drawing, g_value_get_uint(value));
        if(src->context){
            gst_gl_context_thread_add (src->context,
              (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
        }
      break;
    case PROP_TEXT_X:
        setTextX(&src->gl_drawing, g_value_get_float(value));
      break;
    case PROP_TEXT_Y:
        setTextY(&src->gl_drawing, g_value_get_float(value));
      break;
    case PROP_TEXT_SIZE_MULT:
        setTextSizeMultiplier(&src->gl_drawing, g_value_get_float(value));
        if(src->context){
            gst_gl_context_thread_add (src->context,
              (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
        }
      break;
    case PROP_ALIGN_X:
        {
          int v=1;
          if(strcmp(g_value_get_string(value),"left")==0)v=0;
          if(strcmp(g_value_get_string(value),"center")==0)v=1;
          if(strcmp(g_value_get_string(value),"right")==0)v=2;
          gldraw_set_align_x(&src->gl_drawing, v);
          if(src->context){
            gst_gl_context_thread_add (src->context,
              (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
          }
        }
      break;
    case PROP_ALIGN_Y:
        {
          int v=1;
          if(strcmp(g_value_get_string(value),"top")==0)v=0;
          if(strcmp(g_value_get_string(value),"center")==0)v=1;
          if(strcmp(g_value_get_string(value),"bottom")==0)v=2;
          gldraw_set_align_y(&src->gl_drawing, v);
          if(src->context){
            gst_gl_context_thread_add (src->context,
              (GstGLContextThreadFunc) gst_gl_text_overlay_close_gldraw, src);
          }
        }
      break;
    default:
      break;
  }
}

static void
gst_gl_text_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (object);

  switch (prop_id) {
    case PROP_TIMESTAMP_OFFSET:
      g_value_set_int64 (value, src->timestamp_offset);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (src)));
      break;
    case PROP_FONT_STYLE:
      g_value_set_string(value, gldraw_get_font_style(src));
      break;
    case PROP_BG_COLOR_ARGB:
      g_value_set_uint(value, getBGColor(&src->gl_drawing));
      break;
    case PROP_TEXT_COLOR_ARGB:
      g_value_set_uint(value, getTextColor(&src->gl_drawing));
      break;
    case PROP_TEXT_X:
      g_value_set_float(value, getTextX(&src->gl_drawing));
      break;
    case PROP_TEXT_Y:
      g_value_set_float(value, getTextY(&src->gl_drawing));
      break;
    case PROP_TEXT_SIZE_MULT:
      g_value_set_float(value, getTextSizeMultiplier(&src->gl_drawing));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_gl_text_overlay_setcaps (GstBaseSrc * bsrc, GstCaps * caps)
{
  GstGLTextOverlay *gltextoverlay = GST_GL_TEXT_OVERLAY (bsrc);

  GST_DEBUG ("setcaps");

  if (!gst_video_info_from_caps (&gltextoverlay->vinfo, caps))
    goto wrong_caps;

  gltextoverlay->negotiated = TRUE;

  gst_caps_replace (&gltextoverlay->out_caps, caps);

  return TRUE;

/* ERRORS */
wrong_caps:
  {
    GST_WARNING ("wrong caps");
    return FALSE;
  }
}

static void
gst_gl_text_overlay_set_context (GstElement * element, GstContext * context)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (element);

  gst_gl_handle_set_context (element, context, &src->display,
      &src->other_context);

  if (src->display)
    gst_gl_display_filter_gl_api (src->display, SUPPORTED_GL_APIS);

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);
}

static gboolean
gst_gl_text_overlay_query (GstBaseSrc * bsrc, GstQuery * query)
{
  gboolean res = FALSE;
  GstGLTextOverlay *src;

  src = GST_GL_TEXT_OVERLAY (bsrc);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
    {
      if (gst_gl_handle_context_query ((GstElement *) src, query,
              src->display, src->context, src->other_context))
        return TRUE;
      break;
    }
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      res =
          gst_video_info_convert (&src->vinfo, src_fmt, src_val, dest_fmt,
          &dest_val);
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);

      return res;
    }
    default:
      break;
  }

  return GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
}

static void
gst_gl_text_overlay_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  /* for live sources, sync on the timestamp of the buffer */
  if (gst_base_src_is_live (basesrc)) {
    GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

    if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
      /* get duration to calculate end time */
      GstClockTime duration = GST_BUFFER_DURATION (buffer);

      if (GST_CLOCK_TIME_IS_VALID (duration))
        *end = timestamp + duration;
      *start = timestamp;
    }
  } else {
    *start = -1;
    *end = -1;
  }
}

static gboolean
gst_gl_text_overlay_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  GstClockTime time;
  GstGLTextOverlay *src;

  src = GST_GL_TEXT_OVERLAY (bsrc);

  segment->time = segment->start;
  time = segment->position;

  /* now move to the time indicated */
  if (src->vinfo.fps_n) {
    src->n_frames = gst_util_uint64_scale (time,
        src->vinfo.fps_n, src->vinfo.fps_d * GST_SECOND);
  } else
    src->n_frames = 0;

  if (src->vinfo.fps_n) {
    src->running_time = gst_util_uint64_scale (src->n_frames,
        src->vinfo.fps_d * GST_SECOND, src->vinfo.fps_n);
  } else {
    /* FIXME : Not sure what to set here */
    src->running_time = 0;
  }

  g_return_val_if_fail (src->running_time <= time, FALSE);

  return TRUE;
}

static gboolean
gst_gl_text_overlay_is_seekable (GstBaseSrc * psrc)
{
  /* we're seekable... */
  return TRUE;
}

static gboolean
gst_gl_text_overlay_init_shader (GstGLTextOverlay * gltextoverlay)
{
  if (gst_gl_context_get_gl_api (gltextoverlay->context)) {
    /* blocking call, wait until the opengl thread has compiled the shader */
  }
  return TRUE;
}


static gboolean
gst_gl_text_overlay_callback (gpointer stuff)
{

  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (stuff);

  gboolean res=FALSE;
  float a,r,g,b;

  unsigned int bg_color=0x00ff00ff;

  if(src->gl_drawing.gl_drawing_created==0){

    a=(float)((bg_color & 0xff000000)>>24)/255.0;
    r=(float)((bg_color & 0x00ff0000)>>16)/255.0;
    g=(float)((bg_color & 0x0000ff00)>>8)/255.0;
    b=(float)((bg_color & 0x000000ff))/255.0;

    res =  gldraw_init (src->context, &src->gl_drawing,
                                                src->vinfo.width, src->vinfo.height,
                                                ((float)src->vinfo.fps_n)/((float)src->vinfo.fps_d),
                                                0,
                                                r,g,b,a);

    if(!res) {
      GST_ERROR (src, "Failed to initialize gldraw_init: \n %s",src->gl_drawing.error_message);
      gldraw_close (src->context, &src->gl_drawing);
      return FALSE;
    }

  }else {

   res=gldraw_render (src->context, &src->gl_drawing);

  }

  return res;

}


static void
_fill_gl (GstGLContext * context, GstGLTextOverlay * src)
{

  src->gl_result = gst_gl_framebuffer_draw_to_texture (src->fbo, src->out_tex,
      gst_gl_text_overlay_callback, src);

}

static GstFlowReturn
gst_gl_text_overlay_fill (GstPushSrc * psrc, GstBuffer * buffer)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (psrc);
  GstClockTime next_time;
  GstVideoFrame out_frame;
  GstGLSyncMeta *sync_meta;

  if (G_UNLIKELY (!src->negotiated || !src->context))
    goto not_negotiated;

  /* 0 framerate and we are at the second frame, eos */
  if (G_UNLIKELY (GST_VIDEO_INFO_FPS_N (&src->vinfo) == 0
          && src->n_frames == 1))
    goto eos;

  if (!gst_video_frame_map (&out_frame, &src->vinfo, buffer,
          GST_MAP_WRITE | GST_MAP_GL)) {
    return GST_FLOW_NOT_NEGOTIATED;
  }

  src->out_tex = (GstGLMemory *) out_frame.map[0].memory;

  gst_gl_context_thread_add (src->context, (GstGLContextThreadFunc) _fill_gl,
      src);
  if (!src->gl_result) {
    gst_video_frame_unmap (&out_frame);
    goto gl_error;
  }
  gst_video_frame_unmap (&out_frame);
  if (!src->gl_result)
    goto gl_error;

  sync_meta = gst_buffer_get_gl_sync_meta (buffer);
  if (sync_meta)
    gst_gl_sync_meta_set_sync_point (sync_meta, src->context);

  GST_BUFFER_TIMESTAMP (buffer) = src->timestamp_offset + src->running_time;
  GST_BUFFER_OFFSET (buffer) = src->n_frames;
  src->n_frames++;
  GST_BUFFER_OFFSET_END (buffer) = src->n_frames;
  if (src->vinfo.fps_n) {
    next_time = gst_util_uint64_scale_int (src->n_frames * GST_SECOND,
        src->vinfo.fps_d, src->vinfo.fps_n);
    GST_BUFFER_DURATION (buffer) = next_time - src->running_time;
  } else {
    next_time = src->timestamp_offset;
    /* NONE means forever */
    GST_BUFFER_DURATION (buffer) = GST_CLOCK_TIME_NONE;
  }

  src->running_time = next_time;

  return GST_FLOW_OK;

gl_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (_("failed to draw pattern")),
        (_("A GL error occurred")));
    return GST_FLOW_NOT_NEGOTIATED;
  }
not_negotiated:
  {
    GST_ELEMENT_ERROR (src, CORE, NEGOTIATION, (NULL),
        (_("format wasn't negotiated before get function")));
    return GST_FLOW_NOT_NEGOTIATED;
  }
eos:
  {
    GST_DEBUG_OBJECT (src, "eos: 0 framerate, frame %d", (gint) src->n_frames);
    return GST_FLOW_EOS;
  }
}

static gboolean
gst_gl_text_overlay_start (GstBaseSrc * basesrc)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (basesrc);

  if (!gst_gl_ensure_element_data (src, &src->display, &src->other_context))
    return FALSE;

  gst_gl_display_filter_gl_api (src->display, SUPPORTED_GL_APIS);

  src->running_time = 0;
  src->n_frames = 0;
  src->negotiated = FALSE;

  return TRUE;
}

static void
gst_gl_text_overlay_gl_stop (GstGLContext * context, GstGLTextOverlay * src)
{

  if (src->fbo)
    gst_object_unref (src->fbo);
  src->fbo = NULL;

  if(src->gl_drawing.gl_drawing_created==TRUE){
    gldraw_close (src->context, &src->gl_drawing);
    src->gl_drawing.gl_drawing_created=FALSE;
  }

}

static gboolean
gst_gl_text_overlay_stop (GstBaseSrc * basesrc)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (basesrc);

  if (src->context)
    gst_gl_context_thread_add (src->context,
        (GstGLContextThreadFunc) gst_gl_text_overlay_gl_stop, src);

  gst_caps_replace (&src->out_caps, NULL);

  if (src->context)
    gst_object_unref (src->context);
  src->context = NULL;

  return TRUE;
}

static gboolean
_find_local_gl_context (GstGLTextOverlay * src)
{
  if (gst_gl_query_local_gl_context (GST_ELEMENT (src), GST_PAD_SRC,
          &src->context))
    return TRUE;
  return FALSE;
}

static void
_src_generate_fbo_gl (GstGLContext * context, GstGLTextOverlay * src)
{
  src->fbo = gst_gl_framebuffer_new_with_default_depth (src->context,
      GST_VIDEO_INFO_WIDTH (&src->vinfo),
      GST_VIDEO_INFO_HEIGHT (&src->vinfo));
}

static gboolean
gst_gl_text_overlay_decide_allocation (GstBaseSrc * basesrc, GstQuery * query)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (basesrc);
  GstBufferPool *pool = NULL;
  GstStructure *config;
  GstCaps *caps;
  guint min, max, size;
  gboolean update_pool;
  GError *error = NULL;

  if (!gst_gl_ensure_element_data (src, &src->display, &src->other_context))
    return FALSE;

  gst_gl_display_filter_gl_api (src->display, SUPPORTED_GL_APIS);

  _find_local_gl_context (src);

  if (!src->context) {
    GST_OBJECT_LOCK (src->display);
    do {
      if (src->context) {
        gst_object_unref (src->context);
        src->context = NULL;
      }
      /* just get a GL context.  we don't care */
      src->context =
          gst_gl_display_get_gl_context_for_thread (src->display, NULL);
      if (!src->context) {
        if (!gst_gl_display_create_context (src->display, src->other_context,
                &src->context, &error)) {
          GST_OBJECT_UNLOCK (src->display);
          goto context_error;
        }
      }
    } while (!gst_gl_display_add_context (src->display, src->context));
    GST_OBJECT_UNLOCK (src->display);
  }

  if ((gst_gl_context_get_gl_api (src->context) & SUPPORTED_GL_APIS) == 0)
    goto unsupported_gl_api;

  if (src->context)
    gst_gl_context_thread_add (src->context,
        (GstGLContextThreadFunc) gst_gl_text_overlay_gl_stop, src);

  gst_gl_context_thread_add (src->context,
      (GstGLContextThreadFunc) _src_generate_fbo_gl, src);
  if (!src->fbo)
    goto context_error;

  gst_query_parse_allocation (query, &caps, NULL);

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);

    update_pool = TRUE;
  } else {
    GstVideoInfo vinfo;

    gst_video_info_init (&vinfo);
    gst_video_info_from_caps (&vinfo, caps);
    size = vinfo.size;
    min = max = 0;
    update_pool = FALSE;
  }

  if (!pool || !GST_IS_GL_BUFFER_POOL (pool)) {
    /* can't use this pool */
    if (pool)
      gst_object_unref (pool);
    pool = gst_gl_buffer_pool_new (src->context);
  }
  config = gst_buffer_pool_get_config (pool);

  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
  if (gst_query_find_allocation_meta (query, GST_GL_SYNC_META_API_TYPE, NULL))
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_GL_SYNC_META);
  gst_buffer_pool_config_add_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_GL_TEXTURE_UPLOAD_META);

  gst_buffer_pool_set_config (pool, config);

  if (update_pool)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);

  gst_gl_text_overlay_init_shader (src);

  gst_object_unref (pool);

  return TRUE;

unsupported_gl_api:
  {
    GstGLAPI gl_api = gst_gl_context_get_gl_api (src->context);
    gchar *gl_api_str = gst_gl_api_to_string (gl_api);
    gchar *supported_gl_api_str = gst_gl_api_to_string (SUPPORTED_GL_APIS);
    GST_ELEMENT_ERROR (src, RESOURCE, BUSY,
        ("GL API's not compatible context: %s supported: %s", gl_api_str,
            supported_gl_api_str), (NULL));

    g_free (supported_gl_api_str);
    g_free (gl_api_str);
    return FALSE;
  }
context_error:
  {
    if (error) {
      GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, ("%s", error->message),
          (NULL));
      g_clear_error (&error);
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL), (NULL));
    }
    if (src->context)
      gst_object_unref (src->context);
    src->context = NULL;
    return FALSE;
  }
}



static GstStateChangeReturn
gst_gl_text_overlay_change_state (GstElement * element, GstStateChange transition)
{
  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  GST_DEBUG_OBJECT (src, "changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_gl_ensure_element_data (element, &src->display,
              &src->other_context))
        return GST_STATE_CHANGE_FAILURE;
      gst_gl_display_filter_gl_api (src->display, SUPPORTED_GL_APIS);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      if (src->other_context) {
        gst_object_unref (src->other_context);
        src->other_context = NULL;
      }

      if (src->display) {
        gst_object_unref (src->display);
        src->display = NULL;
      }
      break;
    default:
      break;
  }

  return ret;
}

static void gst_gl_text_overlay_dispose (GObject * object){

  GstGLTextOverlay *src = GST_GL_TEXT_OVERLAY (object);
  G_OBJECT_CLASS (parent_class)->dispose (object);

}

static void
gst_gl_text_overlay_class_init (GstGLTextOverlayClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;
  GstElementClass *element_class;

  GST_DEBUG_CATEGORY_INIT (gl_text_overlay_debug, "gltextoverlay", 0,
      "Video Test Source");

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;
  element_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_gl_text_overlay_set_property;
  gobject_class->get_property = gst_gl_text_overlay_get_property;

  gobject_class->dispose = gst_gl_text_overlay_dispose;


  g_object_class_install_property (gobject_class,
      PROP_TIMESTAMP_OFFSET, g_param_spec_int64 ("timestamp-offset",
          "Timestamp offset",
          "An offset added to timestamps set on buffers (in ns)", G_MININT64,
          G_MAXINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "Is Live",
          "Whether to act as a live source", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TEXT,
      g_param_spec_string("text", "Text",
          "Text", "Default text",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FONT_CAPTION,
      g_param_spec_string("font-caption", "Font caption",
          "Font caption", "",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FONT_STYLE,
      g_param_spec_string("font-style", "Font style",
          "Font style", "",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_BG_COLOR_ARGB,
         g_param_spec_uint("bg-color-argb", "Backgound color ARGB",
                          "Backgound color ARGB",
                          0, G_MAXUINT32, 0xff000000,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property(gobject_class, PROP_TEXT_COLOR_ARGB,
         g_param_spec_uint("text-color-argb", "Text color ARGB",
                          "Text color ARGB",
                          0, G_MAXUINT32, 0xffffffff,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property(gobject_class, PROP_ALIGN_X,
         g_param_spec_string("align-x", "Align horizontal",
                          "Keywords: left, center, right",
                          "center", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property(gobject_class, PROP_ALIGN_Y,
         g_param_spec_string("align-y", "Align vertical",
                          "Keywords: top, center, bottom",
                          "center", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property(gobject_class, PROP_TEXT_X,
         g_param_spec_float("text-x", "Text x position",
                          "Text x position",
                          0.0, 1.0, 0.0,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property(gobject_class, PROP_TEXT_Y,
         g_param_spec_float("text-y", "Text y position",
                          "Text y position",
                          0.0, 1.0, 0.0,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property(gobject_class, PROP_TEXT_SIZE_MULT,
         g_param_spec_float("text-size-mult", "Text size multiplier",
                          "Text size multiplier. Positive number > 0.0",
                          0.0, 1000.0, 1.0,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

  gst_element_class_set_metadata (element_class, "gltextoverlay",
      "Gl display errors", "Gl display errors",
      "NIIT. Fedotov Ivan. <ivanfedotovmail@yandex.ru>");

  gst_element_class_add_static_pad_template (element_class, &src_factory);

  element_class->set_context = gst_gl_text_overlay_set_context;
  element_class->change_state = gst_gl_text_overlay_change_state;

  gstbasesrc_class->set_caps = gst_gl_text_overlay_setcaps;
  gstbasesrc_class->is_seekable = gst_gl_text_overlay_is_seekable;
  gstbasesrc_class->do_seek = gst_gl_text_overlay_do_seek;
  gstbasesrc_class->query = gst_gl_text_overlay_query;
  gstbasesrc_class->get_times = gst_gl_text_overlay_get_times;
  gstbasesrc_class->start = gst_gl_text_overlay_start;
  gstbasesrc_class->stop = gst_gl_text_overlay_stop;
  gstbasesrc_class->fixate = gst_gl_text_overlay_fixate;
  gstbasesrc_class->decide_allocation = gst_gl_text_overlay_decide_allocation;

  gstpushsrc_class->fill = gst_gl_text_overlay_fill;

}

static void
gst_gl_text_overlay_init (GstGLTextOverlay * src)
{

  src->timestamp_offset = 0;

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

  gldraw_first_init(&src->gl_drawing);

  GstGLTextOverlayClass *filter_class = GST_GL_TEXT_OVERLAY_CLASS (G_OBJECT_GET_CLASS (src));

}


static gboolean
gl_text_overlay_init (GstPlugin * gl_text_overlay)
{

  GST_DEBUG_CATEGORY_INIT (gl_text_overlay_debug, "gltextoverlay",
      0, "Template gltextoverlay");

  return gst_element_register (gl_text_overlay, "gltextoverlay", GST_RANK_NONE,
      GST_TYPE_GL_TEXT_OVERLAY);

}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "gltextoverlay"
#endif

/* gstreamer looks for this structure to register gl_text_overlays
 *
 * exchange the string 'Template gl_text_overlay' with your gl_text_overlay description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gltextoverlay,
    "Template gltextoverlay",
    gl_text_overlay_init,
    "1.0",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

