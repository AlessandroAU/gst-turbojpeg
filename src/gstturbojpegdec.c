#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstturbojpegdec.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_turbojpegdec_debug);
#define GST_CAT_DEFAULT gst_turbojpegdec_debug

enum
{
  PROP_0,
  PROP_MAX_ERRORS
};

#define DEFAULT_MAX_ERRORS 5

static GstStaticPadTemplate gst_turbojpegdec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg")
    );

static GstStaticPadTemplate gst_turbojpegdec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGB, I420 }"))
    );

#define gst_turbojpegdec_parent_class parent_class
G_DEFINE_TYPE (GstTurboJpegDec, gst_turbojpegdec, GST_TYPE_VIDEO_DECODER);

GST_ELEMENT_REGISTER_DEFINE (turbojpegdec, "turbojpegdec", GST_RANK_PRIMARY + 1,
    GST_TYPE_TURBOJPEGDEC);

static void gst_turbojpegdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_turbojpegdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_turbojpegdec_finalize (GObject * object);

static gboolean gst_turbojpegdec_start (GstVideoDecoder * decoder);
static gboolean gst_turbojpegdec_stop (GstVideoDecoder * decoder);
static gboolean gst_turbojpegdec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_turbojpegdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);

static void
gst_turbojpegdec_class_init (GstTurboJpegDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstVideoDecoderClass *vdec_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  vdec_class = (GstVideoDecoderClass *) klass;

  gobject_class->set_property = gst_turbojpegdec_set_property;
  gobject_class->get_property = gst_turbojpegdec_get_property;
  gobject_class->finalize = gst_turbojpegdec_finalize;

  g_object_class_install_property (gobject_class, PROP_MAX_ERRORS,
      g_param_spec_int ("max-errors", "Maximum Errors",
          "Maximum number of errors tolerated before stopping processing",
          0, G_MAXINT, DEFAULT_MAX_ERRORS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (element_class,
      &gst_turbojpegdec_sink_pad_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_turbojpegdec_src_pad_template);

  gst_element_class_set_static_metadata (element_class,
      "TurboJPEG decoder", "Codec/Decoder/Image",
      "Decode JPEG images using libturbojpeg",
      "Your Name <your.email@example.com>");

  vdec_class->start = GST_DEBUG_FUNCPTR (gst_turbojpegdec_start);
  vdec_class->stop = GST_DEBUG_FUNCPTR (gst_turbojpegdec_stop);
  vdec_class->set_format = GST_DEBUG_FUNCPTR (gst_turbojpegdec_set_format);
  vdec_class->handle_frame = GST_DEBUG_FUNCPTR (gst_turbojpegdec_handle_frame);

  GST_DEBUG_CATEGORY_INIT (gst_turbojpegdec_debug, "turbojpegdec", 0,
      "TurboJPEG decoder");
}

static void
gst_turbojpegdec_init (GstTurboJpegDec * dec)
{
  dec->tjInstance = NULL;
  dec->max_errors = DEFAULT_MAX_ERRORS;
  dec->error_count = 0;
  dec->input_state = NULL;
  dec->output_state = NULL;

  gst_video_decoder_set_packetized (GST_VIDEO_DECODER (dec), TRUE);
}

static void
gst_turbojpegdec_finalize (GObject * object)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (object);

  if (dec->tjInstance) {
    tj3Destroy (dec->tjInstance);
    dec->tjInstance = NULL;
  }

  if (dec->input_state)
    gst_video_codec_state_unref (dec->input_state);
  if (dec->output_state)
    gst_video_codec_state_unref (dec->output_state);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_turbojpegdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (object);

  switch (prop_id) {
    case PROP_MAX_ERRORS:
      dec->max_errors = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_turbojpegdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (object);

  switch (prop_id) {
    case PROP_MAX_ERRORS:
      g_value_set_int (value, dec->max_errors);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_turbojpegdec_start (GstVideoDecoder * decoder)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);

  dec->tjInstance = tj3Init (TJINIT_DECOMPRESS);
  if (!dec->tjInstance) {
    GST_ERROR_OBJECT (dec, "Failed to initialize TurboJPEG decompressor");
    return FALSE;
  }

  dec->error_count = 0;

  GST_WARNING_OBJECT (dec, "*** TurboJPEG decoder CONTEXT CREATED ***");
  return TRUE;
}

static gboolean
gst_turbojpegdec_stop (GstVideoDecoder * decoder)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);

  if (dec->tjInstance) {
    tj3Destroy (dec->tjInstance);
    dec->tjInstance = NULL;
  }

  if (dec->input_state) {
    gst_video_codec_state_unref (dec->input_state);
    dec->input_state = NULL;
  }

  if (dec->output_state) {
    gst_video_codec_state_unref (dec->output_state);
    dec->output_state = NULL;
  }

  GST_WARNING_OBJECT (dec, "*** TurboJPEG decoder CONTEXT DESTROYED ***");
  return TRUE;
}

