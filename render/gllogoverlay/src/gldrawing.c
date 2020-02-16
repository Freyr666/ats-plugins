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
 * Nuklear - 1.40.8 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2017 by Micha Mettke
 * emscripten from 2016 by Chris Willcocks
 * OpenGL ES 2.0 from 2017 by Dmitry Hrabrov a.k.a. DeXPeriX
 */

#include <gst/gst.h>
#include "gldrawing.h"

#include <stdlib.h>
#include <math.h>

#include <fontconfig/fontconfig.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "../../nuklear/nuklear.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
#define LEN(a) (sizeof(a)/sizeof(a)[0])

#include <string.h>
#include <time.h>

struct nk_sdl_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, ebo, vao;

    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
    GLsizei vs;
    size_t vp, vt, vc;
};

struct nk_sdl_vertex {
    GLfloat position[2];
    GLfloat uv[2];
    nk_byte col[4];
};

static struct nk_custom {

    struct nk_sdl_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;

    struct nk_buffer vbuf, ebuf;

    int vertices[MAX_VERTEX_MEMORY];
    int elements[MAX_ELEMENT_MEMORY];

};

#define NK_SHADER_VERSION "#version 150\n"


void gldraw_clear_set_pipeline_clock(GlDrawing *src, GstClock *_pipeline_clock){
  src->pipeline_clock=_pipeline_clock;
}

int gldraw_get_history_errors_window_size(GlDrawing *src){

  return src->history_errors_window_size;

}

int gldraw_get_history_errors_full_size(GlDrawing *src){

  return src->history_errors_window_size+1;

}

void gldraw_set_history_errors_window_size(GlDrawing *src, int value){

  if(value<1)value=1;
  if(value>history_errors_full_max_size-1)value=history_errors_full_max_size-1;
  src->history_errors_window_size=value;

}


void gldraw_set_text_color(GlDrawing *src, unsigned int color){

  src->color_text[0]=(float)((color & 0xff000000)>>24);
  src->color_text[1]=(float)((color & 0x00ff0000)>>16);
  src->color_text[2]=(float)((color & 0x0000ff00)>>8);
  src->color_text[3]=(float)((color & 0x000000ff));

}

unsigned int gldraw_get_text_color(GlDrawing *src){

  return (((unsigned int)src->color_text[0]))<<24 |
         (((unsigned int)src->color_text[1]))<<16 |
         (((unsigned int)src->color_text[2]))<<8  |
         (((unsigned int)src->color_text[3]));

}


const nk_rune nk_font_russian_glyph_ranges[] = {
  0x0020, 0x00FF,
  0x0400, 0x04FF,
  /*0x2200, 0x22FF, // Mathematical Operators
  0x0370, 0x03FF, // Greek and Coptic
  */
  0
};


static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "uniform float height;"
        "attribute vec2 Position;\n"
        "attribute vec2 TexCoord;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main() {\n"
        "   vec4 pos4;"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   pos4 = ProjMtx * vec4(Position.x,height-Position.y, 0.0, 1.0);\n"
        "   gl_Position = vec4(pos4.x,pos4.y,pos4.z,pos4.w);\n"
        "}\n";

static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
         "#endif\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "uniform int bg_fill_flag;"
        "uniform vec4 bg_color;"

        "void main(){\n"
        " if(bg_fill_flag==0){"
        "   gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV);\n"
        " }else{"
        "   gl_FragColor = bg_color;\n"
        " }"
        "}\n";



ErrorProperties errors_properties[SEVERITY_COUNT]=
{

    0.0,0.0,0.0,255,  0.0,255,0.0,255,  255,255,0,230,
    0.0,0.0,0.0,255,  255,255,0.0,255,  255,255,100,230,
    0.0,0.0,0.0,255,  255,200,0.0,255,  255,255,100,230,
    0.0,0.0,0.0,255,  255,160,0.0,255,  255,200,100,230,
    0.0,0.0,0.0,255,  255,100,0.0,255,  255,150,100,230,
    0.0,0.0,0.0,255,  255,0.0,0.0,255,  255,100,100,230

};



gboolean gldraw_add_error_code(GlDrawing *src, int code,int show_time_flag_override, double error_create_time_override, double error_last_time_override);


double getCurrentTime(){

  struct timespec tms;
  if(clock_gettime(CLOCK_REALTIME,&tms))return 0.0;
  return (double)tms.tv_sec+((double)(tms.tv_nsec))/1000000000.0;

}





void fire_error_draw_callback(GlDrawing *src, int draw_callback_event_type){

   DisplayedErrorData *history_ptrs[history_errors_full_max_size];
   int i;

   for(i=0;i<gldraw_get_history_errors_full_size(src);i++){
     history_ptrs[i]=&src->history_textlines[i].displayedErrorData;
   }

   if(src->error_draw_callback!=NULL){
     (*src->error_draw_callback)(src->error_draw_callback_receiver, src,
                                 draw_callback_event_type,
                                 history_ptrs,gldraw_get_history_errors_full_size(src),
                                 &src->big_textline.displayedErrorData,
                                 &src->big_flash_rect.displayedErrorData);
   }

}


