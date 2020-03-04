#include <gst/gst.h>
#include "config.h"

#define PLUGIN_NAME     "analysis"
#define PLUGIN_DESC     "Package for picture and sound quality analysis"
#define PLUGIN_LICENSE  "Proprietary"

static gboolean
plugin_init (GstPlugin * plugin)
{
  return TRUE;
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    analysis,
    PLUGIN_DESC,
    plugin_init, PACKAGE_VERSION, PLUGIN_LICENSE, PACKAGE, PACKAGE_BUGREPORT)
