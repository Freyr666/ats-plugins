/* 71.6
 * before patch resize in device fixed! (code with freeze error!) patch: https://github.com/IvanFedotovIvanFedotov/glsoundbar_patches read patches_links.txt
 * resize with restart fixed
 *
 * Copyright (C) <2011> Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) <2015> Luis de Bethencourt <luis@debethencourt.com>
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
 *
 */

/**
 * SECTION:element-glsoundbar
 *
 * FIXME:Describe glsoundbar here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 *
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <gst/gl/gl.h>
#include <gst/gl/gstglutils.h>

#include <time.h>
#include <math.h>
#include <string.h>

#include <gst/audio/audio.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include "gstglsoundbar.h"
#include "gldrawing.h"

GST_DEBUG_CATEGORY_STATIC (gst_glsoundbar_debug);
#define GST_CAT_DEFAULT gst_glsoundbar_debug

#define USE_PEER_BUFFERALLOC
#define SUPPORTED_GL_APIS (GST_GL_API_ANY)

//  (GST_GL_API_OPENGL | GST_GL_API_OPENGL3 | GST_GL_API_GLES2)

enum
{

  PROP_0,
  PROP_BARS_DIRECTION,
  PROP_BG_COLOR_ARGB,
  PROP_LAST

};

#define gst_glsoundbar_parent_class parent_class
G_DEFINE_TYPE (GstGLSoundbar, gst_glsoundbar, GST_TYPE_ELEMENT);

#define GST_GLSOUNDBAR_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GLSOUNDBAR, GstGLSoundbarPrivate))

struct _GstGLSoundbarPrivate
{

  GstBufferPool *pool;
  gboolean pool_active;
  GstAllocator *allocator;
  GstAllocationParams params;
  GstQuery *query;

  // pads
  GstPad *srcpad, *sinkpad;
  int sinkpad_reconfigure_flag;

  GstAdapter *adapter;

  GstBuffer *inbuf;

  guint spf;                    // samples per video frame
  guint64 frame_duration;

  guint processed;

};

static GstStaticPadTemplate gst_glsoundbar_src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
        "format = (string) RGBA, "
        "width = " GST_VIDEO_SIZE_RANGE ", "
        "height = " GST_VIDEO_SIZE_RANGE ", "
        "framerate = " GST_VIDEO_FPS_RANGE ","
        "texture-target = (string) 2D")
    );


static GstStaticPadTemplate gst_glsoundbar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) S16LE, "
        "layout = (string) interleaved"
        )
    );

static void gst_glsoundbar_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_glsoundbar_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_glsoundbar_dispose (GObject * object);
static gboolean gst_glsoundbar_src_negotiate (GstGLSoundbar * scope);
static gboolean gst_glsoundbar_src_setcaps (GstGLSoundbar *
    scope, GstCaps * caps);
static gboolean gst_glsoundbar_sink_setcaps (GstGLSoundbar *
    scope, GstCaps * caps);
static GstFlowReturn gst_glsoundbar_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static gboolean gst_glsoundbar_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_glsoundbar_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);

static gboolean gst_glsoundbar_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static gboolean gst_glsoundbar_sink_query (GstPad * pad,
    GstObject * parent, GstQuery * query);

static GstStateChangeReturn gst_glsoundbar_change_state (GstElement *
    element, GstStateChange transition);

static gboolean
    default_decide_allocation (GstGLSoundbar * scope, GstQuery * query2);

static void
gst_glsoundbar_gl_stop (GstGLContext * context, GstGLSoundbar * src);

//must run in gl thread
static void
gst_glsoundbar_close_gldraw (GstGLContext * context, GstGLSoundbar * src);

static gboolean
_find_local_gl_context (GstGLSoundbar * src);

static void
_src_generate_fbo_gl (GstGLContext * context, GstGLSoundbar * src);

static void gst_glsoundbar_unreff_all(GstGLSoundbar * scope);

//static gboolean
//gst_glsoundbar_set_allocation (GstGLSoundbar * scope,
//    GstBufferPool * pool, GstAllocator * allocator,
//    GstAllocationParams * params, GstQuery * query);

static gboolean evaluate_loudness_s16le (loudness *result, GstAudioInfo *ainfo, gpointer *buf, int buf_frames_num);


static void
gst_glsoundbar_reset (GstGLSoundbar * scope)
{
  gst_adapter_clear (scope->priv->adapter);

  GST_OBJECT_LOCK (scope);

  scope->priv->processed = 0;
  GST_OBJECT_UNLOCK (scope);
}

static gboolean
gst_glsoundbar_sink_setcaps (GstGLSoundbar * scope, GstCaps * caps)
{
  GstAudioInfo info;
  int i;

  if (!gst_audio_info_from_caps (&info, caps))
    goto wrong_caps;

  scope->ainfo = info;

  GST_DEBUG_OBJECT (scope, "audio: channels %d, rate %d",
      GST_AUDIO_INFO_CHANNELS (&info), GST_AUDIO_INFO_RATE (&info));

  for(i=0;i<MAX_CHANNELS;i++){
    scope->result.loud_average[i]=0.0;
    scope->result.loud_peak[i]=0.0;
    scope->result.loud_average_db[i]=0.0;
    scope->result.loud_peak_db[i]=0.0;
    scope->result.channels=scope->ainfo.channels;
    scope->result.rate=scope->ainfo.rate;
  }

  gst_adapter_clear (scope->priv->adapter);

  scope->priv->sinkpad_reconfigure_flag=1;

  if(scope->context==NULL){
    if (!gst_glsoundbar_src_negotiate (scope)) {
      goto not_negotiated;
    }
  }

  return TRUE;

  // Errors
wrong_caps:
  {
    GST_WARNING_OBJECT (scope, "could not parse caps");
    return FALSE;
  }
not_negotiated:
  {
    GST_WARNING_OBJECT (scope, "failed to negotiate");
    return FALSE;
  }

}

static void gst_glsoundbar_unreff_all(GstGLSoundbar * scope){
    if (scope->priv->query) {
      gst_query_unref (scope->priv->query);
      scope->priv->query=NULL;
    }
    if (scope->priv->pool!=NULL) {
      gst_buffer_pool_set_active (scope->priv->pool, FALSE);
      gst_object_unref (scope->priv->pool);
      scope->priv->pool=NULL;
      scope->priv->pool_active=FALSE;
    }
    if (scope->context){
      gst_gl_context_thread_add (scope->context, (GstGLContextThreadFunc) gst_glsoundbar_gl_stop, scope);//-3 context +0 display
    }
    if(scope->context!=NULL){
      gst_object_unref(scope->context);
      scope->context=NULL;
    }
    if(scope->other_context!=NULL){
      gst_object_unref(scope->other_context);
      scope->other_context=NULL;
    }
    if(scope->display!=NULL){
      gst_object_unref(scope->display);
      scope->display=NULL;
    }
}

static gboolean
gst_glsoundbar_src_setcaps (GstGLSoundbar * scope, GstCaps * caps)
{
  GstVideoInfo info;

  gboolean res;

  if (!gst_video_info_from_caps (&info, caps))
    goto wrong_caps;

  scope->vinfo = info;

  scope->priv->frame_duration = gst_util_uint64_scale_int (GST_SECOND,
      GST_VIDEO_INFO_FPS_D (&info), GST_VIDEO_INFO_FPS_N (&info));
  scope->priv->spf =
      gst_util_uint64_scale_int (GST_AUDIO_INFO_RATE (&scope->ainfo),
      GST_VIDEO_INFO_FPS_D (&info), GST_VIDEO_INFO_FPS_N (&info));
  scope->req_spf = scope->priv->spf;

  gst_pad_set_caps (scope->priv->srcpad, caps);

  gst_glsoundbar_unreff_all(scope);

  GST_DEBUG_OBJECT (scope, "doing allocation query");
  scope->priv->query = gst_query_new_allocation (caps, TRUE);

/*
  if (!gst_pad_peer_query (scope->priv->srcpad, scope->priv->query)) {
    // not a problem, we use the query defaults
    GST_DEBUG_OBJECT (scope, "allocation query failed");
  }
*/

  GST_DEBUG_OBJECT (scope, "calling decide_allocation");

  res = default_decide_allocation (scope, scope->priv->query);

  gst_caps_unref (caps);

  return res;

  // ERRORS
