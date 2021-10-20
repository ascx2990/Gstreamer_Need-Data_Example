#include <gst/gst.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#define INPUT_IMG_WIDTH 512
#define INPUT_IMG_HEIGHT 512
#define INPUT_VIDEO_WIDTH 512
#define INPUT_VIDEO_HEIGHT 512

pthread_mutex_t depth_mutex = PTHREAD_MUTEX_INITIALIZER;

// static unsigned long int gColorImgSize = 0;

static GMainLoop *loop;
//用矩陣紀錄抓取的每張frame

cv::VideoCapture gCap;
typedef struct _GstPip
{
  GstElement *color_image_pipeline, *color_image_appsrc, *color_image_conv, *color_image_tee, *color_image_queue1, *color_image_queue2, *color_image_imagesink, *color_image_videosink, *color_image_conv1, *color_image_caps;
  GstBus *bus;
  guint bus_watch_id;
  guint sourceid;   /* To control the GSource */
  gboolean playing; /* Playing or Paused */
} GstPip;

static gboolean cb_need_data(GstPip *gstPip)
{
  static gboolean white = FALSE;
  static GstClockTime timestamp = 0;
  GstBuffer *buffer;
  guint size, depth, height, width, step, channels;
  GstFlowReturn ret;
  IplImage *img;
  guchar *data1;
  GstMapInfo map;

  img = cvLoadImage("./raw.jpg", CV_LOAD_IMAGE_COLOR);
  height = img->height;
  width = img->width;
  step = img->widthStep;
  channels = img->nChannels;
  depth = img->depth;
  data1 = (guchar *)img->imageData;
  size = height * width * channels;
  // printf("height:%d\n", height);
  // printf("width:%d\n", width);
  // printf("channels:%d\n", channels);
  // printf("size:%d\n", size);
  buffer = gst_buffer_new_allocate(NULL, size, NULL);
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  pthread_mutex_lock(&depth_mutex);
  memcpy((guchar *)map.data, data1, gst_buffer_get_size(buffer));
  pthread_mutex_unlock(&depth_mutex);
  GST_BUFFER_PTS(buffer) = timestamp;
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 1); // fps
  timestamp += GST_BUFFER_DURATION(buffer);

  g_signal_emit_by_name(gstPip->color_image_appsrc, "push-buffer", buffer, &ret);

  if (ret != GST_FLOW_OK)
  {
    /* something wrong, stop pushing */
    std::cout << "!GST_FLOW_OK" << std::endl;
    g_main_loop_quit(loop);
    return FALSE;
  }
  return TRUE;
}

static void start_feed_color_image(GstElement *appsrc, guint unused_size, GstPip *gstPip)
{
  if (gstPip->sourceid == 0)
  {
    // g_print("Start feeding\n");
    gstPip->sourceid = g_idle_add((GSourceFunc)cb_need_data, gstPip); // image
  }
}
static void stop_feed_color_image(GstElement *appsrc, GstPip *gstPip)
{
  if (gstPip->sourceid != 0)
  {
    // g_print("Stop feeding\n");
    g_source_remove(gstPip->sourceid);
    gstPip->sourceid = 0;
  }
}

static GstFlowReturn colorImageCallback(GstElement *sink)
{
  // std::cout << "colorImageCallback" << std::endl;
  GstSample *sample;
  GstMemory *mem;
  GstMapInfo map; // map.data
  mem = gst_allocator_alloc(NULL, 1000000, NULL);
  gst_memory_map(mem, &map, GST_MAP_WRITE);
  g_signal_emit_by_name(sink, "pull-sample", &sample);
  if (sample)
  {

    GstBuffer *buffer = gst_sample_get_buffer(sample);

    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
      // save image
      //   g_print("size: %d\n", map.size);
      /* The only thing we do in this example is print a * to indicate a received buffer */
      cv::Mat image = cv::Mat(INPUT_IMG_HEIGHT, INPUT_IMG_WIDTH, CV_8UC3, map.data);
      //  if need chang formate
      // cv::Mat RGB_img = cv::cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
      cv::imwrite("color_image.jpg", image);
    }
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
}

