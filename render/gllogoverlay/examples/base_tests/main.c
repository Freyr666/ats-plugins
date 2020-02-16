/* USING
./testfilterapp test1
./testfilterapp test2
./testfilterapp test3

print font list:
./testfilterapp fonts
*/

#include <gst/gst.h>
#include <unistd.h>
#include <string.h>

#include <gst/audio/audio.h>

#include <glib.h>

#include <gst/app/gstappsrc.h>

#include <stdint.h>

#include <fontconfig/fontconfig.h>

#include <stdio.h>

//#include "../gllogoverlay/gllogoverlay/gstgllogoverlay.h"

#define arr_size 16



typedef struct {
 int severity;
 int source;
 int type;
 __uint64_t timestamp;
 __uint64_t delta_lasting;
 char msg[512];

}InputError;



void print_all_fonts(){

  FcConfig* config = FcInitLoadConfigAndFonts();
  FcPattern* pat = FcPatternCreate();
  FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
  FcFontSet* fs = FcFontList(config, pat, os);
  //printf("Total matching fonts: %d\n", fs->nfont);
  FcChar8 *file, *style, *family;
  file=NULL;//(FcChar8 *)src->font_caption;
  style=NULL;
  family=NULL;
  //int k;
  int i;

  char buf[1000];


  for (i=0; fs && i < fs->nfont; ++i) {
     FcPattern* font = fs->fonts[i];

     if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
         FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
         FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
     {

        printf("Filename: [%s] family: [%s] style: [%s]\n", file, family, style);
        //write(file1,buf,strlen(buf));


     }
  }
  if (fs) FcFontSetDestroy(fs);



}






  GThread *thread1;



  GMutex mutex_pool_releaseing;





  int thread1_loops=0;

  GstStateChangeReturn ret;
  GstElement *pipeline;
  GstElement *gltestsrc, *gltestsrc2, *glsoundbar, *glsoundbar2, *glimagesink,
             *gllogoverlay,
             *gllogoverlay2,
             *audiotestsrc, *audiotestsrc2, *alsasink, *wavescope, *wavescope2, *glvideomixer,
             *gltransformation, *gltransformation2,
             *filesrc, *qtdemux, *parser, *faad,
             *filesink,
             *wavparse2,
             *audioresample,
             *glupload,
             *valve,
             *valve2,
             *valve3,
             *valve4,
             *autoaudiosink,
             *autovideosink,

             *wavescope_arr[arr_size],
             *gltestsrc_arr[arr_size],
             *audiotestsrc_arr[arr_size],
             *glsoundbar_arr[arr_size],
             *gltransformation_arr[arr_size],

             *avenc_mp2_arr[arr_size],
             *avdec_mp2_arr[arr_size],
             *decodebin_arr[arr_size],
             *queue_arr[arr_size],
             *audioconvert_arr[arr_size],
             *filtercaps_audioconvert_arr[arr_size],
             //*filtercaps_audiotestsrc_arr[arr_size],

             *avenc_mp2,
             *avdec_mp2,
             *filtercaps_avenc_mp2,
             *decodebin,
             *queue1,
             *audioconvert,
             *filtercaps_audioconvert,

             *gldownload,
             *videoconvert,
             *vaapienc,
             *videomux_for_vaapi,
             *videotestsrc,

             *appsrc,
             *appsrc2

             ;

  GstPad *gllogoverlay_sink_pad1=NULL;
  GstPad *gllogoverlay_sink_pad2=NULL;

  int video_file_index=0;

  GstCaps    *filtercaps_caps_audiotestsrc_arr[arr_size];
  GstElement *filtercaps_audiotestsrc_arr[arr_size];

  GstCaps    *filtercaps_caps_glsoundbar_arr[arr_size];
  GstElement *filtercaps_glsoundbar_arr[arr_size];

  GstCaps    *filtercaps_caps_mix;
  GstElement *filtercaps_mix;


  //считает фактические перезапуски pipeline
  int global_reboots_counter=0;




  GstElement *filtercaps1=NULL, *filtercaps2=NULL, *filtercaps3=NULL, *filtercaps4=NULL, *filtercaps5=NULL;
  GstCaps *filtercaps_caps1, *filtercaps_caps2, *filtercaps_caps3, *filtercaps_caps4, *filtercaps_caps5;

  GMainLoop *loop;
  GstBus *bus;
  guint watch_id;
  GstElement* wavparse = NULL;
  GstCaps *convert1_caps = NULL;
  GstMessage *msg = NULL;

  GstPad *pad;

  int selector=-1;

  int end_flag=1;

  GstClock *pipeline_clock;


