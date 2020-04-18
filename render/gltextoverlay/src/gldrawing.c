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
 *
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




void setBGColor(GlDrawing *src, unsigned int color){

  src->bgColor[0]=(float)((color & 0xff000000)>>24)/255.0;
  src->bgColor[1]=(float)((color & 0x00ff0000)>>16)/255.0;
  src->bgColor[2]=(float)((color & 0x0000ff00)>>8)/255.0;
  src->bgColor[3]=(float)((color & 0x000000ff))/255.0;

}

unsigned int getBGColor(GlDrawing *src){

  return (((unsigned int)src->bgColor[0])*255 % 256)<<24 |
         (((unsigned int)src->bgColor[1])*255 % 256)<<16 |
         (((unsigned int)src->bgColor[2])*255 % 256)<<8  |
         (((unsigned int)src->bgColor[3])*255 % 256);

}

void setTextColor(GlDrawing *src, unsigned int color){

  src->textColor[0]=(float)((color & 0xff000000)>>24)/255.0;
  src->textColor[1]=(float)((color & 0x00ff0000)>>16)/255.0;
  src->textColor[2]=(float)((color & 0x0000ff00)>>8)/255.0;
  src->textColor[3]=(float)((color & 0x000000ff))/255.0;

}

unsigned int getTextColor(GlDrawing *src){

  return (((unsigned int)src->textColor[0])*255 % 256)<<24 |
         (((unsigned int)src->textColor[1])*255 % 256)<<16 |
         (((unsigned int)src->textColor[2])*255 % 256)<<8  |
         (((unsigned int)src->textColor[3])*255 % 256);

}


void gldraw_set_text(GlDrawing *src, char *_text){
  if(strlen(_text)>TEXT_MAX_SIZE)return;
  strcpy(src->text,_text);
}

char *gldraw_get_text(GlDrawing *src){
  return src->text;
}

void setTextX(GlDrawing *src, float val){
  src->text_texture_x=val;
}
float getTextX(GlDrawing *src){
  return src->text_texture_x;
}

void setTextY(GlDrawing *src, float val){
  src->text_texture_y=val;
}
float getTextY(GlDrawing *src){
  return src->text_texture_y;
}