wrong_caps:
  {
    GST_DEBUG_OBJECT (scope, "error parsing caps");
    return FALSE;
  }

}

static gboolean
default_decide_allocation (GstGLSoundbar * scope, GstQuery * query2)
{

  GstGLSoundbar *src = scope;

  GstStructure *config;
  GstCaps *caps;
  guint min, max, size;
  gboolean update_pool;
  GError *error = NULL;

  if (!gst_gl_ensure_element_data (src, &src->display, &src->other_context)){
    return FALSE;
  }

  src->running_time = 0;
  src->n_frames = 0;

  gst_gl_display_filter_gl_api(src->display, SUPPORTED_GL_APIS);


  if(scope->context==NULL){

    _find_local_gl_context (src);

    if (!scope->context){
      GST_OBJECT_LOCK (scope->display);
      do {
        if (scope->context){
          gst_object_unref (scope->context);
        }
        // just get a GL context.  we don't care
        scope->context = gst_gl_display_get_gl_context_for_thread (scope->display, NULL);
        if(scope->context!=NULL)scope->context_refs++;

        if (!scope->context) {
          if (!gst_gl_display_create_context (scope->display,
                  scope->other_context, &scope->context, &error)) {
            GST_OBJECT_UNLOCK (scope->display);
            goto context_error;
          }
        }
      } while (!gst_gl_display_add_context (scope->display, scope->context));
      GST_OBJECT_UNLOCK (scope->display);
    }

  }

  gst_gl_context_thread_add (scope->context, (GstGLContextThreadFunc) gst_glsoundbar_gl_stop, scope);

  if ((gst_gl_context_get_gl_api (src->context) & SUPPORTED_GL_APIS) == 0){
    goto unsupported_gl_api;
  }

  gst_gl_context_thread_add (src->context, (GstGLContextThreadFunc) _src_generate_fbo_gl, scope);

  if (!src->fbo){
    goto context_error;
  }

  gst_query_parse_allocation (scope->priv->query, &caps, NULL);

  if (gst_query_get_n_allocation_pools (scope->priv->query) > 0) {
    gst_query_parse_nth_allocation_pool (scope->priv->query, 0, &scope->priv->pool, &size, &min, &max);

    update_pool = TRUE;
  } else {
    GstVideoInfo vinfo;
    //guint min, max, size;
    gst_video_info_init (&vinfo);
    gst_video_info_from_caps (&vinfo, caps);
    size = vinfo.size;
    min = max = 0;
    update_pool = FALSE;
  }

  if (!scope->priv->pool || !GST_IS_GL_BUFFER_POOL (scope->priv->pool)) {
    // can't use this pool
    if (scope->priv->pool)
      gst_object_unref (scope->priv->pool);
    //context+1
    scope->priv->pool = gst_gl_buffer_pool_new (src->context);
  }
  config = gst_buffer_pool_get_config (scope->priv->pool);

  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
  if (gst_query_find_allocation_meta (scope->priv->query, GST_GL_SYNC_META_API_TYPE, NULL))
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_GL_SYNC_META);
  gst_buffer_pool_config_add_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_GL_TEXTURE_UPLOAD_META);

  gst_buffer_pool_set_config (scope->priv->pool, config);

  if (update_pool)
    gst_query_set_nth_allocation_pool (scope->priv->query, 0, scope->priv->pool, size, min, max);
  else
    gst_query_add_allocation_pool (scope->priv->query, scope->priv->pool, size, min, max);

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