static void prepare_buffer(GstAppSrc* appsrc, InputError *input_errors, int input_errors_num) {

}

static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data) {

}



static void push_buf_to_gllogoverlay(InputError *input_errors, int input_errors_num) {


  static GstClockTime timestamp = 0;
  GstBuffer *buffer1,*buffer2;
  guint size;
  GstFlowReturn ret;

  size = sizeof(InputError)*input_errors_num;

  buffer1 = gst_buffer_new_wrapped_full( 0, (gpointer)input_errors, size, 0, size, NULL, NULL );
  GST_BUFFER_PTS (buffer1) = timestamp;
  GST_BUFFER_DURATION (buffer1) = gst_util_uint64_scale_int (1, GST_SECOND, 10);

  buffer2 = gst_buffer_new_wrapped_full( 0, (gpointer)input_errors, size, 0, size, NULL, NULL );
  GST_BUFFER_PTS (buffer2) = timestamp;
  GST_BUFFER_DURATION (buffer2) = gst_util_uint64_scale_int (1, GST_SECOND, 10);

  timestamp += GST_BUFFER_DURATION (buffer1);


  if(gllogoverlay_sink_pad1==NULL){
    gllogoverlay_sink_pad1=gst_element_get_static_pad(appsrc,"src");
  }

  if(gllogoverlay_sink_pad2==NULL){
    gllogoverlay_sink_pad2=gst_element_get_static_pad(appsrc2,"src");
  }


  ret=gst_pad_push(gllogoverlay_sink_pad1, buffer1);
  ret=gst_pad_push(gllogoverlay_sink_pad2, buffer2);


  if (ret != GST_FLOW_OK) {
    /* something wrong, stop pushing */
    // g_main_loop_quit (loop);
  }
}


