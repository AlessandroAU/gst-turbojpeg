#ifndef __GST_TURBOJPEGDEC_H__
#define __GST_TURBOJPEGDEC_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideodecoder.h>
#include <turbojpeg.h>

G_BEGIN_DECLS

#define GST_TYPE_TURBOJPEGDEC \
  (gst_turbojpegdec_get_type())
#define GST_TURBOJPEGDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TURBOJPEGDEC,GstTurboJpegDec))
#define GST_TURBOJPEGDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TURBOJPEGDEC,GstTurboJpegDecClass))
#define GST_IS_TURBOJPEGDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TURBOJPEGDEC))
#define GST_IS_TURBOJPEGDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TURBOJPEGDEC))

typedef struct _GstTurboJpegDec GstTurboJpegDec;
typedef struct _GstTurboJpegDecClass GstTurboJpegDecClass;

struct _GstTurboJpegDec
{
  GstVideoDecoder parent;

  tjhandle tjInstance;
  
  gint max_errors;
  gint error_count;
  
  GstVideoCodecState *input_state;
  GstVideoCodecState *output_state;
};

struct _GstTurboJpegDecClass
{
  GstVideoDecoderClass parent_class;
};

GType gst_turbojpegdec_get_type (void);

GST_ELEMENT_REGISTER_DECLARE (turbojpegdec);

G_END_DECLS

#endif /* __GST_TURBOJPEGDEC_H__ */