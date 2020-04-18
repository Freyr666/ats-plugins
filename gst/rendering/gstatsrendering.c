#include <gst/gst.h>

#include "config.h"

#include "gstglsoundbar.h"

#define PLUGIN_NAME     "atsrendering"
#define PLUGIN_DESC     "Package for various rendering elements"
#define PLUGIN_LICENSE  "LGPL"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = TRUE;

  ret &= gst_element_register (plugin,
      "glsoundbar", GST_RANK_NONE, GST_TYPE_GLSOUNDBAR);

  return ret;
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    atsrendering,
    PLUGIN_DESC,
    plugin_init, PACKAGE_VERSION, PLUGIN_LICENSE, PACKAGE, PACKAGE_BUGREPORT)