void setTextSizeMultiplier(GlDrawing *src, float val){
  src->text_size_multiplier=val;
}
float getTextSizeMultiplier(GlDrawing *src){
  return src->text_size_multiplier;
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
        "uniform float text_x;"
        "uniform float text_y;"
        "uniform float text_lx;"
        "uniform float text_ly;"
        "uniform int draw_bg_flag;"
        "uniform vec4 text_color;"
        "uniform vec4 bg_color;"
        "void main(){\n"
        "  if(draw_bg_flag==0){"
        "    gl_FragColor = Frag_Color*texture2D(Texture, Frag_UV);\n"
        "  }else{"
        "    gl_FragColor = bg_color;\n"
        "  }"
        "}\n";




typedef struct{
  nk_rune rune;
  int rune_len;
}rune_ex;

//return 1 if no errors
int custom_range_gliphs(GlDrawing *src, char *input_text){

  int inp_sz,runes_sz;
  int i,sorted;
  int pos;
  int duplicates;
  rune_ex rune;
  rune_ex buf[TEXT_MAX_SIZE];

  inp_sz=strlen(input_text);

  if(inp_sz==0 || inp_sz>=TEXT_MAX_SIZE)return 0;

  runes_sz=nk_utf_len(input_text, inp_sz);

  if(runes_sz==0)return 0;

  for(i=0;i<runes_sz;i++){
    nk_utf_at(input_text,inp_sz,i,&buf[i].rune,&buf[i].rune_len);
  }

  sorted=0;
  while(!sorted){
    sorted=1;
    for(i=0;i<runes_sz-1;i++){
      if(buf[i].rune>buf[i+1].rune){
        rune=buf[i];
        buf[i]=buf[i+1];
        buf[i+1]=rune;
        sorted=0;
      }
    }
  }

  //remove duplicates symbols
  pos=0;
  for(i=0;i<runes_sz-1;i++){
    duplicates=0;
    while(buf[i].rune==buf[i+1].rune){
      i++;
      duplicates=1;
      if(i+1>=runes_sz)break;
    }
    if(pos+1<runes_sz && i+1<runes_sz){
      buf[pos+1]=buf[i+1];
      pos++;
    }
  }

  runes_sz=pos+1;
  if(runes_sz==0)return 0;

  for(i=0;i<runes_sz;i++){
    src->custom_range[i*2]=buf[i].rune;
    src->custom_range[i*2+1]=buf[i].rune;
  }
  src->custom_range[runes_sz*2]=0;

  return 1;

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

  strcpy(src->text,"\0");
  src->gl_drawing_created=0;

  src->font_full_filename[0]=0;
  gldraw_set_font_caption(src, FONT_CAPTION_DEFAULT);
  gldraw_set_font_style(src, FONT_STYLE_DEFAULT);

  src->all_context_nk=NULL;
  src->font_small_text_line=NULL;

  src->text_texture=0;
  src->text_fbo=0;

  src->bgColor[0]=0.7;
  src->bgColor[1]=1.0;
  src->bgColor[2]=1.0;
  src->bgColor[3]=1.0;

  src->textColor[0]=1.0;
  src->textColor[1]=1.0;
  src->textColor[2]=0.0;
  src->textColor[3]=0.0;

  src->text_texture_x=0;
  src->text_texture_y=0;
  src->text_texture_lx=1.0;
  src->text_texture_ly=1.0;

  src->text_size_multiplier=1.0;

  src->align_x=1;
  src->align_y=1;

  src->custom_range[0]=32;
  src->custom_range[1]=32;
  src->custom_range[2]=0;

}


void gldraw_set_align_x(GlDrawing *src, int val){

  src->align_x=val;
  if(src->align_x<0 || src->align_x>2)src->align_x=1;

}

void gldraw_set_align_y(GlDrawing *src, int val){

  src->align_y=val;
  if(src->align_y<0 || src->align_y>2)src->align_y=1;

}

void calc_texture_xy_at_align(GlDrawing *src){

  nk_handle handle1;
  handle1.ptr=src->font_small_text_line;
  float len_pix;

  if(src->width==0 || src->height==0)return;

  len_pix=nk_font_text_width(handle1, src->font_small_text_line->info.height, src->text, strlen(src->text));

  if(src->align_x==0){
    src->text_texture_x=0;
  }else
  if(src->align_x==1){
    src->text_texture_x=(src->width-len_pix)/2.0;
  }else
  if(src->align_x==2){
    src->text_texture_x=src->width-len_pix;
  }
  src->text_texture_x/=(float)src->width;

  if(src->align_y==0){
    src->text_texture_y=-src->font_small_text_line->info.height*0.20;
  }else
  if(src->align_y==1){
    src->text_texture_y=(src->height-src->font_small_text_line->info.height*1.15)/2.0;
  }else
  if(src->align_y==2){
    src->text_texture_y=src->height-src->font_small_text_line->info.height*0.85;
  }
  src->text_texture_y/=(float)src->height;

  int a;
  a=0;

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

    custom_range_gliphs(src,src->text);

    src->width=width;
    src->height=height;
    src->fps=fps;


    src->all_context_nk=malloc(sizeof(struct nk_custom));
    memset(&src->all_context_nk->ogl.cmds,0,sizeof(src->all_context_nk->ogl.cmds));

    nk_init_default(&src->all_context_nk->ctx, 0);

    src->all_context_nk->ctx.clip.userdata = nk_handle_ptr(0);


    GError *error = NULL;

    const GstGLFuncs *gl = context->gl_vtable;


//--- create new fbo


     glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &src->gstreamer_draw_fbo);

     glGenFramebuffers(1,&src->text_fbo);
     glGenTextures(1,&src->text_texture);

     glBindFramebuffer(GL_FRAMEBUFFER, src->text_fbo);
     glBindTexture(GL_TEXTURE_2D, src->text_texture);

     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, floor(src->width), floor(src->height), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);

     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src->text_texture, 0);

     glViewport(0, 0, floor(src->width), floor(src->height));

     glClear(GL_COLOR_BUFFER_BIT);