void gldraw_set_error_draw_callback(GlDrawing *src,
                                    void *error_draw_callback_receiver,
                                    void (*_error_draw_callback)(void *, void *,
                                                                 int, DisplayedErrorData **, int,
                                                                 DisplayedErrorData *,
                                                                 DisplayedErrorData *)
                                    ){

  src->error_draw_callback=_error_draw_callback;
  src->error_draw_callback_receiver=error_draw_callback_receiver;

}

typedef struct{
  nk_rune rune;
  int rune_len;
}rune_ex;


int calc_cut_string_len_in_chars_num(struct nk_font *font_ptr,char *str,int max_pixels_str_len){

  if(font_ptr==NULL)return 0;

  int runes_sz,cur_pix_len,runes_less_num,chars_less_num;
  rune_ex rune;
  int i,inp_sz;
  int prev_rune_len;

  nk_handle handle1;
  handle1.ptr=font_ptr;

  inp_sz=strlen(str);
  if(inp_sz==0)return 0;
  runes_sz=nk_utf_len(str, inp_sz);
  if(runes_sz==0)return 0;

  cur_pix_len=0;
  runes_less_num=0;
  chars_less_num=0;
  prev_rune_len=0;
  for(i=0;i<runes_sz;i++){
    nk_utf_at(str,inp_sz,i,&rune.rune,&rune.rune_len);
    if(chars_less_num+rune.rune_len>inp_sz){
        break;
    }
    cur_pix_len+=nk_font_text_width(handle1,font_ptr->info.height,&str[chars_less_num],rune.rune_len);
    if(cur_pix_len>max_pixels_str_len){
        runes_less_num--;
        chars_less_num-=prev_rune_len;
        break;
    }
    prev_rune_len=rune.rune_len;
    chars_less_num+=rune.rune_len;
    runes_less_num++;

  }

  return chars_less_num;

}



float calc_bigtextline_ly(GlDrawing *src){

  return src->all_text_lines_ly_percent*((float)src->height)*0.3;

}


float calc_history_one_line_ly(GlDrawing *src){

  if(src->history_errors_window_size==0)return 0;
  return (src->all_text_lines_ly_percent*(float)src->height-calc_bigtextline_ly(src))/((float)src->history_errors_window_size);

}


float move_history_textlines_shift_y_one_frame(GlDrawing *src){

  if(src->history_textlines_shift_y<0.0)src->history_textlines_shift_y+=calc_history_one_line_ly(src)/src->fps;
  if(src->history_textlines_shift_y>0.0)src->history_textlines_shift_y=0.0;

}

float gldraw_move_history_textlines_shift_y_only_one_frame(GlDrawing *src){

  src->history_textlines_shift_y=-calc_history_one_line_ly(src);

}