static gboolean
gst_glsoundbar_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = FALSE;

  GstGLSoundbar *scope;

  scope = GST_GLSOUNDBAR (parent);

  switch (GST_QUERY_TYPE (query)) {
    //case GST_QUERY_ALLOCATION:
    //  _find_local_gl_context(scope);//'''
    case GST_QUERY_CONTEXT:

      if(scope->context!=NULL){
        GST_OBJECT_LOCK(scope);//'''
        if(gst_gl_handle_context_query ((GstElement *) scope, query,
             scope->display, scope->context,
             scope->other_context)){
                  GST_OBJECT_UNLOCK(scope);
                  return TRUE;
        }
        GST_OBJECT_UNLOCK(scope);
      }
      return FALSE;
      //res = gst_pad_query_default (pad, parent, query);
      break;
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static gboolean gst_glsoundbar_sink_query (GstPad * pad,
    GstObject * parent, GstQuery * query){

  gboolean res = FALSE;

  GstGLSoundbar *scope;

  scope = GST_GLSOUNDBAR (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:

      if(scope->context!=NULL){
        GST_OBJECT_LOCK(scope);//'''
        if(gst_gl_handle_context_query ((GstElement *) scope, query,
             scope->display, scope->context,
             scope->other_context)){
                  GST_OBJECT_UNLOCK(scope);
                  return TRUE;
        }
        GST_OBJECT_UNLOCK(scope);
      }
      return FALSE;

      break;
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;

}

/*
// takes ownership of the pool, allocator and query
static gboolean
gst_glsoundbar_set_allocation (GstGLSoundbar * scope,
    GstBufferPool * pool, GstAllocator * allocator,
    GstAllocationParams * params, GstQuery * query)
{
  GstAllocator *oldalloc=NULL;
  GstBufferPool *oldpool=NULL;
  GstQuery *oldquery=NULL;
  GstGLSoundbarPrivate *priv = scope->priv;

  GST_OBJECT_LOCK (scope);
  oldpool = priv->pool;
  priv->pool = pool;
  priv->pool_active = FALSE;

  oldalloc = priv->allocator;
  priv->allocator = allocator;

  oldquery = priv->query;
  priv->query = query;

  if (params)
    priv->params = *params;
  else
    gst_allocation_params_init (&priv->params);
  GST_OBJECT_UNLOCK (scope);

  if (oldpool) {
    GST_DEBUG_OBJECT (scope, "deactivating old pool %p", oldpool);
    gst_buffer_pool_set_active (oldpool, FALSE);
    gst_object_unref (oldpool);
  }
  if (oldalloc) {
    gst_object_unref (oldalloc);
  }
  if (oldquery) {
    gst_query_unref (oldquery);
  }
  return TRUE;
}
*/

static gboolean
gst_glsoundbar_src_negotiate (GstGLSoundbar * scope)
{
  GstCaps *othercaps, *target;
  GstStructure *structure;
  GstCaps *templ;
  gboolean ret;

  templ = gst_pad_get_pad_template_caps (scope->priv->srcpad);

  GST_DEBUG_OBJECT (scope, "performing negotiation");

  // see what the peer can do
  othercaps = gst_pad_peer_query_caps (scope->priv->srcpad, NULL);
  if (othercaps) {
    target = gst_caps_intersect (othercaps, templ);
    gst_caps_unref (othercaps);
    gst_caps_unref (templ);

    if (gst_caps_is_empty (target))
      goto no_format;

    target = gst_caps_truncate (target);
  } else {
    target = templ;
  }

  target = gst_caps_make_writable (target);
  structure = gst_caps_get_structure (target, 0);
  gst_structure_fixate_field_nearest_int (structure, "width", 640);
  gst_structure_fixate_field_nearest_int (structure, "height", 480);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate", 25, 1);
  if (gst_structure_has_field (structure, "pixel-aspect-ratio"))
    gst_structure_fixate_field_nearest_fraction (structure,
        "pixel-aspect-ratio", 1, 1);

  target = gst_caps_fixate (target);

  GST_DEBUG_OBJECT (scope, "final caps are %" GST_PTR_FORMAT, target);

  ret = gst_glsoundbar_src_setcaps (scope, target);

  return ret;

no_format:
  {
    gst_caps_unref (target);
    return FALSE;
  }
}


static gboolean
_find_local_gl_context (GstGLSoundbar * src)
{

  if (gst_gl_query_local_gl_context (GST_ELEMENT (src), GST_PAD_SRC,  &src->context)){
    return TRUE;
  }

  if (gst_gl_query_local_gl_context (GST_ELEMENT (src), GST_PAD_SINK,  &src->context)){
    return TRUE;
  }


  return FALSE;
}

static void
_src_generate_fbo_gl (GstGLContext * context, GstGLSoundbar * src)
{
  src->fbo = gst_gl_framebuffer_new_with_default_depth (src->context,
      GST_VIDEO_INFO_WIDTH (&src->vinfo),
      GST_VIDEO_INFO_HEIGHT (&src->vinfo));

}

static GstStateChangeReturn
gst_glsoundbar_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstGLSoundbar *scope;

  scope = GST_GLSOUNDBAR (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_glsoundbar_reset (scope);
      break;
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      //gst_glsoundbar_set_allocation (scope, NULL, NULL, NULL, NULL);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_glsoundbar_unreff_all(scope);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_glsoundbar_gl_stop (GstGLContext * context, GstGLSoundbar * src)
{

  if (src->fbo){
    gst_object_unref (src->fbo);
    src->fbo = NULL;
  }

  gst_glsoundbar_close_gldraw(context, src);

}


static void
gst_glsoundbar_close_gldraw (GstGLContext * context, GstGLSoundbar * src){
  if(src->gl_drawing_created==TRUE){
    gldraw_close (src->context, &src->gl_drawing, &src->result);
    src->gl_drawing_created=FALSE;
  }
}


static void
gst_glsoundbar_dispose (GObject * object)
{

  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  if(scope->priv->adapter!=NULL){
    gst_object_unref(scope->priv->adapter);
    scope->priv->adapter=NULL;
  }

  if(scope->priv->inbuf!=NULL){
    gst_buffer_unref(scope->priv->inbuf);
    scope->priv->inbuf=NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);

}

static GstFlowReturn
default_prepare_output_buffer (GstGLSoundbar * scope, GstBuffer ** outbuf)
{
  GstGLSoundbarPrivate *priv;

  priv = scope->priv;

  g_assert (priv->pool != NULL);

  // we can't reuse the input buffer
  if (!priv->pool_active) {
    GST_DEBUG_OBJECT (scope, "setting pool %p active", priv->pool);
    if (!gst_buffer_pool_set_active (priv->pool, TRUE))
      goto activate_failed;
    priv->pool_active = TRUE;
  }

  GstFlowReturn ret;

  ret=gst_buffer_pool_acquire_buffer (priv->pool, outbuf, NULL);

  return ret;

activate_failed:
  {
    GST_ELEMENT_ERROR (scope, RESOURCE, SETTINGS,
        ("failed to activate bufferpool"), ("failed to activate bufferpool"));
    return GST_FLOW_ERROR;
  }
}



static gboolean
gst_glsoundbar_callback (gpointer stuff)
{
  GstGLSoundbar *src = GST_GLSOUNDBAR (stuff);

  gboolean res=FALSE;
  float a,r,g,b;

  if(src->gl_drawing_created==FALSE){

    a=(float)((src->bg_color & 0xff000000)>>24)/255.0;
    r=(float)((src->bg_color & 0x00ff0000)>>16)/255.0;
    g=(float)((src->bg_color & 0x0000ff00)>>8)/255.0;
    b=(float)((src->bg_color & 0x000000ff))/255.0;

    res =  gldraw_init (src->context, &src->gl_drawing, &src->result,
                                                src->vinfo.width, src->vinfo.height,
                                                src->bars_draw_direction,
                                                r,g,b,a);

    src->gl_drawing_created=TRUE;

    if(!res) {
      GST_ERROR (src, "Failed to initialize gldraw_init: \n %s",src->gl_drawing.error_message);
      gldraw_close (src->context, &src->gl_drawing, &src->result);
      src->gl_drawing_created=FALSE;
      return FALSE;
    }

  }else {

   res=gldraw_render (src->context, &src->gl_drawing, &src->result);

  }

  return res;

}

static void
_fill_gl (GstGLContext * context, GstGLSoundbar * src)
{

  src->gl_result = gst_gl_framebuffer_draw_to_texture (src->fbo, src->out_tex,
      gst_glsoundbar_callback, src);

}

static GstFlowReturn
gst_glsoundbar_fill (GstGLSoundbar * psrc, GstBuffer * buffer)
{
  GstGLSoundbar *src = psrc;

  GstClockTime next_time;
  GstVideoFrame out_frame;
  GstGLSyncMeta *sync_meta;

  if (!gst_video_frame_map (&out_frame, &src->vinfo, buffer,
          GST_MAP_WRITE | GST_MAP_GL)) {
    return GST_FLOW_NOT_NEGOTIATED;
  }

  src->out_tex = (GstGLMemory *) out_frame.map[0].memory;

  gst_gl_context_thread_add (src->context, (GstGLContextThreadFunc) _fill_gl, src);

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

  src->n_frames++;
  next_time = gst_util_uint64_scale_int (src->n_frames * GST_SECOND,
        src->vinfo.fps_d, src->vinfo.fps_n);
  src->running_time = next_time;

  return GST_FLOW_OK;

gl_error:
  {
    return GST_FLOW_NOT_NEGOTIATED;
  }

}

static GstFlowReturn
gst_glsoundbar_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{

  GstFlowReturn ret = GST_FLOW_OK;
  GstGLSoundbar *scope;

  GstBuffer *inbuf;
  guint64 dist, ts;
  guint avail, sbpf;
  gpointer adata;
  gint bpf, rate;

  scope = GST_GLSOUNDBAR (parent);

  GST_LOG_OBJECT (scope, "chainfunc called");

  // resync on DISCONT
  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT)) {
    gst_adapter_clear (scope->priv->adapter);
  }

  GstCaps *othercaps=NULL;
  GstCaps *target=NULL;
  GstCaps *templ=NULL;
  GstStructure *structure=NULL;
  GstCaps *caps=NULL;
  GstVideoInfo info;
  guint min, max, size;
  gboolean update_pool;
  GstStructure *config;

  scope = GST_GLSOUNDBAR (parent);

  if((scope->priv->sinkpad_reconfigure_flag==1 ||
      gst_pad_check_reconfigure (scope->priv->srcpad)) &&
      scope->context!=NULL ){
    scope->priv->sinkpad_reconfigure_flag=0;
    templ = gst_pad_get_pad_template_caps (scope->priv->srcpad);
    othercaps = gst_pad_peer_query_caps (scope->priv->srcpad, NULL);
    if(othercaps==NULL){
      target = templ;
    }
    if(othercaps){
      target = gst_caps_intersect (othercaps, templ);
      if (gst_caps_is_empty (target)==TRUE){
        target = gst_caps_truncate (target);
      }
    }
    if (gst_caps_is_empty (target)==FALSE){
      target = gst_caps_make_writable (target);
      structure = gst_caps_get_structure (target, 0);
      gst_structure_fixate_field_nearest_int (structure, "width", 640);
      gst_structure_fixate_field_nearest_int (structure, "height", 480);
      gst_structure_fixate_field_nearest_fraction (structure, "framerate", 25, 1);
      if (gst_structure_has_field (structure, "pixel-aspect-ratio"))
        gst_structure_fixate_field_nearest_fraction (structure,
        "pixel-aspect-ratio", 1, 1);
      target = gst_caps_fixate (target);
      if (gst_video_info_from_caps (&info, target)==TRUE){
        scope->vinfo = info;
        scope->priv->frame_duration = gst_util_uint64_scale_int (GST_SECOND,
          GST_VIDEO_INFO_FPS_D (&info), GST_VIDEO_INFO_FPS_N (&info));
        scope->priv->spf =
          gst_util_uint64_scale_int (GST_AUDIO_INFO_RATE (&scope->ainfo),
          GST_VIDEO_INFO_FPS_D (&info), GST_VIDEO_INFO_FPS_N (&info));
        scope->req_spf = scope->priv->spf;
        gst_pad_set_caps (scope->priv->srcpad, target);
        if (scope->priv->pool!=NULL) {
          gst_buffer_pool_set_active (scope->priv->pool, FALSE);
          gst_object_unref (scope->priv->pool);
          scope->priv->pool=NULL;
          scope->priv->pool_active=FALSE;
        }
        if (scope->priv->query) {
          gst_query_unref (scope->priv->query);
          scope->priv->query=NULL;
        }
        scope->priv->query = gst_query_new_allocation (target, TRUE);
        gst_gl_context_thread_add (scope->context, (GstGLContextThreadFunc) gst_glsoundbar_gl_stop, scope);
        if ((gst_gl_context_get_gl_api (scope->context) & SUPPORTED_GL_APIS) == 0){
          //goto unsupported_gl_api;
          GST_DEBUG_OBJECT (scope, "GL. Reconfigure. Unsupported gl api");
        }
        gst_gl_context_thread_add (scope->context, (GstGLContextThreadFunc) _src_generate_fbo_gl, scope);
        if (!scope->fbo){
          GST_DEBUG_OBJECT (scope, "GL. Reconfigure. FBO not created");
        }
        gst_query_parse_allocation (scope->priv->query, &caps, NULL);
        if (gst_query_get_n_allocation_pools (scope->priv->query) > 0) {
          gst_query_parse_nth_allocation_pool (scope->priv->query, 0, &scope->priv->pool, &size, &min, &max);
          update_pool = TRUE;
        } else {
          size = scope->vinfo.size;
          min = max = 0;
          update_pool = FALSE;
        }
        if (!scope->priv->pool || !GST_IS_GL_BUFFER_POOL (scope->priv->pool)) {
          // can't use this pool
          if (scope->priv->pool){
            gst_object_unref (scope->priv->pool);
          }
          scope->priv->pool = gst_gl_buffer_pool_new (scope->context);
        }
        config = gst_buffer_pool_get_config (scope->priv->pool);

        gst_buffer_pool_config_set_params (config, caps, size, min, max);
        gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
        if (gst_query_find_allocation_meta (scope->priv->query, GST_GL_SYNC_META_API_TYPE, NULL))
          gst_buffer_pool_config_add_option (config,
              GST_BUFFER_POOL_OPTION_GL_SYNC_META);
        gst_buffer_pool_config_add_option (config,
            GST_BUFFER_POOL_OPTION_VIDEO_GL_TEXTURE_UPLOAD_META);

        gst_buffer_pool_set_config (scope->priv->pool, config);

        if (update_pool)
          gst_query_set_nth_allocation_pool (scope->priv->query, 0, scope->priv->pool, size, min, max);
        else
          gst_query_add_allocation_pool (scope->priv->query, scope->priv->pool, size, min, max);
        gst_caps_unref (caps);
      }
    }
    if(othercaps!=NULL)gst_caps_unref (othercaps);
    if(templ!=NULL)gst_caps_unref (templ);
    othercaps=NULL;
    templ=NULL;
  }

  rate = GST_AUDIO_INFO_RATE (&scope->ainfo);
  bpf = GST_AUDIO_INFO_BPF (&scope->ainfo);

  if (bpf == 0) {
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto beach;
  }

  gst_adapter_push (scope->priv->adapter, buffer);

  // this is what we want
  sbpf = scope->req_spf * bpf;

  inbuf = scope->priv->inbuf;
  // FIXME: the timestamp in the adapter would be different
  gst_buffer_copy_into (inbuf, buffer, GST_BUFFER_COPY_METADATA, 0, -1);

  // this is what we have
  avail = gst_adapter_available (scope->priv->adapter);
  GST_LOG_OBJECT (scope, "avail: %u, bpf: %u", avail, sbpf);
  while (avail >= sbpf) {
    GstBuffer *outbuf=NULL;
    // get timestamp of the current adapter content
    ts = gst_adapter_prev_pts (scope->priv->adapter, &dist);
    if (GST_CLOCK_TIME_IS_VALID (ts)) {
      // convert bytes to time
      ts += gst_util_uint64_scale_int (dist, GST_SECOND, rate * bpf);
    }

    ++scope->priv->processed;

    ret = default_prepare_output_buffer (scope, &outbuf);

    // recheck as the value could have changed
    sbpf = scope->req_spf * bpf;

    // no buffer allocated, we don't care why.
    if (ret != GST_FLOW_OK)
      break;

    // sync controlled properties
    if (GST_CLOCK_TIME_IS_VALID (ts))
      gst_object_sync_values (GST_OBJECT (scope), ts);

    GST_BUFFER_PTS (outbuf) = ts;
    GST_BUFFER_DURATION (outbuf) = scope->priv->frame_duration;

    // this can fail as the data size we need could have changed
    if (!(adata = (gpointer) gst_adapter_map (scope->priv->adapter, sbpf)))
      break;

    gst_buffer_replace_all_memory (inbuf,
        gst_memory_new_wrapped (GST_MEMORY_FLAG_READONLY, adata, sbpf, 0,
            sbpf, NULL, NULL));

    GstMapInfo amap;

    gst_buffer_map (inbuf, &amap, GST_MAP_READ);

    if(!evaluate_loudness_s16le (&scope->result, &scope->ainfo, (gpointer *)amap.data, amap.size / scope->ainfo.bpf)){
      gst_buffer_unmap (inbuf, &amap);
      goto audiosamplesbuf_proceed_error;
    }

    gst_buffer_unmap (inbuf, &amap);
    gst_glsoundbar_fill(scope, outbuf);
    scope->prev_push_outbuf=outbuf;
    ret = gst_pad_push (scope->priv->srcpad, outbuf);
/*
    if(ret!=GST_FLOW_OK){
      GST_DEBUG_OBJECT (scope, "gst_pad_push. error code=%d. ignore", ret);
      ret=GST_FLOW_OK;
    }
*/

    outbuf = NULL;

    // recheck as the value could have changed:
    sbpf = scope->req_spf * bpf;
    GST_LOG_OBJECT (scope, "avail: %u, bpf: %u", avail, sbpf);
    // we want to take less or more, depending on spf : req_spf
    if (avail - sbpf >= sbpf) {
      gst_adapter_flush (scope->priv->adapter, sbpf);
      gst_adapter_unmap (scope->priv->adapter);
    } else if (avail >= sbpf) {
      // just flush a bit and stop
      gst_adapter_flush (scope->priv->adapter, (avail - sbpf));
      gst_adapter_unmap (scope->priv->adapter);
      break;
    }
    avail = gst_adapter_available (scope->priv->adapter);

    if (ret != GST_FLOW_OK)
      break;
  }


beach:

  return ret;
/*
not_negotiated:
  {
    GST_DEBUG_OBJECT (scope, "Failed to renegotiate");
    g_mutex_unlock(&scope->mutex_chain_function);
    return GST_FLOW_NOT_NEGOTIATED;
  }
*/
audiosamplesbuf_proceed_error:
  {
    GST_DEBUG_OBJECT (scope, "Failed to audiosamplesbuf_proceed");

    return GST_FLOW_NOT_NEGOTIATED;
  }

}




static gboolean
gst_glsoundbar_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_RECONFIGURE:

      // dont't forward
      gst_event_unref (event);

      res = TRUE;
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static gboolean
gst_glsoundbar_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res;
  GstGLSoundbar *scope;

  scope = GST_GLSOUNDBAR (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      res = gst_glsoundbar_sink_setcaps (scope, caps);
      gst_event_unref (event);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      gst_glsoundbar_reset (scope);
      res = gst_pad_push_event (scope->priv->srcpad, event);
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static void
gst_glsoundbar_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{

  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  switch (prop_id) {
    case PROP_BARS_DIRECTION:
        scope->bars_draw_direction=g_value_get_int(value);
        if(scope->context){
          gst_gl_context_thread_add (scope->context,
            (GstGLContextThreadFunc) gst_glsoundbar_close_gldraw, scope);
        }
      break;
    case PROP_BG_COLOR_ARGB:
        scope->bg_color=g_value_get_uint(value);
        setBGColor(&scope->gl_drawing, scope->bg_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}

static void
gst_glsoundbar_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{

  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  switch (prop_id) {
    case PROP_BARS_DIRECTION:
        g_value_set_int(value,scope->bars_draw_direction);
      break;

    case PROP_BG_COLOR_ARGB:
        g_value_set_uint(value, scope->bg_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}

static void
gst_glsoundbar_set_context (GstElement * element, GstContext * context)
{
  GstGLSoundbar *src = GST_GLSOUNDBAR (element);

  gst_gl_handle_set_context (element, context, &src->display,
      &src->other_context);

  if (src->display)
    gst_gl_display_filter_gl_api (src->display, SUPPORTED_GL_APIS);

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);

}

static void
gst_glsoundbar_class_init (GstGLSoundbarClass * klass)
{

  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gst_element_class_set_details_simple(gstelement_class,
    "GLSoundbarBase",
    "GLSoundbar",
    "GLSoundbar Element",
    "NIITV Ivan Fedotov<ivanfedotovmail@yandex.ru");

  klass->supported_gl_api = GST_GL_API_ANY;

  g_type_class_add_private (klass, sizeof (GstGLSoundbarPrivate));

  gobject_class->set_property = gst_glsoundbar_set_property;
  gobject_class->get_property = gst_glsoundbar_get_property;
  gobject_class->dispose = gst_glsoundbar_dispose;


  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_glsoundbar_change_state);


  gstelement_class->set_context = gst_glsoundbar_set_context;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_glsoundbar_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_glsoundbar_sink_template);

  g_object_class_install_property
        (gobject_class, PROP_BARS_DIRECTION,
         g_param_spec_int("direction", "Bar draw direction",
                          "0 - direction to up, 1 - direction to right",
                          0, 1, 0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BG_COLOR_ARGB,
         g_param_spec_uint("bg-color-argb", "Backgound color ARGB",
                          "Backgound color ARGB",
                          0, G_MAXUINT32, 0xff000000,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

}

static void
gst_glsoundbar_init (GstGLSoundbar * filter)
{
  GstPadTemplate *pad_template;

  GstGLSoundbarClass *filter_class = GST_GLSOUNDBAR_CLASS (G_OBJECT_GET_CLASS (filter));

  filter->priv = GST_GLSOUNDBAR_GET_PRIVATE (filter);

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (filter_class), "sink");
  g_return_if_fail (pad_template != NULL);
  filter->priv->sinkpad = gst_pad_new_from_template (pad_template, "sink");
  gst_pad_set_chain_function (filter->priv->sinkpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_chain));
  gst_pad_set_event_function (filter->priv->sinkpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_sink_event));
  gst_element_add_pad (GST_ELEMENT (filter), filter->priv->sinkpad);

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (filter_class), "src");
  g_return_if_fail (pad_template != NULL);
  filter->priv->srcpad = gst_pad_new_from_template (pad_template, "src");
  gst_pad_set_event_function (filter->priv->srcpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_src_event));
  gst_pad_set_query_function (filter->priv->srcpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_src_query));

  gst_pad_set_query_function (filter->priv->sinkpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_sink_query));

  gst_element_add_pad (GST_ELEMENT (filter), filter->priv->srcpad);

  filter->priv->adapter = gst_adapter_new ();
  filter->priv->inbuf = gst_buffer_new ();

  filter->priv->pool=NULL;
  filter->priv->allocator=NULL;
  filter->priv->query=NULL;

  filter->priv->sinkpad_reconfigure_flag=0;

  // reset the initial video state
  gst_video_info_init (&filter->vinfo);
  filter->priv->frame_duration = GST_CLOCK_TIME_NONE;

  // reset the initial state
  gst_audio_info_init (&filter->ainfo);
  gst_video_info_init (&filter->vinfo);

  gldraw_first_init(&filter->gl_drawing);

  filter->gl_drawing_created=FALSE;

  filter->fbo=NULL;
  filter->out_tex=NULL;
  filter->display=NULL;
  filter->context=NULL;
  filter->other_context=NULL;
  filter->gl_result=FALSE;

  filter->context_refs=0;
  filter->display_refs=0;

  filter->priv->pool_active = FALSE;

  filter->running_time = 0;
  filter->n_frames = 0;

  filter->bars_draw_direction=GLSOUND_BAR_DRAW_DIRECTION_TO_UP;

  filter->bg_color=0xff000000;

  int i;
  for(i=0;i<MAX_CHANNELS;i++){
    filter->result.loud_average[i]=0.0;
    filter->result.loud_peak[i]=0.0;
    filter->result.loud_average_db[i]=0.0;
    filter->result.loud_peak_db[i]=0.0;
    filter->result.channels=0;
    filter->result.rate=0;
  }

  filter->prev_push_outbuf=NULL;

}

