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



#include <gst/gst.h>
#include "errorshandler.h"

#include <stdlib.h>
#include <math.h>

#include <stdio.h>

#include <time.h>


void errors_handler_clear(GlDrawing *gldrw_src, ErrorsHandler *src){

  gldraw_clear_errors(gldrw_src);

  src->ERBbegin=0;
  src->ERBlen=0;
  src->capture_time_end=0;
  src->SortedErrorsArr_size=0;
  src->errors_in_history_num=0;
  src->animation_move_down_flag=0;

}


void add_error_to_ERB(ErrorsHandler *src, InputErrorEx *inpError){

  int ERBbegin,ERBlen,ERBsize;

  ERBbegin=src->ERBbegin;
  ERBlen=src->ERBlen;
  ERBsize=ERRORS_RING_BUF_SIZE;


  int find;
  int i,k;
  int index,index2;
  InputErrorEx *iex;


  find=0;
  for(i=ERBbegin;i<ERBbegin+ERBlen;i++){

    index=i % ERBsize;
    iex=&src->ErrorsRingBuf[index];

    if(inpError->ie.source==iex->ie.source &&
       inpError->ie.type==iex->ie.type &&
       iex->ie.timestamp+iex->ie.delta_lasting > inpError->ie.timestamp){

       if(inpError->ie.timestamp>iex->ie.timestamp)
           iex->ie.delta_lasting=inpError->ie.delta_lasting+(inpError->ie.timestamp-iex->ie.timestamp);

           strcpy(iex->ie.msg,inpError->ie.msg);
           iex->ie.severity=inpError->ie.severity;

         find=1;
         break;

       }

  }

  if(find==0){
   ERBbegin--;
   if(ERBbegin<0){
     ERBbegin=ERBsize-1;
   }

   ERBlen++;
   if(ERBlen>ERBsize)ERBlen=ERBsize;

   src->global_id++;

   src->ErrorsRingBuf[ERBbegin]=*inpError;
   src->ErrorsRingBuf[ERBbegin].id=src->global_id;
   src->ERBbegin=ERBbegin;
   src->ERBlen=ERBlen;
   src->errors_added_num++;
   src->errors_added_num=CLAMP(src->errors_added_num,0,MAX(0,ERRORS_RING_BUF_SIZE));

  }

  switch(src->errorsSortingType){
    case ERRORS_SORTING_TYPE_TIME:
      src->animation_move_down_flag=sortErrors_byTime(src);
    break;
    case ERRORS_SORTING_TYPE_PRIORITY:
      src->animation_move_down_flag=sortErrors_byPriority(src);
    break;
    default:
    break;
  }

}

//id == 0   = invalid id
InputErrorEx *get_error_in_ERB_at_id(ErrorsHandler *src, __uint64_t id){

  int sz=src->ERBlen;
  int index,i;
  if(id==0)return NULL;
  for(i=src->ERBbegin;i<src->ERBbegin+src->ERBlen;i++){
    index=i % ERRORS_RING_BUF_SIZE;
    if(src->ErrorsRingBuf[index].id==id)return &src->ErrorsRingBuf[index];
  }
  return NULL;

}


int get_ERB_errors_count(ErrorsHandler *src){

  return src->ERBlen;

}

InputErrorEx *get_error_from_ERB(ErrorsHandler *src, int num){

  if(num<0 || num>=src->ERBlen)return NULL;
  int index;
  index=(src->ERBbegin+num) % ERRORS_RING_BUF_SIZE;
  return &src->ErrorsRingBuf[index];

}

void errors_handler_redraw_content(ErrorsHandler *src){
    src->redraw_content_flag=1;
}

void errors_handler_set_sorting(ErrorsHandler *src, int _sort){

    src->errorsSortingType=ERRORS_SORTING_TYPE_TIME;
    if(_sort==0)src->errorsSortingType=ERRORS_SORTING_TYPE_TIME;
    if(_sort==1)src->errorsSortingType=ERRORS_SORTING_TYPE_PRIORITY;
    src->sorting_changed_flag=1;

}

void errors_handler_first_init(ErrorsHandler *src){

  src->ERBbegin=0;
  src->ERBlen=0;
  memset(src->ErrorsRingBuf,0,ERRORS_RING_BUF_SIZE*sizeof(InputErrorEx));
  src->pipeline_clock=NULL;
  src->capture_time_delta=3000000000;
  src->capture_time_end=0;
  src->SortedErrorsArr_size=0;
  src->errors_in_history_num=0;
  src->errorsSortingType=ERRORS_SORTING_TYPE_TIME;
  memset(&src->past_first_error,0,sizeof(InputErrorEx));
  src->animation_move_down_flag=0;
  src->global_id=0;//0=not valid
  src->sorting_changed_flag=0;
  src->redraw_content_flag=0;

}

