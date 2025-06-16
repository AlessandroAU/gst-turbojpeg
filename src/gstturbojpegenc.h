#ifndef __GST_TURBOJPEGENC_H__
#define __GST_TURBOJPEGENC_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>
#include <turbojpeg.h>

G_BEGIN_DECLS

#define GST_TYPE_TURBOJPEGENC \
  (gst_turbojpegenc_get_type())
#define GST_TURBOJPEGENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TURBOJPEGENC,GstTurboJpegEnc))
#define GST_TURBOJPEGENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TURBOJPEGENC,GstTurboJpegEncClass))
#define GST_IS_TURBOJPEGENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TURBOJPEGENC))
#define GST_IS_TURBOJPEGENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TURBOJPEGENC))

typedef struct _GstTurboJpegEnc GstTurboJpegEnc;
typedef struct _GstTurboJpegEncClass GstTurboJpegEncClass;

struct _GstTurboJpegEnc
{
  GstVideoEncoder parent;

  tjhandle tjInstance;
  
  gint quality;
  gint subsampling;
  gboolean optimized_huffman;
  gboolean progressive;
  
  GstVideoCodecState *input_state;
  
  /* Memory optimization fields */
  guchar *jpeg_buffer;        /* Pre-allocated JPEG output buffer */
  gulong jpeg_buffer_size;    /* Size of pre-allocated buffer */
  GstBufferPool *buffer_pool; /* Output buffer pool */
};

struct _GstTurboJpegEncClass
{
  GstVideoEncoderClass parent_class;
};

GType gst_turbojpegenc_get_type (void);

GST_ELEMENT_REGISTER_DECLARE (turbojpegenc);

G_END_DECLS

#endif /* __GST_TURBOJPEGENC_H__ */