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

#define error_message_size 1000


struct nk_custom;

#define SEVERITY_COUNT (5+1)
#define history_errors_full_max_size 12
#define text_line_size 1000

#define FONT_CAPTION_DEFAULT "Noto Sans Display\0"
#define FONT_STYLE_DEFAULT "Regular\0"

typedef struct {
 int severity;

 __uint64_t id;

 __uint64_t delta_lasting;
 __uint64_t timestamp;
 //in secs
 int creation_time_s;
 int creation_time_m;
 int creation_time_h;
 int creation_time_d;

 char msg[512];

 int flag_show_this;
 int flag_show_time;
 int flag_show_msg;
 int flag_is_continuous;
 int flag_allow_rect_blink;

}DisplayedErrorData;


enum{

 ERROR_DRAW_CALLBACK_EVENT_BEGIN_FRAME=0,
 ERROR_DRAW_CALLBACK_EVENT_LIST_MOVE_END

};

#define SELECTED_FONT_PTRS_NUM 4


typedef struct {

    DisplayedErrorData displayedErrorData;

    float x,y,lx,ly;
    float border_x,border_y,border_lx,border_ly;

    float border_thick;
    //RGBA
    float border_color[4];

    float background_color[4];
    int  background_enabled_flag;//paint background or not

    float text_x, text_y, text_ly, text_lx;
    float text_color[4];
    char text_line[text_line_size];

    float text_time_x, text_time_y, text_time_ly, text_time_lx;
    float text_time_x_zone_begin;
    float text_time_x_zone_end;
    char text_time[text_line_size];

    struct nk_font *fonts_ptrs[SELECTED_FONT_PTRS_NUM];
    int fonts_ptrs_num;
    int fonts_ptr_selected;

    struct nk_font *font_time_text_ptr;

}TextRect;


typedef struct {

   float text_color[4];
   float border_color[4];
   float background_color[4];

}ErrorProperties;


typedef struct{

  //GUI

  struct nk_custom *all_context_nk;
  GstGLShader *shader;

  char font_caption[200];
  char font_style[200];//bold
  char font_full_filename[4000];

  struct nk_font *font_small_text_line;
  struct nk_font *font_medium_text_line;
  struct nk_font *font_big_text_line;

  //1. error_draw_callback_receiver
  //2. sender
  //3. ERROR_DRAW_CALLBACK_EVENT enum
  //4. history: array of DisplayedErrorData
  //5. history: element count in array of DisplayedErrorData (history_errors_full_size)
  //
  //6. big error plate
  //7. blink rectangle
  void (*error_draw_callback)(void *, void *,
                              int,
                              DisplayedErrorData **, int,
                              DisplayedErrorData *,
                              DisplayedErrorData *);
  void *error_draw_callback_receiver;


  double time_delta_continous_error_min;
  double time_big_text_capture_time;

  float all_text_lines_begin_y_percent;
  float all_text_lines_ly_percent;

  TextRect big_textline;

  //history lines window
  //заданное количество линий в истории (видимых в окне истории)
  int history_errors_window_size;

  //shift for move textlines
  float history_textlines_shift_y;
  //relative coords at history_textlines_y
  TextRect history_textlines[history_errors_full_max_size];

  double time_big_rect_flash_delta;
  TextRect big_flash_rect;

  char error_message[error_message_size];


  int width;
  int height;
  float fps;

  gboolean gl_drawing_created;

  float color_background[4];//[0,255]
  float color_text[4];//[0,255]

  GstClock *pipeline_clock;

  //для очистки альфа канала фона тем же шейдером
  GLuint dummy_texture;


}GlDrawing;

void gldraw_clear_errors(GlDrawing *src);

void gldraw_clear_set_pipeline_clock(GlDrawing *src, GstClock *_pipeline_clock);

int gldraw_get_history_errors_full_size(GlDrawing *src);

void gldraw_set_history_errors_window_size(GlDrawing *src, int value);

//Сдвигает историю на 1 линию вниз (но не ниже)
float gldraw_move_history_textlines_shift_y_only_one_frame(GlDrawing *src);

void gldraw_set_font_caption(GlDrawing *src, char *_font_caption);
void gldraw_set_font_style(GlDrawing *src, char *_font_style);
char *gldraw_get_font_caption(GlDrawing *src);
char *gldraw_get_font_style(GlDrawing *src);

void gldraw_first_init(GlDrawing *src);



void gldraw_set_text_color(GlDrawing *src, unsigned int color);
unsigned int gldraw_get_text_color(GlDrawing *src);


void gldraw_set_error_draw_callback(GlDrawing *src,
                                    void *error_draw_callback_receiver,
                                    void (*_error_draw_callback)(void *, void *,
                                                                 int, DisplayedErrorData **, int,
                                                                 DisplayedErrorData *,
                                                                 DisplayedErrorData *)
                                    );


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