void calc_one_line(GstClock *pipeline_clock,
                   TextRect *text_line,
                   float x, float y, float lx, float ly,
                   struct nk_font **_fonts_ptrs,
                   int fonts_num,
                   struct nk_font *_font_time_text_ptr,
                   float text_x_caption_begin_percent, float text_x_caption_end_percent,
                   float text_x_time_begin_percent, float text_x_time_end_percent,
                   float text_y,
                   float border_thick,
                   float *color_text //argb float[4]
                   ){

      int i;

      text_line->border_thick=border_thick;
      if(text_line->border_thick<2.0)text_line->border_thick=2.0;

      text_line->x=x+text_line->border_thick/2.0;
      text_line->y=y+text_line->border_thick/2.0;
      text_line->lx=lx-text_line->border_thick/2.0;
      text_line->ly=ly-text_line->border_thick/2.0;

      text_line->border_x=text_line->x;
      text_line->border_y=text_line->y;
      text_line->border_lx=text_line->lx;
      text_line->border_ly=text_line->ly;

      text_line->background_enabled_flag=1;

      for(i=0;i<fonts_num;i++){
        text_line->fonts_ptrs[i]=_fonts_ptrs[i];
      }
      text_line->fonts_ptrs_num=fonts_num;

      text_line->font_time_text_ptr=_font_time_text_ptr;

      int severity;

      severity=text_line->displayedErrorData.severity;
      severity=CLAMP(severity, 0, SEVERITY_COUNT-1);

      memcpy(text_line->border_color,
             errors_properties[severity].border_color,
             4*sizeof(float));

      memcpy(text_line->background_color,
             errors_properties[severity].background_color,
             4*sizeof(float));

      text_line->text_color[0]=color_text[1];
      text_line->text_color[1]=color_text[2];
      text_line->text_color[2]=color_text[3];
      text_line->text_color[3]=color_text[0];

      int len;
      int len_cut;
      int max_text_len_pixels;

      if(_fonts_ptrs!=NULL)text_line->text_ly=_fonts_ptrs[text_line->fonts_ptr_selected]->info.height;
      text_line->text_x=text_line->x+text_x_caption_begin_percent*text_line->lx;
      text_line->text_y=text_line->y+text_line->ly/2.0-text_line->text_ly/2.0;//text_line->y+text_y-text_line->border_thick/2.0;

      double showed_time;
      __uint64_t current_time;

      int d,h,m,s;
      int val;

      current_time=0;

      if(GST_IS_CLOCK(pipeline_clock)){
        current_time=gst_clock_get_time(pipeline_clock);
      }

      if(text_line->displayedErrorData.flag_is_continuous==1){
        showed_time=(current_time-text_line->displayedErrorData.timestamp)/1000000000;

        val=(int)(showed_time);

        d=val/60/60/24;
        val=val-d*60*60*24;

        h=val/60/60;
        val=val-h*60*60;

        m=val/60;
        val=val-m*60;

        s=val;

        if(d>0){
          sprintf(text_line->text_time,"%02dD:%02dH:%02dm:%02ds",d,h,m,s);
        }else
        if(h>0){
          sprintf(text_line->text_time,"%02dH:%02dm:%02ds",h,m,s);
        }else
        if(m>0){
          sprintf(text_line->text_time,"%02dm:%02ds",m,s);
        }else
        if(s>=0){
          sprintf(text_line->text_time,"%02ds",s);
        }

      }else{
        sprintf(text_line->text_time,"\0");
      }


      int len_pix;
      char buf1[100];
      nk_handle handle1;
      handle1.ptr=text_line->font_time_text_ptr;
      if(text_line->displayedErrorData.flag_is_continuous==1){
        sprintf(buf1,"___%s\0",text_line->text_time);
      }else{
        buf1[0]='\0';
      }

      len_pix=nk_font_text_width(handle1,text_line->font_time_text_ptr->info.height,buf1,strlen(buf1));
      text_line->text_lx=text_x_time_end_percent*text_line->lx-text_line->x-len_pix;

      len=strlen(text_line->displayedErrorData.msg);
      max_text_len_pixels=text_line->text_lx;


      int char_point_len_pix=0;

      for(i=0;i<text_line->fonts_ptrs_num;i++){

         char_point_len_pix=calc_cut_string_len_in_chars_num(text_line->fonts_ptrs[i],
                                                  ".",
                                                  10000);

         len_cut=calc_cut_string_len_in_chars_num(text_line->fonts_ptrs[i],
                                                  text_line->displayedErrorData.msg,
                                                  max_text_len_pixels-20-char_point_len_pix*3);
         text_line->fonts_ptr_selected=i;
         if(len==len_cut){
           break;
         }
      }
/*
      if(text_line->displayedErrorData.msg[0]=='2' && len_cut!=92){
        int aa;
        aa=0;
        len_cut=calc_cut_string_len_in_chars_num(text_line->fonts_ptrs[0],
                                                  text_line->displayedErrorData.msg,
                                                  max_text_len_pixels-10);
      }
*/

      if(len>0 && len<text_line_size){

         if(len_cut>0){

           memcpy(text_line->text_line,
                text_line->displayedErrorData.msg,
                len_cut);

           if(len!=len_cut){
             text_line->text_line[len_cut]='.';
             text_line->text_line[len_cut+1]='.';
             text_line->text_line[len_cut+2]='.';
             text_line->text_line[len_cut+3]=0;
           }else{
             text_line->text_line[len_cut]=0;
           }

         }else{
           text_line->text_line[0]=0;
         }

      }

      if(_fonts_ptrs!=NULL)text_line->text_time_ly=text_line->font_time_text_ptr->info.height;
      text_line->text_time_x=text_line->x+text_x_time_begin_percent*text_line->lx;
      text_line->text_time_y=text_line->y+text_line->ly/2.0-text_line->text_time_ly/2.0;
      text_line->text_time_x_zone_begin=text_line->x+text_x_time_begin_percent*text_line->lx;
      text_line->text_time_x_zone_end=text_line->x+text_x_time_end_percent*text_line->lx;

      handle1.ptr=_font_time_text_ptr;
      text_line->text_time_lx=nk_font_text_width(handle1,_font_time_text_ptr->info.height,text_line->text_time,strlen(text_line->text_time));




      //<<<
      //text_line->text_lx=1000;

}