//отдельный тред для изменения различных параметров тестируемого плагина во время работы (state playing)
void *thread_function1(void *data){

  int i;

  char buf[1000]={0};
  char str1[1000]={0};

  int err_sends_counter=1;

  InputError input_errors[20];


  int sizex,sizey;


  int reboot_num=0;

  sleep(1);

  pipeline_clock=NULL;

   //общий цикл, для выполнения перезагрузок (если они требуются)
  while(!end_flag){


    if(pipeline_clock==NULL){
      pipeline_clock=gst_element_get_clock(glvideomixer);
      //pipeline_clock=gst_element_get_clock(pipeline);
    }

    //sleep(1);
    g_mutex_lock(&mutex_pool_releaseing);

    if(selector==1){

       int test_selector=1;
       int test_counter=0;
       if(strcmp(((char *)data),"test1")==0 || strcmp(((char *)data),"")==0)test_selector=1;
       if(strcmp(((char *)data),"test2")==0)test_selector=2;
       if(strcmp(((char *)data),"test3")==0)test_selector=3;
       if(strcmp(((char *)data),"fonts")==0)test_selector=4;



       //test_selector=1;
       int sort_selector;

       sort_selector=0;

       g_object_set(gllogoverlay,"set-history-size",10,NULL);
       //g_object_set(gllogoverlay,"sort",1,NULL);

       //g_object_set(gllogoverlay,"text-color-argb",0xffff0000,NULL);
       //g_object_set(gllogoverlay,"bg-color-argb",0xff00ff00,NULL);

       if(test_selector==1){


         test_counter=0;
         while(!end_flag){

           if(pipeline_clock==NULL){
             pipeline_clock=gst_element_get_clock(glvideomixer);
             //pipeline_clock=gst_element_get_clock(pipeline);
           }

           usleep(450000);

           if((test_counter % 10) == 0){
             input_errors[0].severity=3;
             input_errors[0].source=3;
             input_errors[0].type=1;
             input_errors[0].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[0].delta_lasting=1000000000;
             strcpy(input_errors[0].msg,"3.Редкая ошибка.");

             push_buf_to_gllogoverlay(input_errors, 1);
           }







           if((test_counter % 40) == 10){

             thread1_loops++;
             sizex=600;
             sizey=700;
             sprintf(buf,
               "video/x-raw(memory:GLMemory),"
               "format=(string)RGBA,"
               "width=(int)%d,"
               "height=(int)%d,"
               "framerate=(fraction)25/1,"
               "texture-target=(string)2D",sizex,sizey);
             filtercaps_caps1 = gst_caps_from_string(buf);
             g_object_set (G_OBJECT (filtercaps1), "caps", filtercaps_caps1, NULL);
             gst_caps_unref (filtercaps_caps1);
             sprintf(str1,"sink_0");
             pad=gst_element_get_static_pad(glvideomixer,str1);
             g_object_set(pad,"xpos",0,NULL);
             g_object_set(pad,"ypos",0,NULL);
             g_object_set(pad,"width",sizex,NULL);
             g_object_set(pad,"height",sizey,NULL);

           }

           if((test_counter % 40) == 30){

             usleep(3000000);
             thread1_loops++;
             sizex=700;
             sizey=600;
             sprintf(buf,
               "video/x-raw(memory:GLMemory),"
               "format=(string)RGBA,"
               "width=(int)%d,"
               "height=(int)%d,"
               "framerate=(fraction)25/1,"
               "texture-target=(string)2D",sizex,sizey);
             filtercaps_caps1 = gst_caps_from_string(buf);
             g_object_set (G_OBJECT (filtercaps1), "caps", filtercaps_caps1, NULL);
             gst_caps_unref (filtercaps_caps1);
             sprintf(str1,"sink_0");
             pad=gst_element_get_static_pad(glvideomixer,str1);
             g_object_set(pad,"xpos",0,NULL);
             g_object_set(pad,"ypos",0,NULL);
             g_object_set(pad,"width",sizex,NULL);
             g_object_set(pad,"height",sizey,NULL);

           }

           input_errors[0].severity=1;
           input_errors[0].source=1;
           input_errors[0].type=1;
           input_errors[0].timestamp=gst_clock_get_time(pipeline_clock);
           input_errors[0].delta_lasting=1000000000;
           strcpy(input_errors[0].msg,"1.Ошибка 1. Длинная фраза с текстом для проверки обрезки.");

           //push_buf_to_gllogoverlay(&input_errors[1], 1);

           input_errors[1].severity=2;
           input_errors[1].source=2;
           input_errors[1].type=1;
           input_errors[1].timestamp=gst_clock_get_time(pipeline_clock);
           input_errors[1].delta_lasting=1000000000;
           strcpy(input_errors[1].msg,"2.Ошибка 2. Длинная фраза с текстом для проверки обрезки.");

           //push_buf_to_gllogoverlay(&input_errors[2], 1);

           push_buf_to_gllogoverlay(input_errors, 2);

           //sorts switch test:
           if((test_counter % 30) == 15){
             //g_object_set(gllogoverlay,"set-history-size",(sort_selector+1)*5,NULL);
             //g_object_set(gllogoverlay,"clear",0,NULL);
             //g_object_set(gllogoverlay,"sort",1-sort_selector,NULL);
             sort_selector++;
             if(sort_selector>1)sort_selector=0;

           }


           test_counter++;




           err_sends_counter++;
           if(err_sends_counter>8)err_sends_counter=0;
         }


      }




     if(test_selector==2){

         test_counter=0;
         while(!end_flag){
           usleep(500000);

           if(pipeline_clock==NULL){
             pipeline_clock=gst_element_get_clock(glvideomixer);
             //pipeline_clock=gst_element_get_clock(pipeline);
           }

           if((test_counter % 10) == 0){
             input_errors[0].severity=1;
             input_errors[0].source=1;
             input_errors[0].type=1;
             input_errors[0].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[0].delta_lasting=1000000000;
             strcpy(input_errors[0].msg,"1.Ошибка 1. Длинная фраза с текстом для проверки обрезки.");

             input_errors[1].severity=2;
             input_errors[1].source=2;
             input_errors[1].type=1;
             input_errors[1].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[1].delta_lasting=1000000000;
             strcpy(input_errors[1].msg,"2.Ошибка 2. Длинная фраза с текстом для проверки обрезки.");

             input_errors[2].severity=3;
             input_errors[2].source=3;
             input_errors[2].type=1;
             input_errors[2].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[2].delta_lasting=1000000000;
             strcpy(input_errors[2].msg,"3.Редкая ошибка.");

             input_errors[3].severity=4;
             input_errors[3].source=4;
             input_errors[3].type=1;
             input_errors[3].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[3].delta_lasting=1000000000;
             strcpy(input_errors[3].msg,"4.Редкая ошибка.");


             push_buf_to_gllogoverlay(input_errors, 4);
           }

/*
           //sorts switch test:
           if((test_counter % 20) == 10){
             //g_object_set(gllogoverlay,"set-history-size",3,NULL);
             g_object_set(gllogoverlay,"clear",0,NULL);
             //g_object_set(gllogoverlay,"sort",sort_selector,NULL);
             sort_selector++;
             if(sort_selector>1)sort_selector=0;

           }
*/

             input_errors[0].severity=1;
             input_errors[0].source=1;
             input_errors[0].type=1;
             input_errors[0].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[0].delta_lasting=1000000000;
             strcpy(input_errors[0].msg,"1.Ошибка 1. Длинная фраза с текстом для проверки обрезки.");

             //push_buf_to_gllogoverlay(&input_errors[1], 1);

             input_errors[1].severity=2;
             input_errors[1].source=2;
             input_errors[1].type=1;
             input_errors[1].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[1].delta_lasting=1000000000;
             strcpy(input_errors[1].msg,"2.Ошибка 2. Длинная фраза с текстом для проверки обрезки.");


             //push_buf_to_gllogoverlay(&input_errors[2], 1);

             push_buf_to_gllogoverlay(input_errors, 2);


             test_counter++;



           err_sends_counter++;
           if(err_sends_counter>8)err_sends_counter=0;
         }

      }




      if(test_selector==3){

         test_counter=0;
         while(!end_flag){
           usleep(500000);

           if(pipeline_clock==NULL){
             pipeline_clock=gst_element_get_clock(glvideomixer);
             //pipeline_clock=gst_element_get_clock(pipeline);
           }

           if((test_counter % 20) < 12){
             input_errors[0].severity=1;
             input_errors[0].source=1;
             input_errors[0].type=1;
             input_errors[0].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[0].delta_lasting=1000000000;
             strcpy(input_errors[0].msg,"1.Ошибка 1. Длинная фраза с текстом для проверки обрезки.");

             input_errors[1].severity=2;
             input_errors[1].source=2;
             input_errors[1].type=1;
             input_errors[1].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[1].delta_lasting=1000000000;
             strcpy(input_errors[1].msg,"2.Ошибка 2. Длинная фраза с текстом для проверки обрезки.");

             input_errors[2].severity=3;
             input_errors[2].source=3;
             input_errors[2].type=1;
             input_errors[2].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[2].delta_lasting=1000000000;
             strcpy(input_errors[2].msg,"3.Приоритет.");

             push_buf_to_gllogoverlay(input_errors, 3);
           }

           /*
           //sorts switch test:
           if((test_counter % 10) == 8){
             //g_object_set(gllogoverlay,"set-history-size",3,NULL);
             //g_object_set(gllogoverlay,"clear",0,NULL);
             g_object_set(gllogoverlay,"sort",sort_selector,NULL);
             sort_selector++;
             if(sort_selector>1)sort_selector=0;

           }
           */

           /*
             input_errors[0].severity=1;
             input_errors[0].source=1;
             input_errors[0].type=1;
             input_errors[0].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[0].delta_lasting=1000000000;
             strcpy(input_errors[0].msg,"1.Ошибка 1. Длинная фраза с текстом для проверки обрезки.");

             //push_buf_to_gllogoverlay(&input_errors[1], 1);

             input_errors[1].severity=2;
             input_errors[1].source=2;
             input_errors[1].type=1;
             input_errors[1].timestamp=gst_clock_get_time(pipeline_clock);
             input_errors[1].delta_lasting=1000000000;
             strcpy(input_errors[1].msg,"2.Ошибка 2. Длинная фраза с текстом для проверки обрезки.");


             //push_buf_to_gllogoverlay(&input_errors[2], 1);

             push_buf_to_gllogoverlay(input_errors, 2);
           */


             test_counter++;



           err_sends_counter++;
           if(err_sends_counter>8)err_sends_counter=0;
         }

      }


      if(test_selector==4){
        print_all_fonts();
        while(!end_flag){
          usleep(500000);
        }
      }



      g_object_set (G_OBJECT (appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
        "is-live", FALSE,
        NULL);




    }//selector==1



    thread1_loops++;

    g_mutex_unlock(&mutex_pool_releaseing);

    //Если используется перезагрузка приложения (разкомментировать):
    if(reboot_num>1){
        reboot_num=0;
/*
        gst_element_post_message(pipeline, gst_message_new_eos(GST_OBJECT(pipeline)));
        gst_element_set_state (pipeline, GST_STATE_NULL);
        g_main_loop_quit (loop);
*/
    }

    reboot_num++;

  }



  return NULL;


}







static gboolean
bus_call (GstBus     *bus,
	  GstMessage *msg,
	  gpointer    data)
{


  GMainLoop *loop = (GMainLoop *)data;



  switch (GST_MESSAGE_TYPE (msg)) {
    /*case GST_MESSAGE_EOS:
      g_print ("End-of-stream\n");
      gst_element_set_state (pipeline, GST_STATE_NULL);
      g_main_loop_quit (loop);
      break;*/
    case GST_MESSAGE_ERROR: {
      gchar *debug = NULL;
      GError *err = NULL;

      gst_message_parse_error (msg, &err, &debug);

      g_print ("Error: %s\n", err->message);
      g_error_free (err);

      if (debug) {
        g_print ("Debug details: %s\n", debug);
        g_free (debug);
      }

      gst_element_set_state (pipeline, GST_STATE_NULL);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_ELEMENT: {
/*
       const GstStructure *s = gst_message_get_structure (msg);
       const gchar *name = gst_structure_get_name (s);

       std::string nameStr(name);

       if(nameStr.compare("loud2")==0){
         //const GValue *debug_msg_couter=gst_structure_get_value(s,"debug_msg_couter");
         //const GValue *calc_time=gst_structure_get_value(s,"calc_time");
         //const GValue *LKFS_momentary_lkfs=gst_structure_get_value(s,"LKFS_momentary_lkfs");
         //const GValue *LKFS_short_term_lkfs=gst_structure_get_value(s,"LKFS_short_term_lkfs");




          const GValue *LKFS_momentary_flagLoudDown=gst_structure_get_value(s,"LKFS_momentary_flagLoudDown");
          const GValue *LKFS_momentary_flagLoudDownWarning=gst_structure_get_value(s,"LKFS_momentary_flagLoudDownWarning");
          const GValue *LKFS_momentary_flagLoudNorm=gst_structure_get_value(s,"LKFS_momentary_flagLoudNorm");
          const GValue *LKFS_momentary_flagLoudUp=gst_structure_get_value(s,"LKFS_momentary_flagLoudUp");
          const GValue *LKFS_momentary_flagLoudUpWarning=gst_structure_get_value(s,"LKFS_momentary_flagLoudUpWarning");
          const GValue *LKFS_momentary_readyFlag=gst_structure_get_value(s,"LKFS_momentary_readyFlag");
          const GValue *LKFS_momentary_lkfs=gst_structure_get_value(s,"LKFS_momentary_lkfs");

          const GValue *LKFS_short_term_flagLoudDown=gst_structure_get_value(s,"LKFS_short_term_flagLoudDown");
          const GValue *LKFS_short_term_flagLoudDownWarning=gst_structure_get_value(s,"LKFS_short_term_flagLoudDownWarning");
          const GValue *LKFS_short_term_flagLoudNorm=gst_structure_get_value(s,"LKFS_short_term_flagLoudNorm");
          const GValue *LKFS_short_term_flagLoudUp=gst_structure_get_value(s,"LKFS_short_term_flagLoudUp");
          const GValue *LKFS_short_term_flagLoudUpWarning=gst_structure_get_value(s,"LKFS_short_term_flagLoudUpWarning");
          const GValue *LKFS_short_term_readyFlag=gst_structure_get_value(s,"LKFS_short_term_readyFlag");
          const GValue *LKFS_short_term_lkfs=gst_structure_get_value(s,"LKFS_short_term_lkfs");

          g_print ("----lkfs_m=%f, r=%d, LD=%d, LDW=%d, LN=%d, LUW=%d, LU=%d\n",
                   LKFS_momentary_lkfs->data->v_float,
                   LKFS_momentary_readyFlag->data->v_int,
                   LKFS_momentary_flagLoudDown->data->v_int,
                   LKFS_momentary_flagLoudDownWarning->data->v_int,
                   LKFS_momentary_flagLoudNorm->data->v_int,
                   LKFS_momentary_flagLoudUpWarning->data->v_int,
                   LKFS_momentary_flagLoudUp->data->v_int);
          g_print ("    lkfs_s=%f, r=%d, LD=%d, LDW=%d, LN=%d, LUW=%d, LU=%d\n",
                   LKFS_short_term_lkfs->data->v_float,
                   LKFS_short_term_readyFlag->data->v_int,
                   LKFS_short_term_flagLoudDown->data->v_int,
                   LKFS_short_term_flagLoudDownWarning->data->v_int,
                   LKFS_short_term_flagLoudNorm->data->v_int,
                   LKFS_short_term_flagLoudUpWarning->data->v_int,
                   LKFS_short_term_flagLoudUp->data->v_int);

           //g_print ("debug_couter=%d, calc_time=%lf, lkfs_m=%f, lkfs_s=%f \n",
           //      debug_msg_couter->data->v_int,
           //      calc_time->data->v_double,
           //      LKFS_momentary_lkfs->data->v_float,
           //      LKFS_short_term_lkfs->data->v_float );

       }

       //if(nameStr.compare("loud2_debug_msg1")==0){

       //  g_print ("loud2_debug_msg1\n");
       //  g_main_loop_quit (loop);

       //}
*/
      break;


    }
    default:
      break;
  }



  return TRUE;
}





gint pipeline_test (gint argc, gchar *argv[])
{



  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new ("my_pipeline");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
 /* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                  (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if(msg!=NULL)gst_message_unref(msg);
  msg=NULL;*/

  watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);


  selector = 1;


  int i;
  gchar str1[100];


  switch (selector) {


    case 1:
    {

      int w,h;
      char buf1[1000];

      w=1424;
      h=868;

      gltestsrc =  gst_element_factory_make("gltestsrc", "_gltestsrc_");

      appsrc = gst_element_factory_make("appsrc", "_appsrc_");
      appsrc2 = gst_element_factory_make("appsrc", "_appsrc2_");

      gllogoverlay = gst_element_factory_make("gllogoverlay", "_gllogoverlay_");
      gllogoverlay2 = gst_element_factory_make("gllogoverlay", "_gllogoverlay2_");

      glvideomixer = gst_element_factory_make ("glvideomixer", "_glvideomixer_");
      glimagesink  = gst_element_factory_make ("glimagesink", "_glimagesink_");


      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",w/2,h/2);
      filtercaps1 = gst_element_factory_make("capsfilter","_filtercaps1_");
      filtercaps_caps1= gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps1), "caps", filtercaps_caps1, NULL);
      gst_caps_unref (filtercaps_caps1);

      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",w/2,h/2);
      filtercaps2 = gst_element_factory_make("capsfilter","_filtercaps2_");
      filtercaps_caps2 = gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps2), "caps", filtercaps_caps2, NULL);
      gst_caps_unref (filtercaps_caps2);

      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",w/2,h/2);
      filtercaps3 = gst_element_factory_make("capsfilter","_filtercaps3_");
      filtercaps_caps3 = gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps3), "caps", filtercaps_caps3, NULL);
      gst_caps_unref (filtercaps_caps3);


      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",w,h);
      filtercaps5 = gst_element_factory_make("capsfilter","_filtercaps5_");
      filtercaps_caps5 = gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps5), "caps", filtercaps_caps5, NULL);
      gst_caps_unref (filtercaps_caps5);








      if (!gllogoverlay
          || !glvideomixer
          || !glimagesink
          ) {
        g_print ("Error init\n");
        return -1;
      }

      g_object_set (G_OBJECT (appsrc), "caps",
  		gst_caps_new_simple ("application/x-bin-error-log",NULL),
  	  NULL);

      gst_bin_add_many (GST_BIN (pipeline),

        appsrc,
        gllogoverlay,
        filtercaps1,

        glvideomixer,
        filtercaps5,
        glimagesink,

        gltestsrc,
        filtercaps3,

        appsrc2,
        gllogoverlay2,
        filtercaps2,

        NULL);

      gst_element_link_many(
        appsrc,
        gllogoverlay,
        filtercaps1,
        glvideomixer,
        NULL);

      gst_element_link_many(
        gltestsrc,
        filtercaps3,
        glvideomixer,
        NULL);

      gst_element_link_many(
        appsrc2,
        gllogoverlay2,
        filtercaps2,
        glvideomixer,
        NULL);



      gst_element_link_many(glvideomixer, filtercaps5, glimagesink);

      sprintf(str1,"sink_0");
      pad=gst_element_get_static_pad(glvideomixer,str1);
      g_object_set(pad,"xpos",0,NULL);
      g_object_set(pad,"ypos",0,NULL);
      g_object_set(pad,"width",w/2,NULL);
      g_object_set(pad,"height",h/2,NULL);

      sprintf(str1,"sink_1");
      pad=gst_element_get_static_pad(glvideomixer,str1);
      g_object_set(pad,"xpos",w/2,NULL);
      g_object_set(pad,"ypos",0,NULL);
      g_object_set(pad,"width",w/2,NULL);
      g_object_set(pad,"height",h/2,NULL);


      sprintf(str1,"sink_2");
      pad=gst_element_get_static_pad(glvideomixer,str1);
      g_object_set(pad,"xpos",w/2,NULL);
      g_object_set(pad,"ypos",0,NULL);
      g_object_set(pad,"width",w/2,NULL);
      g_object_set(pad,"height",h/2,NULL);


      //g_object_set(gllogoverlay,"set-errors",0x07,NULL);

      g_object_set(gllogoverlay,"font-caption","Noto Serif Display",NULL);
      //g_object_set(gllogoverlay,"font-style","Italic",NULL);
      //g_object_set(gllogoverlay,"font-style","Regular",NULL);
      g_object_set(gllogoverlay,"font-style","Bold",NULL);
      //g_object_set(gllogoverlay,"set-history-size",5,NULL);
      //g_object_set(gllogoverlay,"sort",0,NULL);

      g_object_set (G_OBJECT (appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
        "is-live", FALSE,
        NULL);

      g_object_set (G_OBJECT (appsrc2),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
        "is-live", FALSE,
        NULL);


      //gllogoverlay_sink_pad=gst_element_get_static_pad(gllogoverlay,"sink");

      //g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);

      break;
    }


    default:
      break;
  }



  /* run */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);

