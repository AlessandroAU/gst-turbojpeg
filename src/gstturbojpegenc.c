#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstturbojpegenc.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_turbojpegenc_debug);
#define GST_CAT_DEFAULT gst_turbojpegenc_debug

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_SUBSAMPLING,
  PROP_OPTIMIZED_HUFFMAN,
  PROP_PROGRESSIVE
};

#define DEFAULT_QUALITY 80
#define DEFAULT_SUBSAMPLING TJSAMP_420  /* 4:2:0 subsampling */
#define DEFAULT_OPTIMIZED_HUFFMAN TRUE
#define DEFAULT_PROGRESSIVE FALSE

static GstStaticPadTemplate gst_turbojpegenc_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGB, I420 }"))
    );

static GstStaticPadTemplate gst_turbojpegenc_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg, "
        "width = (int) [ 1, MAX ], "
        "height = (int) [ 1, MAX ], "
        "framerate = (fraction) [ 0/1, MAX ]")
    );

#define gst_turbojpegenc_parent_class parent_class
G_DEFINE_TYPE (GstTurboJpegEnc, gst_turbojpegenc, GST_TYPE_VIDEO_ENCODER);

GST_ELEMENT_REGISTER_DEFINE (turbojpegenc, "turbojpegenc", GST_RANK_PRIMARY + 1,
    GST_TYPE_TURBOJPEGENC);

static void gst_turbojpegenc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_turbojpegenc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_turbojpegenc_finalize (GObject * object);

static gboolean gst_turbojpegenc_start (GstVideoEncoder * encoder);
static gboolean gst_turbojpegenc_stop (GstVideoEncoder * encoder);
static gboolean gst_turbojpegenc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_turbojpegenc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);