void calc_all_draw_sizes(GlDrawing *src){


  int i;
  float x,y,lx,ly;
  float font_size_ly, font_size_ly_big;
  float enabled_errors_num=0;
  int first_line_lower_window;


  x=0.03*src->width;

  y=floor(src->all_text_lines_begin_y_percent*((float)src->height)+calc_bigtextline_ly(src));
  lx=src->width-x*2;
  ly=floor(calc_history_one_line_ly(src));

  struct nk_font *input_fonts[SELECTED_FONT_PTRS_NUM];

  input_fonts[0]=src->font_small_text_line;
  for(i=0;i<gldraw_get_history_errors_full_size(src);i++){

      calc_one_line(src->pipeline_clock,
                    &src->history_textlines[i],
                    x, y, lx, ly,
                    input_fonts,1,
                    src->font_small_text_line,
                    0.05, 0.68,
                    0.77, 0.95,
                    ly/2-src->font_small_text_line->info.height/2,
                    3.0,
                    src->color_text);


      y=y+ly;


      if(y+src->history_textlines_shift_y>src->all_text_lines_begin_y_percent*((float)src->height)+calc_bigtextline_ly(src)+calc_history_one_line_ly(src)*((float)src->history_errors_window_size)+0.01){
         src->history_textlines[i].displayedErrorData.flag_show_this=0;
      }

  }

  x=0.03*src->width;
  lx=src->width-x*2;
  y=floor(src->all_text_lines_begin_y_percent*((float)src->height));
  ly=floor(calc_bigtextline_ly(src));

  input_fonts[0]=src->font_big_text_line;
  calc_one_line(src->pipeline_clock,
                &src->big_textline,
                x, y, lx, ly,
                input_fonts,1,
                src->font_big_text_line,
                0.05, 0.68,
                0.77, 0.95,
                ly/2-src->font_big_text_line->info.height/2,
                3.0,
                src->color_text);


  calc_one_line(src->pipeline_clock,
                &src->big_flash_rect,
                0, 0, src->width, src->height,
                NULL,0,
                src->font_small_text_line,
                0, 0,
                0, 0,
                0,
                5.0,
                src->color_text);


}







void find_font_file(GlDrawing *src){

  FcConfig* config = FcInitLoadConfigAndFonts();
  FcPattern* pat = FcPatternCreate();
  FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
  FcFontSet* fs = FcFontList(config, pat, os);

  FcChar8 *file, *style, *family;
  file=NULL;
  style=NULL;
  family=NULL;

  int i;

  for (i=0; fs && i < fs->nfont; ++i) {
     FcPattern* font = fs->fonts[i];

     if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
         FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
         FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
     {

        if(strcmp(family,src->font_caption)==0 &&
           strcmp(style,src->font_style)==0){
            strcpy(src->font_full_filename,file);
            FcFontSetDestroy(fs);
            return;
        }

     }
  }
  if (fs) FcFontSetDestroy(fs);

  src->font_full_filename[0]=0;

}


void gldraw_set_font_style(GlDrawing *src, char *_font_style){

  strcpy(src->font_style, _font_style);

}

char *gldraw_get_font_style(GlDrawing *src){

  return src->font_style;

}

void gldraw_set_font_caption(GlDrawing *src, char *_font_caption){

  strcpy(src->font_caption, _font_caption);


}

char *gldraw_get_font_caption(GlDrawing *src){

  return src->font_caption;

}

void gldraw_first_init(GlDrawing *src){

  src->error_draw_callback=NULL;
  src->error_draw_callback_receiver=NULL;

  src->gl_drawing_created=0;

  src->font_full_filename[0]=0;
  gldraw_set_font_caption(src, FONT_CAPTION_DEFAULT);
  gldraw_set_font_style(src, FONT_STYLE_DEFAULT);

  src->all_context_nk=NULL;
  src->font_small_text_line=NULL;
  src->font_big_text_line=NULL;

  //argb
  src->color_background[0]=0;
  src->color_background[1]=255;
  src->color_background[2]=255;
  src->color_background[3]=255;

  src->color_text[0]=255;
  src->color_text[1]=0;
  src->color_text[2]=0;
  src->color_text[3]=0;


  src->all_text_lines_begin_y_percent=0.05;
  src->all_text_lines_ly_percent=0.9;


  src->history_textlines_shift_y=0;
  //seconds
  src->time_delta_continous_error_min=4.0;
  src->time_big_text_capture_time=3.0;
  src->time_big_rect_flash_delta=2.0;

  memset(&src->big_textline,0,sizeof(TextRect));
  memset(src->history_textlines,0,sizeof(TextRect)*history_errors_full_max_size);
  memset(&src->big_flash_rect,0,sizeof(TextRect));

  gldraw_set_history_errors_window_size(src, 5);

}


void gldraw_clear_errors(GlDrawing *src){

  src->history_textlines_shift_y=0;

  int i;

  src->big_textline.displayedErrorData.id=0;
  src->big_textline.displayedErrorData.flag_show_this=0;

  src->big_flash_rect.displayedErrorData.id=0;
  src->big_flash_rect.displayedErrorData.flag_show_this=0;

  for(i=0;i<history_errors_full_max_size;i++){
    src->history_textlines[i].displayedErrorData.id=0;
    src->history_textlines[i].displayedErrorData.flag_show_this=0;

  }

}