void errors_handler_set_pipeline_clock(ErrorsHandler *src, GstClock *_pipeline_clock){

  src->pipeline_clock=_pipeline_clock;

}

int is_error_continuous(ErrorsHandler *src, InputErrorEx *_ie, __uint64_t cur_time){

  if(_ie->ie.timestamp+_ie->ie.delta_lasting > cur_time)return 1;
  return 0;

}


//return 1 if run history animation must move down
//else return 0
int sortErrors_byTime(ErrorsHandler *src){

  InputErrorEx *ie;
  int i,sz;

  sz=get_ERB_errors_count(src);
  src->SortedErrorsArr_size=sz;
  for(i=0;i<sz;i++){
    ie=get_error_from_ERB(src,i);
    src->SortedErrorsArr[i]=ie;
  }

  if(src->errors_added_num>0)return 1;
  return 0;

}

//return 1 if run history animation must move down
//else return 0
int sortErrors_byPriority(ErrorsHandler *src){

  InputErrorEx *ie;
  int i,sz,sorted;

  sz=get_ERB_errors_count(src);
  src->SortedErrorsArr_size=sz;
  for(i=0;i<sz;i++){
    src->SortedErrorsArr[i]=get_error_from_ERB(src,i);
  }

  sorted=0;
  while(!sorted){
    sorted=1;
    for(i=0;i<sz-1;i++){
      if(src->SortedErrorsArr[i+1]->ie.severity > src->SortedErrorsArr[i]->ie.severity){
        ie=src->SortedErrorsArr[i+1];
        src->SortedErrorsArr[i+1]=src->SortedErrorsArr[i];
        src->SortedErrorsArr[i]=ie;
        sorted=0;
      }
    }
  }

  if(src->SortedErrorsArr_size>0){
    if(src->past_first_error.ie.source==src->SortedErrorsArr[0]->ie.source &&
       src->past_first_error.ie.type==src->SortedErrorsArr[0]->ie.type){
         src->past_first_error=*src->SortedErrorsArr[0];
         return 0;
    }
    src->past_first_error=*src->SortedErrorsArr[0];
  }
  return 1;

}

InputErrorEx *getFirstContinuousErrorInSortedList(ErrorsHandler *src, __uint64_t cur_time){


  InputErrorEx *iex;
  int i,sz;

  sz=src->SortedErrorsArr_size;

  for(i=0;i<sz;i++){
    iex=src->SortedErrorsArr[i];
    if(is_error_continuous(src, iex, cur_time)==1){
      return iex;
    }
  }

  return NULL;

}