//--- draw backround as pure alpha (if need alpha)


    src->shader = gst_gl_shader_new_link_with_stages (context, &error,
     gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      vertex_shader),
     gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      fragment_shader), NULL);


    if(gl->GenVertexArrays){
      gl->GenVertexArrays (1, &src->all_context_nk->ogl.vao);
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }

    gl->GenBuffers (1, &src->all_context_nk->ogl.vbo);
    gl->GenBuffers (1, &src->all_context_nk->ogl.ebo);


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
    positions[4]=src->bgColor[1];
    positions[5]=src->bgColor[2];
    positions[6]=src->bgColor[3];
    positions[7]=src->bgColor[0];
    positions[8]=src->width;
    positions[9]=0.0;
    positions[10]=1.0;
    positions[11]=0.0;
    positions[12]=src->bgColor[1];
    positions[13]=src->bgColor[2];
    positions[14]=src->bgColor[3];
    positions[15]=src->bgColor[0];
    positions[16]=src->width;
    positions[17]=src->height;
    positions[18]=1.0;
    positions[19]=1.0;
    positions[20]=src->bgColor[1];
    positions[21]=src->bgColor[2];
    positions[22]=src->bgColor[3];
    positions[23]=src->bgColor[0];
    positions[24]=0.0;
    positions[25]=src->height;
    positions[26]=0.0;
    positions[27]=1.0;
    positions[28]=src->bgColor[1];
    positions[29]=src->bgColor[2];
    positions[30]=src->bgColor[3];
    positions[31]=src->bgColor[0];

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

    src->all_context_nk->ogl.attrib_pos = gst_gl_shader_get_attribute_location (src->shader, "Position");
    src->all_context_nk->ogl.attrib_uv = gst_gl_shader_get_attribute_location (src->shader, "TexCoord");
    src->all_context_nk->ogl.attrib_col = gst_gl_shader_get_attribute_location (src->shader, "Color");

    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_uv);
    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_col);
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_pos, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)0);
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_uv, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(2*sizeof(float)));
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_col, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(4*sizeof(float)));

    glBindTexture(GL_TEXTURE_2D, src->text_texture);

    gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1f(src->shader, "text_x", src->text_texture_x*src->width);
    gst_gl_shader_set_uniform_1f(src->shader, "text_y", src->text_texture_y*src->height);
    gst_gl_shader_set_uniform_1f(src->shader, "text_lx", floor(src->width));
    gst_gl_shader_set_uniform_1f(src->shader, "text_ly", floor(src->height));
    gst_gl_shader_set_uniform_1i(src->shader, "draw_bg_flag", 1);
    gst_gl_shader_set_uniform_4f(src->shader, "text_color",src->textColor[1],src->textColor[2],src->textColor[3],src->textColor[0]);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->bgColor[1],src->bgColor[2],src->bgColor[3],src->bgColor[0]);

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

//--- draw text

    gl->BindVertexArray (src->all_context_nk->ogl.vao);

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

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

    conf.range=(nk_rune*)src->custom_range;

    int font_size;
    float w,h;


    font_size=floor(src->text_texture_ly*height*src->text_size_multiplier);//*w/h*3.0/4.0;
    if(font_size<5)font_size=5;
    src->font_small_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
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

//---

    calc_texture_xy_at_align(src);

