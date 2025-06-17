/* GStreamer TurboJPEG Decoder Plugin
 * Copyright (C) 2024 <organization>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideodecoder.h>
#include <string.h>

#include "gstturbojpegdec.h"

GST_DEBUG_CATEGORY_STATIC (gst_turbojpegdec_debug);
#define GST_CAT_DEFAULT gst_turbojpegdec_debug

enum
{
  PROP_0,
  PROP_MAX_ERRORS
};

#define DEFAULT_MAX_ERRORS 10

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, RGB, BGR, RGBx, BGRx, GRAY8 }"))
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
static gboolean gst_turbojpegdec_decide_allocation (GstVideoDecoder * decoder,
    GstQuery * query);

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
      g_param_spec_int ("max-errors", "Max errors",
          "Maximum number of errors before stopping decode",
          0, G_MAXINT, DEFAULT_MAX_ERRORS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_static_metadata (element_class,
      "TurboJPEG Decoder", "Codec/Decoder/Video",
      "Decode JPEG images using libturbojpeg",
      "GStreamer TurboJPEG Plugin");

  vdec_class->start = GST_DEBUG_FUNCPTR (gst_turbojpegdec_start);
  vdec_class->stop = GST_DEBUG_FUNCPTR (gst_turbojpegdec_stop);
  vdec_class->set_format = GST_DEBUG_FUNCPTR (gst_turbojpegdec_set_format);
  vdec_class->handle_frame = GST_DEBUG_FUNCPTR (gst_turbojpegdec_handle_frame);
  vdec_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_turbojpegdec_decide_allocation);

  GST_DEBUG_CATEGORY_INIT (gst_turbojpegdec_debug, "turbojpegdec", 0,
      "TurboJPEG decoder");
}

static void
gst_turbojpegdec_init (GstTurboJpegDec * dec)
{
  dec->tjInstance = NULL;
  g_mutex_init (&dec->tjInstance_lock);
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

  g_mutex_clear (&dec->tjInstance_lock);

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

  GST_DEBUG_OBJECT (dec, "Starting TurboJPEG decoder");

  dec->tjInstance = tj3Init (TJINIT_DECOMPRESS);
  if (!dec->tjInstance) {
    GST_ERROR_OBJECT (dec, "Failed to initialize TurboJPEG decompressor");
    return FALSE;
  }

  dec->error_count = 0;

  GST_DEBUG_OBJECT (dec, "TurboJPEG decoder started successfully");
  return TRUE;
}

static gboolean
gst_turbojpegdec_stop (GstVideoDecoder * decoder)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);

  GST_DEBUG_OBJECT (dec, "Stopping TurboJPEG decoder");

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

  GST_DEBUG_OBJECT (dec, "TurboJPEG decoder stopped");
  return TRUE;
}

static gboolean
gst_turbojpegdec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);

  GST_DEBUG_OBJECT (dec, "Setting format");

  if (dec->input_state)
    gst_video_codec_state_unref (dec->input_state);
  dec->input_state = gst_video_codec_state_ref (state);

  return TRUE;
}


static int
gst_turbojpegdec_get_tjpf_from_format (GstVideoFormat format)
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
    case GST_VIDEO_FORMAT_GRAY8:
      return TJPF_GRAY;
    default:
      return -1;
  }
}

static GstFlowReturn
gst_turbojpegdec_negotiate_format (GstTurboJpegDec * dec, gint width,
    gint height, gint subsamp)
{
  GstVideoDecoder *decoder = GST_VIDEO_DECODER (dec);
  GstVideoFormat format;
  GstVideoCodecState *output_state;
  GstCaps *allowed_caps;
  GstStructure *structure;
  const gchar *format_str;

  allowed_caps = gst_pad_get_allowed_caps (GST_VIDEO_DECODER_SRC_PAD (decoder));
  if (!allowed_caps) {
    allowed_caps = gst_pad_get_pad_template_caps (GST_VIDEO_DECODER_SRC_PAD (decoder));
  }

  allowed_caps = gst_caps_make_writable (allowed_caps);
  allowed_caps = gst_caps_fixate (allowed_caps);

  structure = gst_caps_get_structure (allowed_caps, 0);
  format_str = gst_structure_get_string (structure, "format");

  if (format_str) {
    format = gst_video_format_from_string (format_str);
  } else {
    format = GST_VIDEO_FORMAT_I420;
  }

  GST_DEBUG_OBJECT (dec, "Negotiated format: %s", gst_video_format_to_string (format));

  output_state = gst_video_decoder_set_output_state (decoder, format,
      width, height, dec->input_state);

  if (dec->output_state)
    gst_video_codec_state_unref (dec->output_state);
  dec->output_state = gst_video_codec_state_ref (output_state);

  gst_caps_unref (allowed_caps);

  return gst_video_decoder_negotiate (decoder) ? GST_FLOW_OK : GST_FLOW_NOT_NEGOTIATED;
}

static GstFlowReturn
gst_turbojpegdec_decode_rgb (GstTurboJpegDec * dec, GstMapInfo * map_info,
    GstVideoFrame * frame)
{
  GstVideoFormat format = GST_VIDEO_FRAME_FORMAT (frame);
  int tjpf = gst_turbojpegdec_get_tjpf_from_format (format);
  guint8 *dest = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);
  gint stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);
  gint width = GST_VIDEO_FRAME_WIDTH (frame);
  gint height = GST_VIDEO_FRAME_HEIGHT (frame);
  gsize expected_size;
  int ret;

  if (tjpf < 0) {
    GST_ERROR_OBJECT (dec, "Unsupported RGB format: %s",
        gst_video_format_to_string (format));
    return GST_FLOW_ERROR;
  }

  /* Basic validation - ensure buffer has reasonable size */
  if (!dest || stride <= 0) {
    GST_ERROR_OBJECT (dec, "Invalid output buffer: dest=%p, stride=%d", dest, stride);
    return GST_FLOW_ERROR;
  }

  /* Validate input buffer */
  if (!map_info->data || map_info->size == 0) {
    GST_ERROR_OBJECT (dec, "Invalid input buffer");
    return GST_FLOW_ERROR;
  }

  GST_LOG_OBJECT (dec, "Decoding RGB: %dx%d, stride=%d, format=%s, tjpf=%d",
      width, height, stride, gst_video_format_to_string (format), tjpf);

  g_mutex_lock (&dec->tjInstance_lock);
  ret = tj3Decompress8 (dec->tjInstance, map_info->data, map_info->size,
      dest, stride, tjpf);
  g_mutex_unlock (&dec->tjInstance_lock);

  if (ret < 0) {
    GST_ERROR_OBJECT (dec, "TurboJPEG decompression failed: %s",
        tj3GetErrorStr (dec->tjInstance));
    dec->error_count++;
    if (dec->error_count >= dec->max_errors) {
      GST_ELEMENT_ERROR (dec, STREAM, DECODE, 
          ("Too many decode errors"), 
          ("Error count reached maximum of %d", dec->max_errors));
      return GST_FLOW_ERROR;
    }
    return GST_FLOW_ERROR;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_turbojpegdec_decode_yuv (GstTurboJpegDec * dec, GstMapInfo * map_info,
    GstVideoFrame * frame)
{
  /* For now, decode to RGB first then convert to avoid complex YUV issues */
  GstFlowReturn ret;
  GstVideoFrame temp_frame;
  GstBuffer *temp_buffer;
  GstVideoInfo temp_info;
  
  gint width = GST_VIDEO_FRAME_WIDTH (frame);
  gint height = GST_VIDEO_FRAME_HEIGHT (frame);

  /* Create temporary RGB buffer for decoding */
  gst_video_info_init (&temp_info);
  gst_video_info_set_format (&temp_info, GST_VIDEO_FORMAT_RGB, width, height);
  
  temp_buffer = gst_buffer_new_allocate (NULL, temp_info.size, NULL);
  if (!temp_buffer) {
    GST_ERROR_OBJECT (dec, "Failed to allocate temporary RGB buffer");
    return GST_FLOW_ERROR;
  }

  if (!gst_video_frame_map (&temp_frame, &temp_info, temp_buffer, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (dec, "Failed to map temporary RGB buffer");
    gst_buffer_unref (temp_buffer);
    return GST_FLOW_ERROR;
  }

  /* Decode JPEG to RGB */
  ret = gst_turbojpegdec_decode_rgb (dec, map_info, &temp_frame);
  
  if (ret == GST_FLOW_OK) {
    /* Convert RGB to I420 using GStreamer's converter */
    GstVideoConverter *converter;
    GstVideoInfo rgb_info, yuv_info;
    
    gst_video_info_init (&rgb_info);
    gst_video_info_set_format (&rgb_info, GST_VIDEO_FORMAT_RGB, width, height);
    
    gst_video_info_init (&yuv_info);  
    gst_video_info_set_format (&yuv_info, GST_VIDEO_FORMAT_I420, width, height);
    
    converter = gst_video_converter_new (&rgb_info, &yuv_info, NULL);
    if (converter) {
      gst_video_converter_frame (converter, &temp_frame, frame);
      gst_video_converter_free (converter);
    } else {
      GST_ERROR_OBJECT (dec, "Failed to create video converter");
      ret = GST_FLOW_ERROR;
    }
  }

  gst_video_frame_unmap (&temp_frame);
  gst_buffer_unref (temp_buffer);

  return ret;
}

static GstFlowReturn
gst_turbojpegdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);
  GstMapInfo map_info;
  GstFlowReturn ret = GST_FLOW_OK;
  GstVideoFrame video_frame;
  gint width, height, subsamp;
  gboolean format_changed = FALSE;

  if (!gst_buffer_map (frame->input_buffer, &map_info, GST_MAP_READ)) {
    GST_ERROR_OBJECT (dec, "Failed to map input buffer");
    return GST_FLOW_ERROR;
  }

  /* Validate input buffer */
  if (!map_info.data || map_info.size < 4) {
    GST_ERROR_OBJECT (dec, "Invalid or too small input buffer: size=%" G_GSIZE_FORMAT,
        map_info.size);
    gst_buffer_unmap (frame->input_buffer, &map_info);
    return GST_FLOW_ERROR;
  }

  /* Check for JPEG magic bytes */
  if (map_info.data[0] != 0xFF || map_info.data[1] != 0xD8) {
    GST_ERROR_OBJECT (dec, "Invalid JPEG magic bytes: 0x%02X 0x%02X",
        map_info.data[0], map_info.data[1]);
    gst_buffer_unmap (frame->input_buffer, &map_info);
    return GST_FLOW_ERROR;
  }

  g_mutex_lock (&dec->tjInstance_lock);
  if (tj3DecompressHeader (dec->tjInstance, map_info.data, map_info.size) < 0) {
    GST_ERROR_OBJECT (dec, "Failed to decompress JPEG header: %s",
        tj3GetErrorStr (dec->tjInstance));
    g_mutex_unlock (&dec->tjInstance_lock);
    gst_buffer_unmap (frame->input_buffer, &map_info);
    return GST_FLOW_ERROR;
  }

  width = tj3Get (dec->tjInstance, TJPARAM_JPEGWIDTH);
  height = tj3Get (dec->tjInstance, TJPARAM_JPEGHEIGHT);
  subsamp = tj3Get (dec->tjInstance, TJPARAM_SUBSAMP);
  g_mutex_unlock (&dec->tjInstance_lock);

  /* Validate dimensions */
  if (width <= 0 || height <= 0 || width > 32768 || height > 32768) {
    GST_ERROR_OBJECT (dec, "Invalid JPEG dimensions: %dx%d", width, height);
    gst_buffer_unmap (frame->input_buffer, &map_info);
    return GST_FLOW_ERROR;
  }

  GST_DEBUG_OBJECT (dec, "JPEG: %dx%d, subsampling: %d", width, height, subsamp);

  if (!dec->output_state || 
      GST_VIDEO_INFO_WIDTH (&dec->output_state->info) != width ||
      GST_VIDEO_INFO_HEIGHT (&dec->output_state->info) != height) {
    format_changed = TRUE;
  }

  if (format_changed) {
    ret = gst_turbojpegdec_negotiate_format (dec, width, height, subsamp);
    if (ret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (dec, "Failed to negotiate output format");
      gst_buffer_unmap (frame->input_buffer, &map_info);
      return ret;
    }
  }

  ret = gst_video_decoder_allocate_output_frame (decoder, frame);
  if (ret != GST_FLOW_OK) {
    GST_ERROR_OBJECT (dec, "Failed to allocate output frame");
    gst_buffer_unmap (frame->input_buffer, &map_info);
    return ret;
  }

  if (!gst_video_frame_map (&video_frame, &dec->output_state->info,
          frame->output_buffer, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (dec, "Failed to map output frame");
    gst_buffer_unmap (frame->input_buffer, &map_info);
    return GST_FLOW_ERROR;
  }

  /* Decode based on output format */
  if (GST_VIDEO_FRAME_FORMAT (&video_frame) == GST_VIDEO_FORMAT_I420) {
    ret = gst_turbojpegdec_decode_yuv (dec, &map_info, &video_frame);
  } else {
    ret = gst_turbojpegdec_decode_rgb (dec, &map_info, &video_frame);
  }

  gst_video_frame_unmap (&video_frame);
  gst_buffer_unmap (frame->input_buffer, &map_info);

  if (ret == GST_FLOW_OK) {
    dec->error_count = 0;
    ret = gst_video_decoder_finish_frame (decoder, frame);
  } else {
    gst_video_decoder_drop_frame (decoder, frame);
  }

  return ret;
}

static gboolean
gst_turbojpegdec_decide_allocation (GstVideoDecoder * decoder, GstQuery * query)
{
  GstTurboJpegDec *dec = GST_TURBOJPEGDEC (decoder);
  GstBufferPool *pool;
  GstStructure *config;
  GstCaps *caps;
  guint size, min, max;
  gboolean update_pool;

  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (decoder, query))
    return FALSE;

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
    update_pool = TRUE;
  } else {
    pool = NULL;
    size = 0;
    min = max = 0;
    update_pool = FALSE;
  }

  if (!pool) {
    pool = gst_video_buffer_pool_new ();
  }

  gst_query_parse_allocation (query, &caps, NULL);

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT);

  /* Set proper alignment for SIMD operations */
  if (gst_buffer_pool_config_has_option (config, GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT)) {
    GstVideoAlignment align;
    gst_video_alignment_reset (&align);
    /* Align to 16-pixel boundaries for TurboJPEG MCU requirements */
    align.stride_align[0] = 15;  /* 16-byte alignment (15 = 16-1) */
    align.stride_align[1] = 15;  /* 16-byte alignment */
    align.stride_align[2] = 15;  /* 16-byte alignment */
    /* Add padding for MCU boundary alignment */
    align.padding_right = 15;    /* Pad width to MCU boundary */
    align.padding_bottom = 15;   /* Pad height to MCU boundary */
    gst_buffer_pool_config_set_video_alignment (config, &align);
  }

  if (!gst_buffer_pool_set_config (pool, config)) {
    GST_ERROR_OBJECT (dec, "Failed to set buffer pool configuration");
    gst_object_unref (pool);
    return FALSE;
  }

  if (update_pool) {
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  } else {
    gst_query_add_allocation_pool (query, pool, size, min, max);
  }

  gst_object_unref (pool);
  return TRUE;
}