void errors_handler_draw_callback(void *error_draw_callback_receiver, void *sender,
                                  int event_type, DisplayedErrorData **history, int history_size,
                                                             DisplayedErrorData *big_text_rect,
                                                             DisplayedErrorData *flash_rect){

  ErrorsHandler *src=(ErrorsHandler *)error_draw_callback_receiver;
  GlDrawing *gldr=(GlDrawing *)sender;

  int sz,i,index;
  InputErrorEx *iex;
  __uint64_t current_time;

  current_time=0;

  if(GST_IS_CLOCK(src->pipeline_clock)){
    current_time=gst_clock_get_time(src->pipeline_clock);
  }

  if(src->sorting_changed_flag==1){

    switch(src->errorsSortingType){
      case ERRORS_SORTING_TYPE_TIME:
        sortErrors_byTime(src);
      break;
      case ERRORS_SORTING_TYPE_PRIORITY:
        sortErrors_byPriority(src);
      break;
      default:
      break;
    }

    sz=MIN(src->errors_in_history_num,src->SortedErrorsArr_size);

    for(i=0;i<sz;i++){
      history[i]->id=src->SortedErrorsArr[i]->id;
      history[i]->flag_allow_rect_blink=0;
      history[i]->flag_show_msg=1;
      history[i]->flag_show_this=1;
      history[i]->flag_show_time=1;
      strcpy(history[i]->msg,src->SortedErrorsArr[i]->ie.msg);
      history[i]->severity=src->SortedErrorsArr[i]->ie.severity;
      history[i]->creation_time_d=src->SortedErrorsArr[i]->creation_time_d;
      history[i]->creation_time_h=src->SortedErrorsArr[i]->creation_time_h;
      history[i]->creation_time_m=src->SortedErrorsArr[i]->creation_time_m;
      history[i]->creation_time_s=src->SortedErrorsArr[i]->creation_time_s;
    }
    src->sorting_changed_flag=0;

   }

   if(src->redraw_content_flag==1){

    switch(src->errorsSortingType){
      case ERRORS_SORTING_TYPE_TIME:
        sortErrors_byTime(src);
      break;
      case ERRORS_SORTING_TYPE_PRIORITY:
        sortErrors_byPriority(src);
      break;
      default:
      break;
    }

    sz=MIN(history_size,src->SortedErrorsArr_size);
    for(i=0;i<sz;i++){
      history[i]->id=src->SortedErrorsArr[i]->id;
      history[i]->flag_allow_rect_blink=0;
      history[i]->flag_show_msg=1;
      history[i]->flag_show_this=1;
      history[i]->flag_show_time=1;
      strcpy(history[i]->msg,src->SortedErrorsArr[i]->ie.msg);
      history[i]->severity=src->SortedErrorsArr[i]->ie.severity;
      history[i]->creation_time_d=src->SortedErrorsArr[i]->creation_time_d;
      history[i]->creation_time_h=src->SortedErrorsArr[i]->creation_time_h;
      history[i]->creation_time_m=src->SortedErrorsArr[i]->creation_time_m;
      history[i]->creation_time_s=src->SortedErrorsArr[i]->creation_time_s;
    }

    src->redraw_content_flag=0;
    src->errors_added_num=0;
    src->errors_in_history_num=sz;
    if(src->errors_in_history_num>history_size)src->errors_in_history_num=history_size;
    return;

  }

  switch(src->errorsSortingType){
    case ERRORS_SORTING_TYPE_TIME:

      switch(event_type){
          case ERROR_DRAW_CALLBACK_EVENT_BEGIN_FRAME:

           if(src->SortedErrorsArr_size>0){
            if(current_time > src->capture_time_end){
              src->capture_time_end=current_time+src->capture_time_delta;

              iex=getFirstContinuousErrorInSortedList(src,current_time);
              if(is_error_continuous(src,src->SortedErrorsArr[0],current_time)==1)iex=src->SortedErrorsArr[0];
              if(iex==NULL)iex=src->SortedErrorsArr[0];

              big_text_rect->id=iex->id;
              big_text_rect->flag_allow_rect_blink=0;
              big_text_rect->flag_show_msg=1;
              big_text_rect->flag_show_this=1;
              big_text_rect->flag_show_time=1;
              strcpy(big_text_rect->msg,iex->ie.msg);
              big_text_rect->severity=iex->ie.severity;
              big_text_rect->creation_time_d=iex->creation_time_d;
              big_text_rect->creation_time_h=iex->creation_time_h;
              big_text_rect->creation_time_m=iex->creation_time_m;
              big_text_rect->creation_time_s=iex->creation_time_s;

              flash_rect->id=iex->id;
              flash_rect->flag_show_this=1;
              flash_rect->flag_allow_rect_blink=1;
              flash_rect->severity=iex->ie.severity;

            }
           }

          break;
          case ERROR_DRAW_CALLBACK_EVENT_LIST_MOVE_END:

            if(src->errors_added_num>0){

            src->errors_in_history_num++;
            if(src->errors_in_history_num>history_size)src->errors_in_history_num=history_size;


            for(i=src->errors_in_history_num-1;i>0;i--){
              *history[i]=*history[i-1];
            }

            index=src->errors_added_num-1;

            if(index<src->SortedErrorsArr_size){

              history[0]->id=src->SortedErrorsArr[index]->id;
              history[0]->flag_allow_rect_blink=0;
              history[0]->flag_show_msg=1;
              history[0]->flag_show_this=1;
              history[0]->flag_show_time=1;
              strcpy(history[0]->msg,src->SortedErrorsArr[index]->ie.msg);
              history[0]->severity=src->SortedErrorsArr[index]->ie.severity;
              history[0]->creation_time_d=src->SortedErrorsArr[index]->creation_time_d;
              history[0]->creation_time_h=src->SortedErrorsArr[index]->creation_time_h;
              history[0]->creation_time_m=src->SortedErrorsArr[index]->creation_time_m;
              history[0]->creation_time_s=src->SortedErrorsArr[index]->creation_time_s;

            }

            src->errors_added_num--;
            if(src->errors_added_num<0){
              src->errors_added_num=0;
            }

            gldraw_move_history_textlines_shift_y_only_one_frame(gldr);

           }

          break;
          default:
          break;
      }

    break;
    case ERRORS_SORTING_TYPE_PRIORITY:

      switch(event_type){
          case ERROR_DRAW_CALLBACK_EVENT_BEGIN_FRAME:

           if(src->SortedErrorsArr_size>0){
            if(current_time > src->capture_time_end){
              src->capture_time_end=current_time+src->capture_time_delta;

              iex=getFirstContinuousErrorInSortedList(src,current_time);
              if(is_error_continuous(src,src->SortedErrorsArr[0],current_time)==1)iex=src->SortedErrorsArr[0];
              if(iex==NULL)iex=src->SortedErrorsArr[0];

              big_text_rect->id=iex->id;
              big_text_rect->flag_allow_rect_blink=0;
              big_text_rect->flag_show_msg=1;
              big_text_rect->flag_show_this=1;
              big_text_rect->flag_show_time=1;
              strcpy(big_text_rect->msg,iex->ie.msg);
              big_text_rect->severity=iex->ie.severity;
              big_text_rect->creation_time_d=iex->creation_time_d;
              big_text_rect->creation_time_h=iex->creation_time_h;
              big_text_rect->creation_time_m=iex->creation_time_m;
              big_text_rect->creation_time_s=iex->creation_time_s;

              flash_rect->id=iex->id;
              flash_rect->flag_show_this=1;
              flash_rect->flag_allow_rect_blink=1;
              flash_rect->severity=iex->ie.severity;

            }
           }


          break;
          case ERROR_DRAW_CALLBACK_EVENT_LIST_MOVE_END:

            if(src->errors_added_num>0){

              src->errors_in_history_num=history_size;

              sz=MIN(src->errors_in_history_num,src->SortedErrorsArr_size);

              for(i=0;i<sz;i++){

                history[i]->id=src->SortedErrorsArr[i]->id;
                history[i]->flag_allow_rect_blink=0;
                history[i]->flag_show_msg=1;
                history[i]->flag_show_this=1;
                history[i]->flag_show_time=1;
                strcpy(history[i]->msg,src->SortedErrorsArr[i]->ie.msg);
                history[i]->severity=src->SortedErrorsArr[i]->ie.severity;
                history[i]->creation_time_d=src->SortedErrorsArr[i]->creation_time_d;
                history[i]->creation_time_h=src->SortedErrorsArr[i]->creation_time_h;
                history[i]->creation_time_m=src->SortedErrorsArr[i]->creation_time_m;
                history[i]->creation_time_s=src->SortedErrorsArr[i]->creation_time_s;

              }

              src->errors_added_num--;
              if(src->errors_added_num<0){
                src->errors_added_num=0;
              }

           }

          break;
          default:
          break;
      }

    break;
    default:
    break;
  }

  iex=get_error_in_ERB_at_id(src,big_text_rect->id);
  if(iex!=NULL){
    big_text_rect->timestamp=iex->ie.timestamp;
    big_text_rect->delta_lasting=iex->ie.delta_lasting;
    big_text_rect->flag_is_continuous=is_error_continuous(src,iex,current_time);
  }

  iex=get_error_in_ERB_at_id(src,flash_rect->id);
  if(iex!=NULL){
    flash_rect->timestamp=iex->ie.timestamp;
    flash_rect->delta_lasting=iex->ie.delta_lasting;
    flash_rect->flag_is_continuous=is_error_continuous(src,iex,current_time);
  }

  for(i=0;i<gldraw_get_history_errors_full_size(gldr);i++){
    iex=get_error_in_ERB_at_id(src,history[i]->id);
    if(iex!=NULL){
      history[i]->timestamp=iex->ie.timestamp;
      history[i]->delta_lasting=iex->ie.delta_lasting;
      history[i]->flag_is_continuous=is_error_continuous(src,iex,current_time);
    }
  }

}

void errors_handler_add_errors(ErrorsHandler *src, InputError *inpErrors, int inpErrorsNum){

  int i;

  InputError ie;
  int sorted=0;

  while(!sorted){
    sorted=1;
    for(i=0;i<inpErrorsNum-1;i++){
      if((inpErrors+i)->severity>(inpErrors+i+1)->severity){
         ie=*(inpErrors+i);
         *(inpErrors+i)=*(inpErrors+i+1);
         *(inpErrors+i+1)=ie;
         sorted=0;
      }
    }
  }


  time_t t=time(NULL);
  struct tm *tm1=localtime(&t);

  InputErrorEx iex;

  for(i=0;i<inpErrorsNum;i++){

    iex.ie=*(inpErrors+i);

    iex.creation_time_s=tm1->tm_sec;
    iex.creation_time_m=tm1->tm_min;
    iex.creation_time_h=tm1->tm_hour;
    iex.creation_time_d=tm1->tm_mday;

    add_error_to_ERB(src,&iex);

  }

}