/*
  sleep(5);
  ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
  sleep(5);
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  sleep(5);
  ret = gst_element_set_state (pipeline, GST_STATE_READY);
  sleep(5);
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  sleep(1);
  ret = gst_element_set_state (pipeline, GST_STATE_NULL);
*/



  g_mutex_unlock(&mutex_pool_releaseing);

  g_main_loop_run (loop);
  global_reboots_counter++;
  end_flag=1;

  g_mutex_lock(&mutex_pool_releaseing);


  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);



  gst_object_unref (pipeline);
  g_source_remove (watch_id);
  g_main_loop_unref (loop);


}


gint
main (gint   argc,
      gchar *argv[])
{


  setenv("GST_DEBUG","3",FALSE);

  //setenv("MESA_GL_VERSION_OVERRIDE",3.0,FALSE);


  setenv("GST_GL_PLATFORM","egl",FALSE);

  setenv("GST_GL_WINDOW","x11",FALSE);
  //setenv("GST_GL_WINDOW","wayland",FALSE);

  //setenv("GST_GL_API","opengl",FALSE);
  //setenv("GST_GL_API","gles2",FALSE);
  setenv("GST_GL_API","opengl3",FALSE);




  g_mutex_init (&mutex_pool_releaseing);

  g_mutex_lock(&mutex_pool_releaseing);


  char buf[1000];

  if(argc==2){
    strcpy(buf,argv[1]);
  }

  end_flag=0;

  //Отдельный тред для изменений параметров тестируемого плагина
  thread1=g_thread_create(thread_function1,buf,TRUE,NULL);

  //Цикл для бесконечной перезагрузки приложения
  //while(1){

    pipeline_test(argc,argv);

  //}

  g_mutex_unlock(&mutex_pool_releaseing);



  if (mutex_pool_releaseing.p) {
    g_mutex_clear (&mutex_pool_releaseing);
    mutex_pool_releaseing.p = NULL;
  }




  return 0;
}




