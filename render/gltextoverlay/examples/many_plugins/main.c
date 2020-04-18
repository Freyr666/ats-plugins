/* USING
 
 Test draws many plugin instances. Restart plugins after 10 seconds.
 
./testfilterapp

change:
  #define gldisplayerrors_arr_num 64    = plugins num
  #define gldisplayerrors_rows_num 8    = plugins num in one row

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

//#include "../gldisplayerrors/gldisplayerrors/gstgldisplayerrors.h"





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




  #define gldisplayerrors_arr_num 64
  #define gldisplayerrors_rows_num 8


  int thread1_loops=0;

  GstStateChangeReturn ret;
  GstElement *pipeline;
  GstElement *gltestsrc, *gltestsrc2, *glsoundbar, *glsoundbar2, *glimagesink,

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

             *wavescope_arr[gldisplayerrors_arr_num],
             *gltestsrc_arr[gldisplayerrors_arr_num],
             *audiotestsrc_arr[gldisplayerrors_arr_num],
             *glsoundbar_arr[gldisplayerrors_arr_num],
             *gltransformation_arr[gldisplayerrors_arr_num],

             *avenc_mp2_arr[gldisplayerrors_arr_num],
             *avdec_mp2_arr[gldisplayerrors_arr_num],
             *decodebin_arr[gldisplayerrors_arr_num],
             *queue_arr[gldisplayerrors_arr_num],
             *audioconvert_arr[gldisplayerrors_arr_num],
             *filtercaps_audioconvert_arr[gldisplayerrors_arr_num],
             //*filtercaps_audiotestsrc_arr[gldisplayerrors_arr_num],

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
             *videotestsrc


             ;

  GstPad *gldisplayerrors_sink_pad1=NULL;
  GstPad *gldisplayerrors_sink_pad2=NULL;

  int video_file_index=0;


  GstElement *gltextoverlay[gldisplayerrors_arr_num];

  GstCaps    *filtercaps_gldisplayerrors_caps_arr[gldisplayerrors_arr_num];
  GstElement *filtercaps_gldisplayerrors_arr[gldisplayerrors_arr_num];

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

  //int ready=0;





//отдельный тред для изменения различных параметров тестируемого плагина во время работы (state playing)
void *thread_function1(void *data){

  int i;

  char buf[1000]={0};
  char str1[1000]={0};

  int err_sends_counter=1;

  InputError input_errors[20];


  int sizex,sizey;


  int reboot_num=0;
  thread1_loops=0;

  sleep(2);

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




       //test_selector=1;
       int sort_selector;

       sort_selector=0;

       //g_object_set(gldisplayerrors,"set-history-size",10,NULL);
       //g_object_set(gldisplayerrors,"sort",1,NULL);

       //g_object_set(gldisplayerrors,"text-color-argb",0xffff0000,NULL);
       //g_object_set(gldisplayerrors,"bg-color-argb",0xff00ff00,NULL);

       if(test_selector==1){


         test_counter=0;
         while(!end_flag){

           if(pipeline_clock==NULL){
             pipeline_clock=gst_element_get_clock(glvideomixer);
             //pipeline_clock=gst_element_get_clock(pipeline);
           }

           usleep(450000);


           test_counter++;


           err_sends_counter++;
           if(err_sends_counter>8)err_sends_counter=0;


           thread1_loops++;


           //Если используется перезагрузка приложения (разкомментировать):
           if(thread1_loops>20){
             reboot_num=0;
             end_flag=1;
             gst_element_post_message(pipeline, gst_message_new_eos(GST_OBJECT(pipeline)));
             gst_element_set_state (pipeline, GST_STATE_NULL);
             g_main_loop_quit (loop);

           }

           reboot_num++;





         }//while(!end_flag)


      }//if(test_selector==1)





/*
      g_object_set (G_OBJECT (appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
        "is-live", FALSE,
        NULL);
*/



    }//selector==1




    //Если используется перезагрузка приложения (разкомментировать):
    if(reboot_num>1){
        reboot_num=0;
/*
        gst_element_post_message(pipeline, gst_message_new_eos(GST_OBJECT(pipeline)));
        gst_element_set_state (pipeline, GST_STATE_NULL);
        g_main_loop_quit (loop);
*/
    }

    g_mutex_unlock(&mutex_pool_releaseing);
    reboot_num++;

  }//while




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
  gchar str1[1000];


  switch (selector) {


    case 1:
    {

/*

      //g_object_set(gldisplayerrors,"set-errors",0x07,NULL);

      g_object_set(gldisplayerrors,"font-caption","Noto Serif Display",NULL);
      //g_object_set(gldisplayerrors,"font-style","Italic",NULL);
      //g_object_set(gldisplayerrors,"font-style","Regular",NULL);
      g_object_set(gldisplayerrors,"font-style","Bold",NULL);
      //g_object_set(gldisplayerrors,"set-history-size",5,NULL);
      //g_object_set(gldisplayerrors,"sort",0,NULL);

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


      //gldisplayerrors_sink_pad=gst_element_get_static_pad(gldisplayerrors,"sink");

      //g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);
*/








/*
  GstElement *gldisplayerrors[gldisplayerrors_arr_num],
             *appsrc[gldisplayerrors_arr_num],

  GstCaps    *filtercaps_gldisplayerrors_caps_arr[gldisplayerrors_arr_num];
  GstElement *filtercaps_gldisplayerrors_arr[gldisplayerrors_arr_num];

  GstCaps    *filtercaps_caps_mix;
  GstElement *filtercaps_mix;
*/

     int w,h,wm,hm;
     char buf1[1000];

     w=1424;
     h=868;
     wm=w/(gldisplayerrors_arr_num/gldisplayerrors_rows_num);
     hm=h/gldisplayerrors_rows_num/8;


     for(i=0;i<gldisplayerrors_arr_num;i++){

      sprintf(str1,"_gltextoverlay_%d",i);
      gltextoverlay[i]=gst_element_factory_make ("gltextoverlay", str1);

      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",wm,hm);
      sprintf(str1,"_filtercaps_gldisplayerrors_arr_%d",i);
      filtercaps_gldisplayerrors_arr[i] = gst_element_factory_make("capsfilter",str1);
      filtercaps_gldisplayerrors_caps_arr[i] = gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps_gldisplayerrors_arr[i]), "caps", filtercaps_gldisplayerrors_caps_arr[i], NULL);
      gst_caps_unref (filtercaps_gldisplayerrors_caps_arr[i]);

     }

      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",w,h);
      filtercaps_mix = gst_element_factory_make("capsfilter","_filtercaps_mix_");
      filtercaps_caps_mix = gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps_mix), "caps", filtercaps_caps_mix, NULL);
      gst_caps_unref (filtercaps_caps_mix);

      //gltestsrc = gst_element_factory_make ("gltestsrc", "_gltestsrc_");

      glvideomixer = gst_element_factory_make ("glvideomixer", "_glvideomixer_");
      glimagesink  = gst_element_factory_make ("glimagesink", "_glimagesink_");

      for(i=0;i<gldisplayerrors_arr_num;i++){
        if(!gltextoverlay[i] ||
           !glvideomixer || !glimagesink){
          g_print ("Error init\n");
          return -1;
        }
      }

      for(i=0;i<gldisplayerrors_arr_num;i++){
        gst_bin_add(GST_BIN (pipeline),gltextoverlay[i]);
        gst_bin_add(GST_BIN (pipeline),filtercaps_gldisplayerrors_arr[i]);
      }

      //gst_bin_add(GST_BIN (pipeline),gltestsrc);

      gst_bin_add(GST_BIN (pipeline),filtercaps_mix);
      gst_bin_add(GST_BIN (pipeline),glvideomixer);
      gst_bin_add(GST_BIN (pipeline),glimagesink);


      //gst_element_link_many(gltestsrc,glvideomixer,NULL);

      for(i=0;i<gldisplayerrors_arr_num;i++){
        gst_element_link_many(gltextoverlay[i],filtercaps_gldisplayerrors_arr[i],glvideomixer,NULL);
      }

      gst_element_link_many(glvideomixer, filtercaps_mix, glimagesink);

      //gldisplayerrors_arr_num/gldisplayerrors_rows_num
      for(i=0;i<gldisplayerrors_arr_num;i++){
        sprintf(str1,"sink_%d", i);
        pad=gst_element_get_static_pad(glvideomixer,str1);
        g_object_set(pad,"xpos", (i % (gldisplayerrors_arr_num / gldisplayerrors_rows_num)) * wm,NULL);
        g_object_set(pad,"ypos",( gldisplayerrors_rows_num * i / gldisplayerrors_arr_num) * hm, NULL);
        g_object_set(pad,"width",wm,NULL);
        g_object_set(pad,"height",hm,NULL);
        gst_object_unref(pad);

        sprintf(str1,"text%d Текст Длинный №%d",i,i);
        //sprintf(str1,"1234567890abcdefghijklmn");
        g_object_set(gltextoverlay[i],"text",str1,NULL);

      }





      break;

    }


    default:
      break;
  }



  /* run */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);


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



  //Отдельный тред для изменений параметров тестируемого плагина
  //end_flag=0;
  //thread1=g_thread_create(thread_function1,buf,TRUE,NULL);
  //pipeline_test(argc,argv);

  //Цикл для бесконечной перезагрузки приложения
  while(1){
    end_flag=0;
    thread1=g_thread_create(thread_function1,buf,TRUE,NULL);
    pipeline_test(argc,argv);
    sleep(2);
  }



  g_mutex_unlock(&mutex_pool_releaseing);

  if (mutex_pool_releaseing.p) {
    g_mutex_clear (&mutex_pool_releaseing);
    mutex_pool_releaseing.p = NULL;
  }

  return 0;

}