gboolean gldraw_init (GstGLContext * context, GlDrawing *src,
                      int width, int height,
                      float fps,
                      int direction,
                      float bg_color_r,
                      float bg_color_g,
                      float bg_color_b,
                      float bg_color_a)
{






    gldraw_close(context,src);

    find_font_file(src);
    if(src->font_full_filename[0]==0)return FALSE;

    src->width=width;
    src->height=height;
    src->fps=fps;

    src->history_textlines_shift_y=0;

    src->all_context_nk=malloc(sizeof(struct nk_custom));
    memset(&src->all_context_nk->ogl.cmds,0,sizeof(src->all_context_nk->ogl.cmds));

    nk_init_default(&src->all_context_nk->ctx, 0);

    src->all_context_nk->ctx.clip.userdata = nk_handle_ptr(0);

    GError *error = NULL;

    const GstGLFuncs *gl = context->gl_vtable;

    if(gl->GenVertexArrays){
      gl->GenVertexArrays (1, &src->all_context_nk->ogl.vao);
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }


    gl->GenBuffers (1, &src->all_context_nk->ogl.vbo);
    gl->GenBuffers (1, &src->all_context_nk->ogl.ebo);

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

    src->shader = gst_gl_shader_new_link_with_stages (context, &error,
     gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      vertex_shader),
     gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      fragment_shader), NULL);


    src->all_context_nk->ogl.attrib_pos = gst_gl_shader_get_attribute_location (src->shader, "Position");
    src->all_context_nk->ogl.attrib_uv = gst_gl_shader_get_attribute_location (src->shader, "TexCoord");
    src->all_context_nk->ogl.attrib_col = gst_gl_shader_get_attribute_location (src->shader, "Color");

    {
        src->all_context_nk->ogl.vs = sizeof(struct nk_sdl_vertex);
        src->all_context_nk->ogl.vp = offsetof(struct nk_sdl_vertex, position);
        src->all_context_nk->ogl.vt = offsetof(struct nk_sdl_vertex, uv);
        src->all_context_nk->ogl.vc = offsetof(struct nk_sdl_vertex, col);

    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    nk_font_atlas_init_default(&src->all_context_nk->atlas);
    nk_font_atlas_begin(&src->all_context_nk->atlas);

    const void *image;

    int font_w, font_h;

    struct nk_font_config conf = nk_font_config(0);
    conf.range = &nk_font_russian_glyph_ranges[0];

    int font_size;

    float w,h;

    w=width;
    h=height;

    font_size=calc_history_one_line_ly(src)/1.7;
    if(h*1.2>w)font_size*=w/h*3.0/4.0;
    if(font_size<5)font_size=5;
    src->font_small_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              src->font_full_filename,
                              font_size, &conf);



    font_size=font_size*1.5;
    src->font_big_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              src->font_full_filename,
                              font_size, &conf);


    image = nk_font_atlas_bake(&src->all_context_nk->atlas, &font_w, &font_h, NK_FONT_ATLAS_RGBA32);
    glGenTextures(1, &src->all_context_nk->ogl.font_tex);
    glBindTexture(GL_TEXTURE_2D, src->all_context_nk->ogl.font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)font_w, (GLsizei)font_h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);

    nk_font_atlas_end(&src->all_context_nk->atlas, nk_handle_id((int)src->all_context_nk->ogl.font_tex), &src->all_context_nk->ogl.null);
    if (src->all_context_nk->atlas.default_font)
        nk_style_set_font(&src->all_context_nk->ctx, &src->all_context_nk->atlas.default_font->handle);


    nk_handle handle1;
    handle1.ptr=src->font_small_text_line;

    src->all_context_nk->ctx.style.font = &src->font_small_text_line->handle;
    src->all_context_nk->ctx.stacks.fonts.head = 0;

    glGenTextures(1, &src->dummy_texture);

    glBindTexture(GL_TEXTURE_2D, src->dummy_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    src->gl_drawing_created=1;

    calc_all_draw_sizes(src);

    return TRUE;

}


