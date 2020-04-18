
#include <gst/gst.h>
#include <unistd.h>
#include <string.h>

#include <gst/audio/audio.h>

#include <glib.h>

#include <gst/app/gstappsrc.h>

#include <stdint.h>


#define arr_size 16

/*
USING:

./testfilterapp test1

*/


  GThread *thread1;

  GMutex mutex_pool_releaseing;

  int thread1_loops=0;

  GstStateChangeReturn ret;
  GstElement *pipeline;
  GstElement *gltestsrc, *gltestsrc2, *glsoundbar, *glsoundbar2, *glimagesink,
             *gltextoverlay,
             *gltextoverlay2,
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

             *appsrc

             ;

  GstPad *gltextoverlay_sink_pad=NULL;

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



//отдельный тред для изменения различных параметров тестируемого плагина во время работы (state playing)
void *thread_function1(void *data){

  int i;

  char buf[1000]={0};
  char str1[1000]={0};

  int err_sends_counter=1;

  int reboot_num=0;

  sleep(1);

  pipeline_clock=gst_element_get_clock(pipeline);

  int thread1_loops=0;
  int sizex=((thread1_loops % 5)+1)*50;
  int sizey=(((thread1_loops+3) % 5)+1)*50;

  g_object_set(gltextoverlay,"text","text1 Текст Длинный",NULL);

   //общий цикл, для выполнения перезагрузок (если они требуются)
  while(!end_flag){

    //sleep(1);
    g_mutex_lock(&mutex_pool_releaseing);

    if(selector==1){

       int test_selector=1;
       int test_counter=0;
       if(strcmp(((char *)data),"test1")==0 || strcmp(((char *)data),"")==0)test_selector=1;
       //if(strcmp(((char *)data),"test2")==0)test_selector=2;
       //if(strcmp(((char *)data),"test3")==0)test_selector=3;

       test_selector=1;
       int sort_selector;

       sort_selector=0;

       float x,y,mult;
       x=0;
       y=0;
       mult=1.0;
       if(test_selector==1){


         g_object_set(gltextoverlay,"text","text1 Текст Длинный",NULL);
         g_object_set(gltextoverlay,"bg-color-argb",0x00ffffff,NULL);

         usleep(1000000);
         mult+=0.1;
         g_object_set(gltextoverlay,"text-size-mult",mult,NULL);
         //g_object_set(gltextoverlay2,"align-x","left",NULL);
         //g_object_set(gltextoverlay2,"align-y","top",NULL);
         g_object_set(gltextoverlay2,"align-x","right",NULL);
         g_object_set(gltextoverlay2,"align-y","bottom",NULL);


         usleep(1000000);
         mult+=0.2;
         g_object_set(gltextoverlay,"text-size-mult",mult,NULL);


         usleep(1000000);
         mult+=0.4;
         g_object_set(gltextoverlay,"text-size-mult",mult,NULL);


         g_object_set(gltextoverlay,"bg-color-argb",0x20ffffff,NULL);
         g_object_set(gltextoverlay,"text-color-argb",0x9900ff00,NULL);


         usleep(3000000);
         thread1_loops++;
         sizex=500;
         sizey=50;
         sprintf(buf,
           "video/x-raw(memory:GLMemory),"
           "format=(string)RGBA,"
           "width=(int)%d,"
           "height=(int)%d,"
           "framerate=(fraction)25/1,"
           "texture-target=(string)2D",sizex,sizey);
         filtercaps_caps1= gst_caps_from_string(buf);
         g_object_set (G_OBJECT (filtercaps1), "caps", filtercaps_caps1, NULL);
         gst_caps_unref (filtercaps_caps1);
         sprintf(str1,"sink_1");
         pad=gst_element_get_static_pad(glvideomixer,str1);
         //g_object_set(pad,"xpos",0,NULL);
         //g_object_set(pad,"ypos",0,NULL);
         g_object_set(pad,"width",sizex,NULL);
         g_object_set(pad,"height",sizey,NULL);


         test_counter=0;
         while(!end_flag){
           x++;
           y++;

           if(x>1024)x=0;
           if(y>76)y=0;
           usleep(200000);

           g_object_set(gltextoverlay,"text-x",x/1024.0,NULL);
           g_object_set(gltextoverlay,"text-y",y/768.0,NULL);

           err_sends_counter++;
           if(err_sends_counter>8)err_sends_counter=0;
         }


      }





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

      w=1224;
      h=500;//868;


      gltestsrc = gst_element_factory_make("gltestsrc", "_gltestsrc_");

      gltextoverlay = gst_element_factory_make("gltextoverlay", "_gltextoverlay_");
      gltextoverlay2 = gst_element_factory_make("gltextoverlay", "_gltextoverlay2_");
      glvideomixer = gst_element_factory_make ("glvideomixer", "_glvideomixer_");
      glimagesink  = gst_element_factory_make ("glimagesink", "_glimagesink_");

      sprintf(buf1,"video/x-raw(memory:GLMemory),"
                   "format=(string)RGBA,"
                   "width=(int)%d,"
                   "height=(int)%d,"
                   "framerate=(fraction)25/1,"
                   "texture-target=(string)2D",w/2,h/10);


      filtercaps1 = gst_element_factory_make("capsfilter","_filtercaps1_");
      filtercaps_caps1 = gst_caps_from_string(buf1);
      g_object_set (G_OBJECT (filtercaps1), "caps", filtercaps_caps1, NULL);
      gst_caps_unref (filtercaps_caps1);

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

      if (!gltextoverlay
          || !glvideomixer
          || !glimagesink
          || !filtercaps5
          ) {
        g_print ("Error init\n");
        return -1;
      }



      gst_bin_add_many (GST_BIN (pipeline),

        gltestsrc,
        filtercaps3,
        gltextoverlay,
        gltextoverlay2,
        filtercaps1,
        filtercaps2,
        glvideomixer,
        filtercaps5,
        glimagesink,
        NULL);


      gst_element_link_many(

        gltestsrc,
        filtercaps3,
        glvideomixer,
        NULL);


      gst_element_link_many(

        gltextoverlay,
        filtercaps1,
        glvideomixer,
        NULL);


      gst_element_link_many(

        gltextoverlay2,
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

      g_object_set(pad,"xpos",0,NULL);
      g_object_set(pad,"ypos",h/3,NULL);
      g_object_set(pad,"width",w/2,NULL);
      g_object_set(pad,"height",h/10,NULL);

      sprintf(str1,"sink_2");
      pad=gst_element_get_static_pad(glvideomixer,str1);

      g_object_set(pad,"xpos",w/2,NULL);
      g_object_set(pad,"ypos",0,NULL);
      g_object_set(pad,"width",w/2,NULL);
      g_object_set(pad,"height",h/10,NULL);

      g_object_set(gltextoverlay,"set-errors",0x07,NULL);
      g_object_set(gltextoverlay2,"text","Текст 2",NULL);

      //g_object_set(gltextoverlay,"font-caption","Noto Serif Display",NULL);
      g_object_set(gltextoverlay,"font-style","Italic",NULL);
      //g_object_set(gltextoverlay,"font-style","Bold",NULL);
      //g_object_set(gltextoverlay,"set-history-size",5,NULL);
      //g_object_set(gltextoverlay,"sort",0,NULL);

      //gltextoverlay_sink_pad=gst_element_get_static_pad(gltextoverlay,"sink");

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




