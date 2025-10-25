#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstturbojpegdec.h"
#include "gstturbojpegenc.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;

  ret |= gst_turbojpegdec_register (plugin);
  ret |= gst_turbojpegenc_register (plugin);

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    turbojpeg,
    "TurboJPEG encoder/decoder plugin",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)