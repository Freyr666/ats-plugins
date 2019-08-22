/* test
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

#include <gst/gst.h>
#include "gstglsoundbar.h"

#include <stdlib.h>
#include <math.h>

//0
//-6 dB
//-24
//-42
//-60
#define AUDIO_LEVELS \
"const float audio_levels [6]=float[6](1.00, 0.94, 0.76, 0.58, 0.40, 0.0);"

#define COLOR_AUDIO_LEVELS \
"const vec4 color_audio_levels [5]=vec4[5]("\
"vec4(1.0,0.0,0.0,1.0)," \
"vec4(1.0,1.0,0.0,1.0)," \
"vec4(0.0,1.0,0.0,1.0)," \
"vec4(0.0,0.6,0.0,1.0)," \
"vec4(0.0,0.45,0.0,1.0)" \
");"


#define COLOR_PEAK "vec4(1.0,1.0,1.0,1.0)"

#define PEAK_SIZE 3.0
#define BAR_ASPECT 0.5

static const gchar *bar_vertex_src =
    "#version 120\n"
    "attribute vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}";

static const gchar *bar_fragment_src =
    "#version 120\n"
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform int  bars_num;\n"
    "uniform vec4 bg_color;\n"
    "uniform vec2 width_height; "
    "uniform float loud_average_arr["MAX_CHANNELS_AS_STR"];"
    "uniform float loud_peak_arr["MAX_CHANNELS_AS_STR"];"
    "uniform float bars_begin;\n"
    "uniform float bars_end;\n"
    "uniform float bar_len;\n"
    "uniform float bar_step;\n"
    "uniform bool direction;\n"
    "uniform float peak_size;"
    AUDIO_LEVELS
    COLOR_AUDIO_LEVELS
    "int bar_num;\n"
    "float bar_pos;\n"
    "float pos;\n"
    "float loud_average;"
    "float loud_peak;"
    "void main()\n"
    "{\n"
    "   vec2 coord_xy;\n"
    "   coord_xy=gl_FragCoord.xy/width_height;\n"
    "   if(!direction){"
    "     pos=coord_xy.x-bars_begin;\n"
    "   }"
    "   else{"
    "     pos=coord_xy.y-bars_begin;\n"
    "   }"
    "   bar_pos=mod(pos,bar_step);\n"
    "   bar_num=int(pos/bar_step);\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos>bar_len || pos<0.0){"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
    "   if(!direction){"
    "      loud_average=(1.0-loud_average_arr[bar_num]);\n"
    "      loud_peak=(1.0-loud_peak_arr[bar_num]);\n"
    "      loud_peak=max(loud_peak,peak_size);"
    "      if(coord_xy.y+peak_size>loud_peak && coord_xy.y<loud_peak){\n"
    "          gl_FragColor ="COLOR_PEAK";\n"
    "          return;\n"
    "      }\n"
    "      if(coord_xy.y>loud_average){\n"
    "          int index=0;"
    "          index=int(floor(-5.556*(1.0-coord_xy.y)+6.222));"
    "          index=min(index,4);"
    "            gl_FragColor=color_audio_levels[index];\n"
    "            return;\n"
    "      }\n"
    "   }"
    "   else{"
    "      loud_average=loud_average_arr[bar_num];\n"
    "      loud_peak=loud_peak_arr[bar_num];\n"
    "      loud_peak=max(loud_peak,peak_size);"
    "      if(coord_xy.x<loud_peak && coord_xy.x>loud_peak-peak_size){\n"
    "          gl_FragColor ="COLOR_PEAK";\n"
    "          return;\n"
    "      }\n"
    "      if(coord_xy.x<loud_average){\n"
    "          int index=0;"
    "          index=int(floor(-5.556*coord_xy.x+6.222));"
    "          index=min(index,4);"
    "            gl_FragColor=color_audio_levels[index];\n"
    "            return;\n"
    "      }\n"
    "   }"
    "   gl_FragColor = bg_color;\n"
    "}";

void gldraw_first_init(GlDrawing *src){

  src->shader=NULL;
  src->vao=0;
  src->vbo=0;
  src->vbo_indices=0;
  src->attributes_location = 0;
  memset(src->error_message, 0, error_message_size);

}

void setBGColor(GlDrawing *src, unsigned int color){

  src->bg_color.R=(float)((color & 0x00ff0000)>>16)/255.0;
  src->bg_color.G=(float)((color & 0x0000ff00)>>8)/255.0;
  src->bg_color.B=(float)((color & 0x000000ff))/255.0;
  src->bg_color.A=(float)((color & 0xff000000)>>24)/255.0;

}

gboolean gldraw_init (GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result,
                      int width, int height,
                      int direction,
                      float bg_color_r,
                      float bg_color_g,
                      float bg_color_b,
                      float bg_color_a)
{

  float channels;

  const GstGLFuncs *gl = context->gl_vtable;

  GError *error = NULL;

  gboolean ret;

  ret=TRUE;

  memset(src->error_message, 0, error_message_size);

  src->bg_color.R=bg_color_r;
  src->bg_color.G=bg_color_g;
  src->bg_color.B=bg_color_b;
  src->bg_color.A=bg_color_a;

  src->width=width;
  src->height=height;
  src->draw_direction=direction;

  if(audio_proceess_result->channels<=0)return FALSE;

  src->bar_aspect=1.0/(float)audio_proceess_result->channels*BAR_ASPECT;
  src->bar_quads_num=audio_proceess_result->channels;

  if((float)audio_proceess_result->channels<=0)return FALSE;

  channels=(float)audio_proceess_result->channels;

  src->bar_len=0.0;
  src->bar_step=0.0;
  src->bars_begin=0.0;
  src->bars_end=0.0;
  src->peak_size=0.0;

  if(height>0 && width>0){
    if(src->draw_direction==0){
      src->peak_size=PEAK_SIZE/height;
      src->bar_len=round(src->bar_aspect*width)/width;
      src->bar_step=round((src->bar_len+(1.0-src->bar_len*(channels))/(channels+2.0))*width)/width;
      src->bars_begin=round(((1.0-src->bar_step*channels)/2.0+(src->bar_step-src->bar_len)/2.0)*width)/width;
      src->bars_end=round((src->bars_begin+src->bar_step*channels+(src->bar_step-src->bar_len)/2.0)*width)/width;
    }else{
      src->peak_size=PEAK_SIZE/width;
      src->bar_len=round(src->bar_aspect*height)/height;
      src->bar_step=round((src->bar_len+(1.0-src->bar_len*(channels))/(channels+2.0))*height)/height;
      src->bars_begin=round(((1.0-src->bar_step*channels)/2.0+(src->bar_step-src->bar_len)/2.0)*height)/height;
      src->bars_end=round((src->bars_begin+src->bar_step*channels+(src->bar_step-src->bar_len)/2.0)*height)/height;
    }
  }

  if(gl->GenVertexArrays){
    gl->GenVertexArrays (1, &src->vao);
    gl->BindVertexArray (src->vao);
  }

  gl->GenBuffers (1, &src->vbo);
  gl->GenBuffers (1, &src->vbo_indices);

  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);

  src->shader = gst_gl_shader_new_link_with_stages (context, &error,
    gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      bar_vertex_src),
    gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      bar_fragment_src), NULL);

  if(!src->shader){
    int len;
    len=strlen(error->message);
    if(len>error_message_size)len=error_message_size;
    memcpy(src->error_message, error->message, len);
    ret=FALSE;
  }

  src->attributes_location = gst_gl_shader_get_attribute_location (src->shader, "position");
  gl->BufferData (GL_ARRAY_BUFFER, 4 * sizeof(XYZW), positions, GL_STATIC_DRAW);
  gl->BufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof (gushort), indices_quad, GL_STATIC_DRAW);
  gl->VertexAttribPointer (src->attributes_location, 4, GL_FLOAT, GL_FALSE, sizeof (GLfloat) * 4, (gpointer) (gintptr) 0);

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);
  gl->BindVertexArray (0);

  return ret;
}

gboolean gldraw_render(GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result)
{

  if(audio_proceess_result->channels<=0)return FALSE;

  const GstGLFuncs *gl = context->gl_vtable;
  if(gl->GenVertexArrays){
    gl->BindVertexArray (src->vao);
  }
  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);
  gst_gl_shader_use (src->shader);
  gl->EnableVertexAttribArray (src->attributes_location);

  gst_gl_shader_set_uniform_1i(src->shader, "bars_num", src->bar_quads_num);
  gst_gl_shader_set_uniform_4f(src->shader, "bg_color", src->bg_color.R, src->bg_color.G, src->bg_color.B, src->bg_color.A);
  gst_gl_shader_set_uniform_2f(src->shader, "width_height", src->width, src->height);
  gst_gl_shader_set_uniform_1fv(src->shader, "loud_average_arr", src->bar_quads_num, (float *)audio_proceess_result->loud_average_db);
  gst_gl_shader_set_uniform_1fv(src->shader, "loud_peak_arr", src->bar_quads_num, (float *)audio_proceess_result->loud_peak_db);
  gst_gl_shader_set_uniform_1f(src->shader, "bars_begin",src->bars_begin);
  gst_gl_shader_set_uniform_1f(src->shader, "bars_end",src->bars_end);
  gst_gl_shader_set_uniform_1f(src->shader, "bar_len",src->bar_len);
  gst_gl_shader_set_uniform_1f(src->shader, "bar_step",src->bar_step);
  gst_gl_shader_set_uniform_1i(src->shader, "direction", src->draw_direction);
  gst_gl_shader_set_uniform_1f(src->shader, "peak_size",src->peak_size);

  gl->DrawElements (GL_TRIANGLES, 6 , GL_UNSIGNED_SHORT,(gpointer) (gintptr) 0);

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);
  gl->DisableVertexAttribArray (src->attributes_location);
  if(gl->GenVertexArrays){
    gl->BindVertexArray(0);
  }
  gst_gl_context_clear_shader(context);

  return TRUE;

}

void gldraw_close (GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result)
{

  const GstGLFuncs *gl = context->gl_vtable;

  if(src->shader!=NULL){
    gst_object_unref(src->shader);
    src->shader=NULL;
  }

  gl->DeleteVertexArrays (1, &src->vao);
  gl->DeleteBuffers (1, &src->vbo);
  gl->DeleteBuffers (1, &src->vbo_indices);

  src->vao=0;
  src->vbo=0;
  src->vbo_indices=0;
  src->attributes_location = 0;
  memset(src->error_message, 0, error_message_size);

}