static void
gst_turbojpegenc_class_init (GstTurboJpegEncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstVideoEncoderClass *venc_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  venc_class = (GstVideoEncoderClass *) klass;

  gobject_class->set_property = gst_turbojpegenc_set_property;
  gobject_class->get_property = gst_turbojpegenc_get_property;
  gobject_class->finalize = gst_turbojpegenc_finalize;

  g_object_class_install_property (gobject_class, PROP_QUALITY,
      g_param_spec_int ("quality", "Quality",
          "JPEG compression quality (1-100, higher = better quality)",
          1, 100, DEFAULT_QUALITY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SUBSAMPLING,
      g_param_spec_int ("subsampling", "Chroma Subsampling",
          "Chroma subsampling mode (0=4:4:4, 1=4:2:2, 2=4:2:0, 3=GRAY, 4=4:4:0)",
          0, 4, DEFAULT_SUBSAMPLING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OPTIMIZED_HUFFMAN,
      g_param_spec_boolean ("optimized-huffman", "Optimized Huffman",
          "Use optimized Huffman coding for better compression",
          DEFAULT_OPTIMIZED_HUFFMAN,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROGRESSIVE,
      g_param_spec_boolean ("progressive", "Progressive JPEG",
          "Generate progressive JPEG images",
          DEFAULT_PROGRESSIVE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (element_class,
      &gst_turbojpegenc_sink_pad_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_turbojpegenc_src_pad_template);

  gst_element_class_set_static_metadata (element_class,
      "TurboJPEG encoder", "Codec/Encoder/Image",
      "Encode video frames to JPEG images using libturbojpeg",
      "Your Name <your.email@example.com>");

  venc_class->start = GST_DEBUG_FUNCPTR (gst_turbojpegenc_start);
  venc_class->stop = GST_DEBUG_FUNCPTR (gst_turbojpegenc_stop);
  venc_class->set_format = GST_DEBUG_FUNCPTR (gst_turbojpegenc_set_format);
  venc_class->handle_frame = GST_DEBUG_FUNCPTR (gst_turbojpegenc_handle_frame);

  GST_DEBUG_CATEGORY_INIT (gst_turbojpegenc_debug, "turbojpegenc", 0,
      "TurboJPEG encoder");
}

static void
gst_turbojpegenc_init (GstTurboJpegEnc * enc)
{
  enc->tjInstance = NULL;
  enc->quality = DEFAULT_QUALITY;
  enc->subsampling = DEFAULT_SUBSAMPLING;
  enc->optimized_huffman = DEFAULT_OPTIMIZED_HUFFMAN;
  enc->progressive = DEFAULT_PROGRESSIVE;
  enc->input_state = NULL;
  
  /* Initialize memory optimization fields */
  enc->jpeg_buffer = NULL;
  enc->jpeg_buffer_size = 0;
  enc->buffer_pool = NULL;
}

static void
gst_turbojpegenc_finalize (GObject * object)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (object);

  if (enc->tjInstance) {
    tjDestroy (enc->tjInstance);
    enc->tjInstance = NULL;
  }

  if (enc->input_state)
    gst_video_codec_state_unref (enc->input_state);

  /* Clean up memory optimization resources */
  if (enc->jpeg_buffer) {
    g_free (enc->jpeg_buffer);
    enc->jpeg_buffer = NULL;
  }
  
  if (enc->buffer_pool) {
    gst_object_unref (enc->buffer_pool);
    enc->buffer_pool = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_turbojpegenc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (object);

  switch (prop_id) {
    case PROP_QUALITY:
      enc->quality = g_value_get_int (value);
      break;
    case PROP_SUBSAMPLING:
      enc->subsampling = g_value_get_int (value);
      break;
    case PROP_OPTIMIZED_HUFFMAN:
      enc->optimized_huffman = g_value_get_boolean (value);
      break;
    case PROP_PROGRESSIVE:
      enc->progressive = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_turbojpegenc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (object);

  switch (prop_id) {
    case PROP_QUALITY:
      g_value_set_int (value, enc->quality);
      break;
    case PROP_SUBSAMPLING:
      g_value_set_int (value, enc->subsampling);
      break;
    case PROP_OPTIMIZED_HUFFMAN:
      g_value_set_boolean (value, enc->optimized_huffman);
      break;
    case PROP_PROGRESSIVE:
      g_value_set_boolean (value, enc->progressive);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_turbojpegenc_start (GstVideoEncoder * encoder)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (encoder);

  enc->tjInstance = tjInitCompress ();
  if (!enc->tjInstance) {
    GST_ERROR_OBJECT (enc, "Failed to initialize TurboJPEG compressor");
    return FALSE;
  }

  GST_WARNING_OBJECT (enc, "*** TurboJPEG encoder CONTEXT CREATED ***");
  return TRUE;
}

static gboolean
gst_turbojpegenc_stop (GstVideoEncoder * encoder)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (encoder);

  if (enc->tjInstance) {
    tjDestroy (enc->tjInstance);
    enc->tjInstance = NULL;
  }

  if (enc->input_state) {
    gst_video_codec_state_unref (enc->input_state);
    enc->input_state = NULL;
  }

  /* Clean up buffer pool */
  if (enc->buffer_pool) {
    gst_buffer_pool_set_active (enc->buffer_pool, FALSE);
    gst_object_unref (enc->buffer_pool);
    enc->buffer_pool = NULL;
  }

  GST_WARNING_OBJECT (enc, "*** TurboJPEG encoder CONTEXT DESTROYED ***");
  return TRUE;
}

static int
gst_turbojpegenc_get_tj_pixel_format (GstVideoFormat format)
{
  switch (format) {
    case GST_VIDEO_FORMAT_RGB:
      return TJPF_RGB;
    case GST_VIDEO_FORMAT_I420:
    default:
      return -1;  /* Use YUV handling for I420 */
  }
}

static gboolean
gst_turbojpegenc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (encoder);
  GstVideoCodecState *output_state;
  GstCaps *caps;

  GST_DEBUG_OBJECT (enc, "Setting new caps %" GST_PTR_FORMAT, state->caps);

  if (enc->input_state)
    gst_video_codec_state_unref (enc->input_state);
  enc->input_state = gst_video_codec_state_ref (state);

  /* Allocate JPEG buffer for maximum possible size to avoid malloc/free churn */
  gint width = GST_VIDEO_INFO_WIDTH (&state->info);
  gint height = GST_VIDEO_INFO_HEIGHT (&state->info);
  gulong max_jpeg_size = tjBufSize (width, height, enc->subsampling);
  
  if (enc->jpeg_buffer) {
    g_free (enc->jpeg_buffer);
  }
  enc->jpeg_buffer = g_malloc (max_jpeg_size);
  enc->jpeg_buffer_size = max_jpeg_size;
  
  GST_DEBUG_OBJECT (enc, "Allocated JPEG buffer of size %lu bytes for %dx%d", 
                   max_jpeg_size, width, height);

  /* Create buffer pool for output buffers to avoid per-frame allocation */
  if (enc->buffer_pool) {
    gst_buffer_pool_set_active (enc->buffer_pool, FALSE);
    gst_object_unref (enc->buffer_pool);
  }
  
  enc->buffer_pool = gst_buffer_pool_new ();
  GstStructure *config = gst_buffer_pool_get_config (enc->buffer_pool);
  gst_buffer_pool_config_set_params (config, NULL, max_jpeg_size, 4, 8);
  gst_buffer_pool_config_set_allocator (config, NULL, NULL);
  
  if (!gst_buffer_pool_set_config (enc->buffer_pool, config)) {
    GST_ERROR_OBJECT (enc, "Failed to configure buffer pool");
    gst_object_unref (enc->buffer_pool);
    enc->buffer_pool = NULL;
  } else {
    gst_buffer_pool_set_active (enc->buffer_pool, TRUE);
    GST_DEBUG_OBJECT (enc, "Created buffer pool with %lu byte buffers", max_jpeg_size);
  }

  caps = gst_caps_new_simple ("image/jpeg",
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      "framerate", GST_TYPE_FRACTION,
      GST_VIDEO_INFO_FPS_N (&state->info),
      GST_VIDEO_INFO_FPS_D (&state->info), NULL);

  output_state = gst_video_encoder_set_output_state (encoder, caps, state);
  gst_video_codec_state_unref (output_state);

  return gst_video_encoder_negotiate (encoder);
}


static GstFlowReturn
gst_turbojpegenc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstTurboJpegEnc *enc = GST_TURBOJPEGENC (encoder);
  GstVideoFrame vframe;
  GstVideoFormat format;
  int tj_format;
  gint width, height;
  gulong jpeg_size;
  GstBuffer *output_buffer;
  int flags = 0;
  
  if (!gst_video_frame_map (&vframe, &enc->input_state->info, 
          frame->input_buffer, GST_MAP_READ)) {
    GST_ERROR_OBJECT (enc, "Failed to map input frame");
    return GST_FLOW_ERROR;
  }

  format = GST_VIDEO_FRAME_FORMAT (&vframe);
  width = GST_VIDEO_FRAME_WIDTH (&vframe);
  height = GST_VIDEO_FRAME_HEIGHT (&vframe);

  /* Set encoding flags */
  if (enc->progressive) {
    flags |= TJFLAG_PROGRESSIVE;
  }
  /* Note: Optimized Huffman is not available as a flag in this TurboJPEG version */

  tj_format = gst_turbojpegenc_get_tj_pixel_format (format);
  
  /* Use pre-allocated buffer size */
  jpeg_size = enc->jpeg_buffer_size;
  
  if (tj_format != -1) {
    /* Direct RGB/BGR encoding with pre-allocated buffer */
    guchar *src_data = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0);
    gint src_stride = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 0);
    
    if (tjCompress2 (enc->tjInstance, src_data, width, src_stride, height,
            tj_format, &enc->jpeg_buffer, &jpeg_size, enc->subsampling, 
            enc->quality, flags | TJFLAG_FASTDCT | TJFLAG_NOREALLOC) != 0) {
      GST_ERROR_OBJECT (enc, "Failed to compress JPEG: %s", tjGetErrorStr ());
      gst_video_frame_unmap (&vframe);
      return GST_FLOW_ERROR;
    }
  } else {
    /* YUV encoding */
    guchar *y_plane, *u_plane, *v_plane;
    const guchar *planes[3];
    int strides[3];
    
    y_plane = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0);
    u_plane = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 1);
    v_plane = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 2);
    
    planes[0] = y_plane;
    planes[1] = u_plane;
    planes[2] = v_plane;
    
    strides[0] = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 0);
    strides[1] = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 1);
    strides[2] = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 2);
    
    if (tjCompressFromYUVPlanes (enc->tjInstance, planes, width, strides, 
            height, enc->subsampling, &enc->jpeg_buffer, &jpeg_size, 
            enc->quality, flags | TJFLAG_FASTDCT | TJFLAG_NOREALLOC) != 0) {
      GST_ERROR_OBJECT (enc, "Failed to compress JPEG from YUV: %s", 
          tjGetErrorStr ());
      gst_video_frame_unmap (&vframe);
      return GST_FLOW_ERROR;
    }
  }

  gst_video_frame_unmap (&vframe);

  /* Get buffer from pool to avoid per-frame allocation */
  if (enc->buffer_pool) {
    GstFlowReturn pool_ret = gst_buffer_pool_acquire_buffer (enc->buffer_pool, 
                                                           &output_buffer, NULL);
    if (pool_ret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (enc, "Failed to acquire buffer from pool: %s", 
                       gst_flow_get_name (pool_ret));
      return pool_ret;
    }
    
    /* Map buffer and copy JPEG data */
    GstMapInfo map;
    if (gst_buffer_map (output_buffer, &map, GST_MAP_WRITE)) {
      memcpy (map.data, enc->jpeg_buffer, jpeg_size);
      gst_buffer_unmap (output_buffer, &map);
      gst_buffer_set_size (output_buffer, jpeg_size);
    } else {
      GST_ERROR_OBJECT (enc, "Failed to map output buffer");
      gst_buffer_unref (output_buffer);
      return GST_FLOW_ERROR;
    }
  } else {
    /* Fallback to direct allocation if pool failed */
    output_buffer = gst_buffer_new_allocate (NULL, jpeg_size, NULL);
    if (!output_buffer) {
      GST_ERROR_OBJECT (enc, "Failed to allocate output buffer");
      return GST_FLOW_ERROR;
    }
    gst_buffer_fill (output_buffer, 0, enc->jpeg_buffer, jpeg_size);
  }

  frame->output_buffer = output_buffer;

  return gst_video_encoder_finish_frame (encoder, frame);
}