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

//2 переменные должны быть оодинаковыми
#define MAX_CHANNELS 64
#define MAX_CHANNELS_AS_STR "64"

typedef struct {

  int channels;
  int rate;
  float loud_average[MAX_CHANNELS];
  float loud_peak[MAX_CHANNELS];
  float loud_average_db[MAX_CHANNELS];
  float loud_peak_db[MAX_CHANNELS];

}loudness;

//2 переменные должны быть оодинаковыми
#define AUDIO_LEVELS (5)
#define AUDIO_LEVELS_AS_STR "5"

#define MAX_ATTRIBUTES 4

#define error_message_size 1000

typedef struct
{
  gfloat X, Y, Z, W;
}XYZW;

/* *INDENT-OFF* */
static const GLfloat positions[] = {
     -1.0,  1.0,  0.0, 1.0,
      1.0,  1.0,  0.0, 1.0,
      1.0, -1.0,  0.0, 1.0,
     -1.0, -1.0,  0.0, 1.0,
};

static const GLushort indices_quad[] = { 0, 1, 2, 0, 2, 3 };
/* *INDENT-ON* */

typedef struct{
  int x1,y1,x2,y2;
}RECT_INT;

typedef struct{
  float x1,y1,x2,y2;
}RECT_FLOAT;

typedef struct{
  float R,G,B,A;
}COLOR_COMPONENTS;

typedef struct{

  GstGLShader *shader;
  unsigned int vao;
  unsigned int vbo;
  unsigned int vbo_indices;

  GLuint attributes_location;

  char error_message[error_message_size];

  int bar_quads_num;
  //На примере вертикального бара, рисуемого снизу вверх
  //При повороте переменные в данном блоке всеравно считаются относительно верха бара
  //Отношение ширины к длине 1 бара
  float bar_aspect;
  //Процент высоты риски к высоте бара
  float band_len_percent;
  //Процент расстояния между рисками к высоте бара
  float band_distanse_percent;
  //высота пика в процентах от высоты бара
  float peak_height_percent;

  float bars_begin;
  float bars_end;
  float bar_len;
  float bar_step;
  float band_len;
  float band_distanse;
  float peak_size;

  int draw_direction;

  int width;
  int height;

  COLOR_COMPONENTS bg_color;

  float audio_levels_in_pixels[AUDIO_LEVELS+1];


}GlDrawing;

void gldraw_first_init(GlDrawing *src);

void setBGColor(GlDrawing *src, unsigned int color);

gboolean gldraw_init (GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result,
                      int width, int height,
                      int direction,
                      float bg_color_r,
                      float bg_color_g,
                      float bg_color_b,
                      float bg_color_a);

gboolean gldraw_render(GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result);
void gldraw_close (GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result);

#endif //__GLDRAWING_H__


