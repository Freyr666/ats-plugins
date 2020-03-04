#include <gst/gst.h>

#include "config.h"

#include "gstvideoanalysis.h"
#include "gstaudioanalysis.h"

#define PLUGIN_NAME     "analysis"
#define PLUGIN_DESC     "Package for picture and sound quality analysis"
#define PLUGIN_LICENSE  "Proprietary"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = TRUE;

  ret &= gst_element_register (plugin,
      "videoanalysis", GST_RANK_NONE, GST_TYPE_VIDEOANALYSIS);

  ret &= gst_element_register (plugin,
      "audioanalysis", GST_RANK_NONE, GST_TYPE_AUDIOANALYSIS);

  return ret;
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    analysis,
    PLUGIN_DESC,
    plugin_init, PACKAGE_VERSION, PLUGIN_LICENSE, PACKAGE, PACKAGE_BUGREPORT)
