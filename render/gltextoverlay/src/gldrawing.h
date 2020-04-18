/*
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

#ifndef __GLDRAWING_H__
#define __GLDRAWING_H__

#include <glib.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>

#include <stdio.h>

#define MAX_ATTRIBUTES 4

struct nk_custom;

#define FONT_CAPTION_DEFAULT "Noto Serif Display"
#define FONT_STYLE_DEFAULT "Medium"

#define error_message_size 1000

#define TEXT_MAX_SIZE 4096

typedef struct{

  struct nk_custom *all_context_nk;
  GstGLShader *shader;

  char font_caption[200];
  char font_style[200];//bold
  char font_full_filename[4000];

  struct nk_font *font_small_text_line;

  //---

  char error_message[error_message_size];


  int width;
  int height;
  float fps;

  //public var:
  gboolean gl_drawing_created;


  GLuint text_texture;
  GLuint text_fbo;

  char text[TEXT_MAX_SIZE];

  //nk_rune array:
  unsigned int custom_range[TEXT_MAX_SIZE*2+1];

  //interval [0.0,1.0] values:
  float text_texture_x, text_texture_y, text_texture_lx, text_texture_ly;

  float text_size_multiplier;

  GLuint gstreamer_draw_fbo;

  float bgColor[4];
  float textColor[4];

  //0-top/left
  //1-middle
  //2-bottom/right
  int align_x;
  int align_y;


}GlDrawing;


//0=left, 1=center, 2=right
void gldraw_set_align_x(GlDrawing *src, int val);
//0=top, 1=center, 2=bottom
void gldraw_set_align_y(GlDrawing *src, int val);

void gldraw_set_font_caption(GlDrawing *src, char *_font_caption);
void gldraw_set_font_style(GlDrawing *src, char *_font_style);
char *gldraw_get_font_caption(GlDrawing *src);
char *gldraw_get_font_style(GlDrawing *src);

void gldraw_first_init(GlDrawing *src);

void gldraw_set_text(GlDrawing *src, char *_text);
char *gldraw_get_text(GlDrawing *src);

void setBGColor(GlDrawing *src, unsigned int color);
unsigned int getBGColor(GlDrawing *src);

void setTextColor(GlDrawing *src, unsigned int color);
unsigned int getTextColor(GlDrawing *src);

void setTextX(GlDrawing *src, float val);
float getTextX(GlDrawing *src);

void setTextY(GlDrawing *src, float val);
float getTextY(GlDrawing *src);

void setTextSizeMultiplier(GlDrawing *src, float val);
float getTextSizeMultiplier(GlDrawing *src);

gboolean gldraw_init (GstGLContext * context, GlDrawing *src,
                      int width, int height,
                      float fps,
                      int direction,
                      float bg_color_r,
                      float bg_color_g,
                      float bg_color_b,
                      float bg_color_a);

gboolean gldraw_render(GstGLContext * context, GlDrawing *src);
void gldraw_close (GstGLContext * context, GlDrawing *src);

#endif //__GLDRAWING_H__