gint main(gint argc, gchar *argv[])
{
  GstPip *gstPip;
  gstPip = g_new0(GstPip, 1);
  /* init GStreamer */

  gst_init(&argc, &argv);
  loop = g_main_loop_new(NULL, FALSE);

  /* setup pipeline */
  /* setup color image pipeline */
  gstPip->color_image_pipeline = gst_pipeline_new("pipeline");
  gstPip->color_image_appsrc = gst_element_factory_make("appsrc", "color_image_source");
  gstPip->color_image_conv = gst_element_factory_make("videoconvert", "color_image_conv");

  gstPip->color_image_conv1 = gst_element_factory_make("videoconvert", "color_image_conv1");
  gstPip->color_image_caps = gst_element_factory_make("capsfilter", "caps1");
  gstPip->color_image_tee = gst_element_factory_make("tee", "color_image_tee");
  gstPip->color_image_queue1 = gst_element_factory_make("queue", "color_image_queue1");
  gstPip->color_image_queue2 = gst_element_factory_make("queue", "color_image_queue2");
  gstPip->color_image_videosink = gst_element_factory_make("xvimagesink", "color_image_videosink");
  gstPip->color_image_imagesink = gst_element_factory_make("appsink", "color_image_imagesink");

  /* setup */
  g_object_set(G_OBJECT(gstPip->color_image_appsrc), "caps",
               gst_caps_new_simple("video/x-raw",
                                   "format", G_TYPE_STRING, "BGR",
                                   "width", G_TYPE_INT, INPUT_IMG_WIDTH,
                                   "height", G_TYPE_INT, INPUT_IMG_HEIGHT,
                                   "framerate", GST_TYPE_FRACTION, 1, 1,
                                   NULL),
               NULL);

  g_object_set(G_OBJECT(gstPip->color_image_caps), "caps", gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, INPUT_IMG_WIDTH, "height", G_TYPE_INT, INPUT_IMG_HEIGHT, "framerate", GST_TYPE_FRACTION, 1, 1, NULL),
               NULL);
  gst_bin_add_many(GST_BIN(gstPip->color_image_pipeline), gstPip->color_image_appsrc, gstPip->color_image_conv, gstPip->color_image_videosink,
                   gstPip->color_image_tee, gstPip->color_image_queue1, gstPip->color_image_queue2, gstPip->color_image_imagesink, gstPip->color_image_conv1, gstPip->color_image_caps, NULL);
  gst_element_link_many(gstPip->color_image_appsrc, gstPip->color_image_conv, gstPip->color_image_tee, NULL);
  gst_element_link_many(gstPip->color_image_tee, gstPip->color_image_queue1, gstPip->color_image_videosink, NULL);
  gst_element_link_many(gstPip->color_image_tee, gstPip->color_image_queue2, gstPip->color_image_conv1, gstPip->color_image_caps, gstPip->color_image_imagesink, NULL);

  /* setup appsrc */
  g_object_set(G_OBJECT(gstPip->color_image_appsrc),
              
               "format", GST_FORMAT_TIME, NULL);
  g_signal_connect(gstPip->color_image_appsrc, "need-data", G_CALLBACK(start_feed_color_image), gstPip);
  g_signal_connect(gstPip->color_image_appsrc, "enough-data", G_CALLBACK(stop_feed_color_image), gstPip);

  /* Enable the last-sample property. If FALSE, basesink doesn't keep a
   * reference to the last buffer arrived and the last-sample property is always
   * set to %NULL. This can be useful if you need buffers to be released as soon
   * as possible, eg. if you're using a buffer pool.
   * */
  g_object_set(gstPip->color_image_imagesink, "enable-last-sample", TRUE, NULL);
  g_object_set(gstPip->color_image_imagesink, "emit-signals", TRUE, NULL);
  g_signal_connect(gstPip->color_image_imagesink, "new-sample", G_CALLBACK(colorImageCallback), NULL);

  gst_element_set_state(gstPip->color_image_pipeline, GST_STATE_PLAYING);
  g_print("Running...\n");
  g_main_loop_run(loop);

  /* clean up */
  g_print("Returned, stopping playback\n");
  gst_element_set_state(gstPip->color_image_pipeline, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT(gstPip->color_image_pipeline));
  g_main_loop_unref(loop);

  return 0;
}