gboolean gldraw_render(GstGLContext * context, GlDrawing *src)
{

    const GstGLFuncs *gl = context->gl_vtable;

    GLenum err;

    int i;

    if(src->font_full_filename[0]==0)return FALSE;


    fire_error_draw_callback(src, ERROR_DRAW_CALLBACK_EVENT_BEGIN_FRAME);

    if(src->history_textlines_shift_y>=0)fire_error_draw_callback(src, ERROR_DRAW_CALLBACK_EVENT_LIST_MOVE_END);


    calc_all_draw_sizes(src);
    move_history_textlines_shift_y_one_frame(src);


//------------------- очистка фона прозрачностью


    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };

    GLfloat positions[32];

    positions[0]=0.0;
    positions[1]=0.0;
    positions[2]=0.0;
    positions[3]=0.0;
    positions[4]=src->color_background[1];
    positions[5]=src->color_background[2];
    positions[6]=src->color_background[3];
    positions[7]=src->color_background[0];
    positions[8]=src->width;
    positions[9]=0.0;
    positions[10]=1.0;
    positions[11]=0.0;
    positions[12]=src->color_background[1];
    positions[13]=src->color_background[2];
    positions[14]=src->color_background[3];
    positions[15]=src->color_background[0];
    positions[16]=src->width;
    positions[17]=src->height;
    positions[18]=1.0;
    positions[19]=1.0;
    positions[20]=src->color_background[1];
    positions[21]=src->color_background[2];
    positions[22]=src->color_background[3];
    positions[23]=src->color_background[0];
    positions[24]=0.0;
    positions[25]=src->height;
    positions[26]=0.0;
    positions[27]=1.0;
    positions[28]=src->color_background[1];
    positions[29]=src->color_background[2];
    positions[30]=src->color_background[3];
    positions[31]=src->color_background[0];

    GLushort indices_quad[] = { 0, 1, 2, 0, 2, 3 };

    ortho[0][0] /= (GLfloat)src->width;
    ortho[1][1] /= (GLfloat)src->height;

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    if(gl->GenVertexArrays){
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

    gst_gl_shader_use (src->shader);

    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_uv);
    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_col);
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_pos, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)0);
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_uv, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(2*sizeof(float)));
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_col, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(4*sizeof(float)));

    glBindTexture(GL_TEXTURE_2D, src->dummy_texture);

    gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1i(src->shader, "bg_fill_flag", 1);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->color_background[1],src->color_background[2],src->color_background[3],src->color_background[0]);

    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof (gushort), indices_quad, GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
    gl->BindBuffer (GL_ARRAY_BUFFER, 0);
    gl->DisableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    gl->BindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    gst_gl_context_clear_shader(context);

    glDisable(GL_BLEND);


