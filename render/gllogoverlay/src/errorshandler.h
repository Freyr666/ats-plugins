/*
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

#ifndef __ERRORSHANDLER_H__
#define __ERRORSHANDLER_H__

#include <stdio.h>
#include "gldrawing.h"


typedef struct {
 int severity;
 int source;
 int type;
 __uint64_t timestamp;
 __uint64_t delta_lasting;
 char msg[512];

}InputError;


typedef struct {
 InputError ie;
 __uint64_t id;
 int creation_time_s;
 int creation_time_m;
 int creation_time_h;
 int creation_time_d;

}InputErrorEx;


enum {

 ERRORS_SORTING_TYPE_TIME=0,
 ERRORS_SORTING_TYPE_PRIORITY

};

#define ERRORS_RING_BUF_SIZE 15

typedef struct{

  __uint64_t global_id;

  GstClock *pipeline_clock;

  int errors_added_num;

  //for big_text
  __uint64_t capture_time_delta;
  __uint64_t capture_time_end;

  int sorting_changed_flag;
  int redraw_content_flag;

  InputErrorEx past_first_error;
  int animation_move_down_flag;

  //1. Все входные ошибки
  int ERBbegin;
  int ERBlen;
  InputErrorEx ErrorsRingBuf[ERRORS_RING_BUF_SIZE];
  //2. Массив из входных ошибок - убраны повторы (для продолжительных ошибок).
  //   Повторы склеены:
  int SortedErrorsArr_size;
  InputErrorEx *SortedErrorsArr[ERRORS_RING_BUF_SIZE];

  int errorsSortingType;

  int errors_in_history_num;

}ErrorsHandler;


void errors_handler_redraw_content(ErrorsHandler *src);
void errors_handler_set_sorting(ErrorsHandler *src, int _sort);
void errors_handler_clear(GlDrawing *gldrw_src, ErrorsHandler *src);
void errors_handler_set_pipeline_clock(ErrorsHandler *src, GstClock *_pipeline_clock);
void errors_handler_draw_callback(void *error_draw_callback_receiver, void *sender,
                                  int event_type, DisplayedErrorData **history, int history_size,
                                                             DisplayedErrorData *big_text_rect,
                                                             DisplayedErrorData *flash_rect);
void errors_handler_first_init(ErrorsHandler *src);
void errors_handler_add_errors(ErrorsHandler *src, InputError *inpErrors, int inpErrorsNum);

#endif //__ERRORSHANDLER_H__