//---


    GLenum err;
    int i;

    glBindTexture(GL_TEXTURE_2D, src->text_texture);
    glBindTexture(GL_TEXTURE_2D, 0);

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

    if (nk_begin(&src->all_context_nk->ctx, "Demo", nk_rect(0, 0, floor(src->width),
                                                             floor(src->height)),NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_command_buffer *commands = nk_window_get_canvas(&src->all_context_nk->ctx);

        nk_handle handle1;
        handle1.ptr=src->font_small_text_line;
        float len_pix;

        len_pix=nk_font_text_width(handle1, src->font_small_text_line->info.height, src->text, strlen(src->text));

        nk_layout_row_static(&src->all_context_nk->ctx, 0, floor(src->width), 1);
        if (nk_group_begin(&src->all_context_nk->ctx, "Group0", NK_WINDOW_NO_SCROLLBAR)) {
             nk_draw_text(commands, nk_rect(src->text_texture_x*src->width,src->text_texture_y*src->height,floor(len_pix),floor(src->height)),
                               src->text, strlen(src->text), &src->font_small_text_line->handle,
                               nk_rgba(src->bgColor[1]*255,src->bgColor[2]*255,src->bgColor[3]*255,src->bgColor[0]*255),
                               nk_rgba(src->textColor[1]*255,src->textColor[2]*255,src->textColor[3]*255,src->textColor[0]*255)
                               );
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

    gst_gl_shader_set_uniform_1f(src->shader, "height", floor(src->height));
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1f(src->shader, "text_x", src->text_texture_x*src->width);
    gst_gl_shader_set_uniform_1f(src->shader, "text_y", src->text_texture_y*src->height);
    gst_gl_shader_set_uniform_1f(src->shader, "text_lx", floor(src->width));
    gst_gl_shader_set_uniform_1f(src->shader, "text_ly", floor(src->height));
    gst_gl_shader_set_uniform_1i(src->shader, "draw_bg_flag", 0);
    gst_gl_shader_set_uniform_4f(src->shader, "text_color",src->textColor[1],src->textColor[2],src->textColor[3],src->textColor[0]);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->bgColor[1],src->bgColor[2],src->bgColor[3],src->bgColor[0]);

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

    //off our custom fbo. on gstreamer fbo.
    glBindFramebuffer(GL_FRAMEBUFFER, src->gstreamer_draw_fbo);

    //free big font texture:
    nk_font_atlas_clear(&src->all_context_nk->atlas);
    nk_free(&src->all_context_nk->ctx);

    glDeleteTextures(1, &src->all_context_nk->ogl.font_tex);

    src->gl_drawing_created=1;

    return TRUE;

}


gboolean gldraw_render(GstGLContext * context, GlDrawing *src)
{

    const GstGLFuncs *gl = context->gl_vtable;

    GLenum err;

    int i;

    if(src->font_full_filename[0]==0)return FALSE;

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
    positions[4]=1.0;
    positions[5]=1.0;
    positions[6]=1.0;
    positions[7]=1.0;
    positions[8]=src->width;
    positions[9]=0.0;
    positions[10]=1.0;
    positions[11]=0.0;
    positions[12]=1.0;
    positions[13]=1.0;
    positions[14]=1.0;
    positions[15]=1.0;
    positions[16]=src->width;
    positions[17]=src->height;
    positions[18]=1.0;
    positions[19]=1.0;
    positions[20]=1.0;
    positions[21]=1.0;
    positions[22]=1.0;
    positions[23]=1.0;
    positions[24]=0.0;
    positions[25]=src->height;
    positions[26]=0.0;
    positions[27]=1.0;
    positions[28]=1.0;
    positions[29]=1.0;
    positions[30]=1.0;
    positions[31]=1.0;

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

    glBindTexture(GL_TEXTURE_2D, src->text_texture);

    gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1f(src->shader, "text_x", src->text_texture_x*src->width);
    gst_gl_shader_set_uniform_1f(src->shader, "text_y", src->text_texture_y*src->height);
    gst_gl_shader_set_uniform_1f(src->shader, "text_lx", floor(src->width));
    gst_gl_shader_set_uniform_1f(src->shader, "text_ly", floor(src->height));
    gst_gl_shader_set_uniform_1i(src->shader, "draw_bg_flag", 0);
    gst_gl_shader_set_uniform_4f(src->shader, "text_color",src->textColor[1],src->textColor[2],src->textColor[3],src->textColor[0]);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->bgColor[1],src->bgColor[2],src->bgColor[3],src->bgColor[0]);

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

    return TRUE;

}

void gldraw_close (GstGLContext * context, GlDrawing *src)
{

  const GstGLFuncs *gl = context->gl_vtable;


  if(src->all_context_nk!=NULL){

      struct nk_sdl_device *dev = &src->all_context_nk->ogl;

      glDeleteBuffers(1, &src->all_context_nk->ogl.vbo);
      glDeleteBuffers(1, &src->all_context_nk->ogl.ebo);
      gl->DeleteVertexArrays(1, &src->all_context_nk->ogl.vao);
      nk_buffer_free(&src->all_context_nk->ogl.cmds);

      glDeleteFramebuffers(1,&src->text_fbo);
      glDeleteTextures(1,&src->text_texture);

      free(src->all_context_nk);
  }
  src->all_context_nk=NULL;

  if(src->shader!=NULL){
      gst_object_unref(src->shader);
      src->shader=NULL;
  }

  src->gl_drawing_created=0;

  src->custom_range[0]=32;
  src->custom_range[1]=32;
  src->custom_range[2]=0;

}