static int
gst_turbojpegdec_get_tj_pixel_format (GstVideoFormat format)
{
  switch (format) {
    case GST_VIDEO_FORMAT_RGB:
      return TJPF_RGB;
    case GST_VIDEO_FORMAT_BGR:
      return TJPF_BGR;
    case GST_VIDEO_FORMAT_RGBx:
      return TJPF_RGBX;
    case GST_VIDEO_FORMAT_BGRx:
      return TJPF_BGRX;
    case GST_VIDEO_FORMAT_xRGB:
      return TJPF_XRGB;
    case GST_VIDEO_FORMAT_xBGR:
      return TJPF_XBGR;
    case GST_VIDEO_FORMAT_GRAY8:
      return TJPF_GRAY;
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_YVYU:
    default:
      return -1;
  }
}

static gboolean
gst_turbojpegdec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);

  GST_DEBUG_OBJECT (dec, "Setting new caps %" GST_PTR_FORMAT, state->caps);

  if (dec->input_state)
    gst_video_codec_state_unref (dec->input_state);
  dec->input_state = gst_video_codec_state_ref (state);

  return TRUE;
}


static GstFlowReturn
gst_turbojpegdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);
  GstMapInfo map;
  GstFlowReturn ret = GST_FLOW_OK;
  int width, height, subsamp, colorspace;
  int tj_width, tj_height;
  GstVideoFormat format;
  int tj_format;
  guchar *output_data;
  GstVideoFrame vframe;

  if (!gst_buffer_map (frame->input_buffer, &map, GST_MAP_READ)) {
    GST_ERROR_OBJECT (dec, "Failed to map input buffer");
    return GST_FLOW_ERROR;
  }

  if (tj3DecompressHeader (dec->tjInstance, map.data, map.size) != 0) {
    GST_ERROR_OBJECT (dec, "Failed to read JPEG header: %s", tj3GetErrorStr (dec->tjInstance));
    gst_buffer_unmap (frame->input_buffer, &map);
    dec->error_count++;
    if (dec->error_count >= dec->max_errors) {
      return GST_FLOW_ERROR;
    }
    return GST_FLOW_OK;
  }

  tj_width = tj3Get (dec->tjInstance, TJPARAM_JPEGWIDTH);
  tj_height = tj3Get (dec->tjInstance, TJPARAM_JPEGHEIGHT);
  subsamp = tj3Get (dec->tjInstance, TJPARAM_SUBSAMP);
  colorspace = tj3Get (dec->tjInstance, TJPARAM_COLORSPACE);
  
  width = tj_width;
  height = tj_height;

  /* Implement proper format negotiation - prefer I420 for pipeline compatibility */
  GstCaps *allowed_caps = gst_pad_get_allowed_caps (GST_VIDEO_DECODER_SRC_PAD (decoder));
  GstVideoFormat best_format = GST_VIDEO_FORMAT_UNKNOWN;
  
  /* Preferred formats: I420 first (most compatible), then RGB */
  GstVideoFormat preferred_formats[] = { GST_VIDEO_FORMAT_I420, GST_VIDEO_FORMAT_RGB };
  
  for (int i = 0; i < G_N_ELEMENTS(preferred_formats); i++) {
    GstVideoInfo temp_info;
    GstCaps *temp_caps;
    
    gst_video_info_set_format (&temp_info, preferred_formats[i], width, height);
    temp_caps = gst_video_info_to_caps (&temp_info);
    
    if (temp_caps && allowed_caps && gst_caps_can_intersect (temp_caps, allowed_caps)) {
      best_format = preferred_formats[i];
      gst_caps_unref (temp_caps);
      break;
    }
    if (temp_caps) gst_caps_unref (temp_caps);
  }
  
  if (allowed_caps) gst_caps_unref (allowed_caps);
  
  if (best_format == GST_VIDEO_FORMAT_UNKNOWN) {
    /* Fallback to I420 if negotiation fails */
    best_format = GST_VIDEO_FORMAT_I420;
  }
  
  format = best_format;
  tj_format = (format == GST_VIDEO_FORMAT_RGB) ? TJPF_RGB : -1;

  if (!dec->output_state ||
      GST_VIDEO_INFO_WIDTH (&dec->output_state->info) != width ||
      GST_VIDEO_INFO_HEIGHT (&dec->output_state->info) != height ||
      GST_VIDEO_INFO_FORMAT (&dec->output_state->info) != format) {

    if (dec->output_state)
      gst_video_codec_state_unref (dec->output_state);

    dec->output_state = gst_video_decoder_set_output_state (decoder,
        format, width, height, dec->input_state);
    
    if (!gst_video_decoder_negotiate (decoder)) {
      GST_ERROR_OBJECT (dec, "Failed to negotiate output format");
      gst_buffer_unmap (frame->input_buffer, &map);
      return GST_FLOW_NOT_NEGOTIATED;
    }
  }

  /* Use the output state we already have */
  ret = gst_video_decoder_allocate_output_frame (decoder, frame);
  if (ret != GST_FLOW_OK) {
    GST_ERROR_OBJECT (dec, "Failed to allocate output frame");
    gst_buffer_unmap (frame->input_buffer, &map);
    return ret;
  }

  if (!gst_video_frame_map (&vframe, &dec->output_state->info, frame->output_buffer,
          GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (dec, "Failed to map output frame");
    gst_buffer_unmap (frame->input_buffer, &map);
    return GST_FLOW_ERROR;
  }

  GST_DEBUG_OBJECT (dec, "Decompressing %dx%d JPEG to %s format", 
                   tj_width, tj_height, gst_video_format_to_string(format));
  
  if (format == GST_VIDEO_FORMAT_I420) {
    /* Use optimized YUV planar decompression - writes directly to GStreamer planes */
    guchar *planes[3];
    int strides[3];
    
    planes[0] = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0); /* Y plane */
    planes[1] = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 1); /* U plane */
    planes[2] = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 2); /* V plane */
    
    strides[0] = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 0);
    strides[1] = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 1);
    strides[2] = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 2);
    
    if (tj3DecompressToYUVPlanes8 (dec->tjInstance, map.data, map.size,
            planes, strides) != 0) {
      GST_ERROR_OBJECT (dec, "Failed to decompress JPEG to YUV: %s", tj3GetErrorStr (dec->tjInstance));
      gst_video_frame_unmap (&vframe);
      gst_buffer_unmap (frame->input_buffer, &map);
      dec->error_count++;
      if (dec->error_count >= dec->max_errors) {
        return GST_FLOW_ERROR;
      }
      return GST_FLOW_OK;
    }
  } else {
    /* Use RGB decompression with SIMD acceleration */
    output_data = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0);
    
    if (tj3Decompress8 (dec->tjInstance, map.data, map.size,
            output_data, GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 0), tj_format) != 0) {
      GST_ERROR_OBJECT (dec, "Failed to decompress JPEG to RGB: %s", tj3GetErrorStr (dec->tjInstance));
      gst_video_frame_unmap (&vframe);
      gst_buffer_unmap (frame->input_buffer, &map);
      dec->error_count++;
      if (dec->error_count >= dec->max_errors) {
        return GST_FLOW_ERROR;
      }
      return GST_FLOW_OK;
    }
  }

  gst_video_frame_unmap (&vframe);
  gst_buffer_unmap (frame->input_buffer, &map);
  
  dec->error_count = 0;

  return gst_video_decoder_finish_frame (decoder, frame);
}