//S16LE
static gboolean evaluate_loudness_s16le (loudness *result, GstAudioInfo *ainfo, gpointer *buf, int buf_frames_num){

  if(!buf || !ainfo || !result)return FALSE;
  if(buf_frames_num<0)return FALSE;
  if(buf_frames_num==0)return TRUE;
  if(ainfo->rate<=0)return FALSE;

  float loud_average_norm;

  int i,fr;
  long long loud_average[MAX_CHANNELS] = {0};

  int volume_range;
  float dV;
  float average_fading=10.0;
  float peak_fading=0.5;
  float v;

  volume_range=65536/2;
  dV=((float)buf_frames_num)/((float)ainfo->rate);

  //RMS loud loud_average_norm
  for(i=0;i<ainfo->channels;i++){
    for(fr=0;fr<buf_frames_num;fr++){
      loud_average[i]+=(((signed short *)buf)[fr*ainfo->channels+i])*
                       (((signed short *)buf)[fr*ainfo->channels+i]);
    }

    loud_average_norm=sqrt(((float)loud_average[i])/((float)buf_frames_num))/((float)volume_range);
    result->loud_average[i]=(result->loud_average[i]+loud_average_norm*dV*average_fading)/(1.0+dV*average_fading);
    result->loud_average[i]=CLAMP(result->loud_average[i],0.0,1.0);

    //RMS loud peak
    result->loud_peak[i]-=result->loud_peak[i]*dV*peak_fading;
    result->loud_peak[i]=MAX(result->loud_peak[i],result->loud_average[i]);
    result->loud_peak[i]=CLAMP(result->loud_peak[i],0.0,1.0);

    v=20.0*log10(result->loud_average[i]);
    result->loud_average_db[i]=CLAMP(v/100.0+1.0,0.0,1.0);

    v=20.0*log10(result->loud_peak[i]);
    result->loud_peak_db[i]=CLAMP(v/100.0+1.0,0.0,1.0);
  }

  return TRUE;

}

static gboolean
glsoundbar_init (GstPlugin * glsoundbar)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template glsoundbar' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_glsoundbar_debug, "glsoundbar",
      0, "Template glsoundbar");

  return gst_element_register (glsoundbar, "glsoundbar", GST_RANK_NONE,
      GST_TYPE_GLSOUNDBAR);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "glsoundbar"
#endif

/* gstreamer looks for this structure to register glsoundbars
 *
 * exchange the string 'Template glsoundbar' with your glsoundbar description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    glsoundbar,
    "Template glsoundbar",
    glsoundbar_init,
    "1.0",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