//-----------------------


    nk_buffer_init_default(&src->all_context_nk->ogl.cmds);

    struct nk_style_window window_default_padding;
    struct nk_style_window window_zero_padding;

    window_default_padding=src->all_context_nk->ctx.style.window;
    window_zero_padding=src->all_context_nk->ctx.style.window;

    window_zero_padding.contextual_padding = nk_vec2(0,0);
    window_zero_padding.combo_padding = nk_vec2(0,0);
    window_zero_padding.group_padding = nk_vec2(0,0);
    window_zero_padding.menu_padding = nk_vec2(0,0);
    window_zero_padding.min_row_height_padding = 0;
    window_zero_padding.padding = nk_vec2(0,0);
    window_zero_padding.popup_padding = nk_vec2(0,0);
    window_zero_padding.tooltip_padding = nk_vec2(0,0);
    window_zero_padding.header.label_padding = nk_vec2(0,0);
    window_zero_padding.header.padding = nk_vec2(0,0);
    window_zero_padding.spacing = nk_vec2(0,0);
    window_zero_padding.combo_border = 0;
    window_zero_padding.border = 0;
    window_zero_padding.contextual_border = 0;
    window_zero_padding.group_border = 0;
    window_zero_padding.menu_border = 0;
    window_zero_padding.popup_border = 0;
    window_zero_padding.tooltip_border = 0;
    window_zero_padding.scrollbar_size = nk_vec2(0,0);

    src->all_context_nk->ctx.style.window=window_zero_padding;

    src->all_context_nk->ctx.style.window.background=nk_rgba(0,0,0,0);
    src->all_context_nk->ctx.style.window.fixed_background.data.color=nk_rgba(0,0,0,0);

    if (nk_begin(&src->all_context_nk->ctx, "Demo", nk_rect(0, 0, src->width, src->height),NK_WINDOW_NO_SCROLLBAR))
    {

        struct nk_command_buffer *commands = nk_window_get_canvas(&src->all_context_nk->ctx);

        TextRect *tr;

        double cur_time=getCurrentTime();

        int blink_flag;

        blink_flag=0;
        if(cur_time-floor(cur_time/src->time_big_rect_flash_delta)*src->time_big_rect_flash_delta<0.5*src->time_big_rect_flash_delta)blink_flag=1;

        tr=&src->big_flash_rect;

        if(tr->displayedErrorData.flag_show_this==1){
          if(tr->displayedErrorData.flag_allow_rect_blink==1 && tr->displayedErrorData.flag_is_continuous==1){
            if(blink_flag==1){
              nk_stroke_rect(commands, nk_rect(tr->border_x,tr->border_y,tr->border_lx,tr->border_ly), 0, tr->border_thick,
                         nk_rgba(src->big_textline.border_color[0],
                                 src->big_textline.border_color[1],
                                 src->big_textline.border_color[2],
                                 src->big_textline.border_color[3]));
            }
          }else{
              nk_stroke_rect(commands, nk_rect(tr->border_x,tr->border_y,tr->border_lx,tr->border_ly), 0, tr->border_thick,
                         nk_rgba(src->big_textline.border_color[0],
                                 src->big_textline.border_color[1],
                                 src->big_textline.border_color[2],
                                 src->big_textline.border_color[3]));

          }
        }


        nk_layout_row_static(&src->all_context_nk->ctx,
                             floor(src->all_text_lines_begin_y_percent*((float)src->height)+calc_bigtextline_ly(src)),
                             src->width, 1);

        if (nk_group_begin(&src->all_context_nk->ctx, "Group0", NK_WINDOW_NO_SCROLLBAR)) {

            tr=&src->big_textline;

            if(tr->displayedErrorData.flag_show_this==1){
                if(tr->background_enabled_flag==1)
                  nk_fill_rect(commands, nk_rect(tr->x,tr->y,tr->lx,tr->ly), 0, nk_rgba(tr->background_color[0],
                                                                                         tr->background_color[1],
                                                                                         tr->background_color[2],
                                                                                         tr->background_color[3]));

                nk_stroke_rect(commands, nk_rect(tr->border_x,tr->border_y,tr->border_lx,tr->border_ly), 0, tr->border_thick,
                                 nk_rgba(tr->border_color[0],
                                         tr->border_color[1],
                                         tr->border_color[2],
                                         tr->border_color[3]));

                nk_draw_text(commands, nk_rect(tr->text_x,tr->text_y,tr->text_lx,tr->text_ly),
                               tr->text_line, strlen(tr->text_line), &tr->fonts_ptrs[tr->fonts_ptr_selected]->handle,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));


                if(tr->displayedErrorData.flag_show_time==1 && tr->displayedErrorData.flag_is_continuous==1){

                     nk_draw_text(commands, nk_rect(tr->text_time_x_zone_end-tr->text_time_lx,tr->text_time_y,tr->text_time_lx,tr->text_time_ly),

                               tr->text_time, strlen(tr->text_time), &tr->font_time_text_ptr->handle,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));
                }



            }

           nk_group_end(&src->all_context_nk->ctx);
        }

        nk_layout_row_static(&src->all_context_nk->ctx,
                             floor(src->history_textlines_shift_y+src->all_text_lines_ly_percent*((float)src->height)-calc_bigtextline_ly(src))-2,
                             src->width, 1);

        if (nk_group_begin(&src->all_context_nk->ctx, "Group1", NK_WINDOW_NO_SCROLLBAR)) {


            for(i=0;i<gldraw_get_history_errors_full_size(src);i++){
              tr=&src->history_textlines[i];

              if(tr->displayedErrorData.flag_show_this==1){

                  if(tr->background_enabled_flag==1)
                    nk_fill_rect(commands, nk_rect(tr->x,tr->y+src->history_textlines_shift_y,tr->lx,tr->ly), 0, nk_rgba(tr->background_color[0],
                                                                                         tr->background_color[1],
                                                                                         tr->background_color[2],
                                                                                         tr->background_color[3]));

                  nk_stroke_rect(commands, nk_rect(tr->border_x,tr->border_y+src->history_textlines_shift_y,tr->border_lx,tr->border_ly), 0, tr->border_thick,
                                 nk_rgba(tr->border_color[0],
                                         tr->border_color[1],
                                         tr->border_color[2],
                                         tr->border_color[3]));

                  nk_draw_text(commands, nk_rect(tr->text_x,tr->text_y+src->history_textlines_shift_y,tr->text_lx+200,tr->text_ly),
                               tr->text_line, strlen(tr->text_line), &tr->fonts_ptrs[tr->fonts_ptr_selected]->handle,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));

                if(tr->displayedErrorData.flag_show_time==1 &&
                   tr->displayedErrorData.flag_is_continuous==1){

                  nk_draw_text(commands, nk_rect(tr->text_time_x_zone_end-tr->text_time_lx,tr->text_time_y+src->history_textlines_shift_y,tr->text_time_lx,tr->text_time_ly),
                               tr->text_time, strlen(tr->text_time), &tr->font_time_text_ptr->handle,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));
                }


              }

            }

          nk_group_end(&src->all_context_nk->ctx);
        }

    }
    nk_end(&src->all_context_nk->ctx);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    if(gl->GenVertexArrays){
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

    gst_gl_shader_use (src->shader);

    gl->EnableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    gl->EnableVertexAttribArray (src->all_context_nk->ogl.attrib_uv);
    gl->EnableVertexAttribArray (src->all_context_nk->ogl.attrib_col);

    gl->VertexAttribPointer (src->all_context_nk->ogl.attrib_pos, 2, GL_FLOAT, GL_FALSE, src->all_context_nk->ogl.vs, (void*)src->all_context_nk->ogl.vp);
    gl->VertexAttribPointer (src->all_context_nk->ogl.attrib_uv, 2, GL_FLOAT, GL_FALSE, src->all_context_nk->ogl.vs, (void*)src->all_context_nk->ogl.vt);
    gl->VertexAttribPointer (src->all_context_nk->ogl.attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, src->all_context_nk->ogl.vs, (void*)src->all_context_nk->ogl.vc);

    gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1i(src->shader, "bg_fill_flag", 0);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->color_background[1],src->color_background[2],src->color_background[3],src->color_background[0]);

    const struct nk_draw_command *cmd;
    const nk_draw_index *offset = NULL;

    {
      // fill convert configuration
      struct nk_convert_config config;
      static const struct nk_draw_vertex_layout_element vertex_layout[] = {
       {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, position)},
       {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, uv)},
       {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_sdl_vertex, col)},
       {NK_VERTEX_LAYOUT_END}
      };

      memset(&config, 0, sizeof(config));
      config.vertex_layout = vertex_layout;
      config.vertex_size = sizeof(struct nk_sdl_vertex);
      config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
      config.null = src->all_context_nk->ogl.null;
      config.circle_segment_count = 36;
      config.curve_segment_count = 36;
      config.arc_segment_count = 36;
      config.global_alpha = 1.0f;
      config.shape_AA = NK_ANTI_ALIASING_OFF;
      config.line_AA = NK_ANTI_ALIASING_OFF;

      nk_buffer_init_fixed(&src->all_context_nk->vbuf, (void *)src->all_context_nk->vertices, (nk_size)MAX_VERTEX_MEMORY);
      nk_buffer_init_fixed(&src->all_context_nk->ebuf, (void *)src->all_context_nk->elements, (nk_size)MAX_ELEMENT_MEMORY);

      nk_convert(&src->all_context_nk->ctx, &src->all_context_nk->ogl.cmds, &src->all_context_nk->vbuf, &src->all_context_nk->ebuf, &config);

    }

    glBufferData(GL_ARRAY_BUFFER, (size_t)src->all_context_nk->ctx.draw_list.vertices->memory.size,
                    src->all_context_nk->ctx.draw_list.vertices->memory.ptr, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (size_t)src->all_context_nk->ctx.draw_list.elements->memory.size,
                   src->all_context_nk->ctx.draw_list.elements->memory.ptr, GL_STATIC_DRAW);

    // iterate over and execute each draw command
    nk_draw_foreach(cmd, &src->all_context_nk->ctx, &src->all_context_nk->ogl.cmds) {

      if (!cmd->elem_count) continue;
      glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
      err=glGetError();

      GLint x,y,lx,ly;
      x=(GLint)((cmd->clip_rect.x-1));
      y=(GLint)((cmd->clip_rect.y-1));
      lx=(GLint)((cmd->clip_rect.w+1));
      ly=(GLint)((cmd->clip_rect.h+1));

      glScissor(x,y,lx,ly);

      glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
      err=glGetError();
      offset += cmd->elem_count;

    }

    nk_clear(&src->all_context_nk->ctx);

    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
    gl->BindBuffer (GL_ARRAY_BUFFER, 0);
    gl->DisableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    gl->BindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    gst_gl_context_clear_shader(context);

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    nk_buffer_free(&src->all_context_nk->ogl.cmds);
    memset(&src->all_context_nk->ogl.cmds, 0, sizeof(src->all_context_nk->ogl.cmds));

    return TRUE;

}

void gldraw_close (GstGLContext * context, GlDrawing *src)
{

  const GstGLFuncs *gl = context->gl_vtable;

  if(src->all_context_nk!=NULL){

    nk_font_atlas_clear(&src->all_context_nk->atlas);
    nk_free(&src->all_context_nk->ctx);

    struct nk_sdl_device *dev = &src->all_context_nk->ogl;

    glDeleteTextures(1, &src->all_context_nk->ogl.font_tex);
    glDeleteBuffers(1, &src->all_context_nk->ogl.vbo);
    glDeleteBuffers(1, &src->all_context_nk->ogl.ebo);
    gl->DeleteVertexArrays(1, &src->all_context_nk->ogl.vao);
    nk_buffer_free(&src->all_context_nk->ogl.cmds);

    free(src->all_context_nk);

    glDeleteTextures(1,&src->dummy_texture);

  }
  src->all_context_nk=NULL;

  if(src->shader!=NULL){
    gst_object_unref(src->shader);
    src->shader=NULL;
  }

  src->gl_drawing_created=0;

}


