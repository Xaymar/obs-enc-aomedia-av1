/*
 * AV1 Encoder for Open Broadcaster Software Studio
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "av1-encoder.h"
#include "strings.h"
#include <memory.h>
#include <stdexcept>
#include <vector>
#include <chrono>

const char * AV1Encoder::get_name(void *) {
	return P_TRANSLATE(P_NAME);
}

void * AV1Encoder::create(obs_data_t *data, obs_encoder_t *encoder) {
	try {
		return new AV1Encoder(data, encoder);
	} catch (std::runtime_error ex) {
		PLOG_ERROR("Exception: %s", ex.what());
		return NULL;
	}
}

AV1Encoder::AV1Encoder(obs_data_t *data, obs_encoder_t *encoder) : m_self(encoder) {
	aom_codec_err_t res;

	#pragma region OBS Video Data
	// OBS Video Data
	uint32_t obsWidth = obs_encoder_get_width(encoder);
	uint32_t obsHeight = obs_encoder_get_height(encoder);
	video_t *obsVideoInfo = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(obsVideoInfo);
	uint32_t obsFPSnum = voi->fps_num;
	uint32_t obsFPSden = voi->fps_den;

	/// Color Format correction.
	switch (voi->format) {
		case VIDEO_FORMAT_RGBA:
			PLOG_WARNING("Color Format RGBA not supported, using BGRA instead.");
			m_imageFormat = AOM_IMG_FMT_ARGB_LE;
			break;
		case VIDEO_FORMAT_BGRX:
			PLOG_WARNING("Color Format BGRX not supported, using BGRA instead.");
			m_imageFormat = AOM_IMG_FMT_ARGB_LE;
			break;
		case VIDEO_FORMAT_BGRA:
			m_imageFormat = AOM_IMG_FMT_ARGB_LE;
			break;
		case VIDEO_FORMAT_YVYU:
			m_imageFormat = AOM_IMG_FMT_YVYU;
			break;
		case VIDEO_FORMAT_YUY2:
			m_imageFormat = AOM_IMG_FMT_YUY2;
			break;
		case VIDEO_FORMAT_UYVY:
			m_imageFormat = AOM_IMG_FMT_UYVY;
			break;
		case VIDEO_FORMAT_Y800:
			PLOG_WARNING("Color Format Y800 not supported, using I420 instead.");
			m_imageFormat = AOM_IMG_FMT_I420;
			break;
		case VIDEO_FORMAT_NV12:
			PLOG_WARNING("Color Format NV12 not supported, using I420 instead.");
			m_imageFormat = AOM_IMG_FMT_I420;
			break;
		case VIDEO_FORMAT_I420:
			m_imageFormat = AOM_IMG_FMT_I420;
			break;
		case VIDEO_FORMAT_I444:
			m_imageFormat = AOM_IMG_FMT_I444;
			break;
	}
	#pragma endregion OBS Video Data

	// Ensure correct resolution.
	if ((obsWidth % 2) != 0 || (obsHeight % 2) != 0) {
		throw std::runtime_error("Resolution (Width & Height) must be a multiple of 2.");
	}

	// Test if Codec is available (again)
	const AvxInterface* av1enc = get_aom_encoder_by_name("av1");
	if (!av1enc) {
		throw std::runtime_error("Encoder is not available.");
	}

	// Get default configuration.
	res = aom_codec_enc_config_default(av1enc->codec_interface(), &m_configuration, 0);
	if (res != AOM_CODEC_OK) {
		throw std::runtime_error("Failed to get default encoder configuration.");
	}

	update(data);
	m_configuration.g_timebase.den = obsFPSnum;
	m_configuration.g_timebase.num = obsFPSden;
	m_configuration.g_w = obsWidth;
	m_configuration.g_h = obsHeight;
	m_configuration.g_input_bit_depth = AOM_BITS_8;
	m_configuration.g_bit_depth = AOM_BITS_8;
	m_configuration.g_pass = AOM_RC_ONE_PASS;

	maxencodetime = uint32_t((double_t(obsFPSden) / double_t(obsFPSnum)) * 1000000);

	// Create frame buffer.
	if (!aom_img_alloc(&m_image, m_imageFormat, obsWidth, obsHeight, 1)) {
		throw std::runtime_error("Failed to create frame buffer.");
	} else {
		switch (voi->range) {
			case VIDEO_RANGE_PARTIAL:
				m_image.range = aom_color_range_t::AOM_CR_STUDIO_RANGE;
				break;
			default:
				m_image.range = aom_color_range_t::AOM_CR_FULL_RANGE;
				break;
		}
		switch (voi->colorspace) {
			case VIDEO_CS_601:
				m_image.cs = aom_color_space_t::AOM_CS_BT_601;
				break;
			case VIDEO_CS_DEFAULT:
			case VIDEO_CS_709:
				m_image.cs = aom_color_space_t::AOM_CS_BT_709;
				break;
		}
	}

	// Initialize
	res = aom_codec_enc_init(&m_codec, av1enc->codec_interface(), &m_configuration, 0);
	if (res != AOM_CODEC_OK) {
		std::vector<char> buf(1024);
		sprintf(buf.data(), "Failed to initialize encoder, code %d.", res);
		throw std::runtime_error(std::string(buf.data()));
	}

	PLOG_INFO("Encoder initialized.");
}

void AV1Encoder::destroy(void *ptr) {
	delete reinterpret_cast<AV1Encoder*>(ptr);
}

AV1Encoder::~AV1Encoder() {
	aom_img_free(&m_image);
}

void AV1Encoder::get_defaults(obs_data_t *data) {
	PLOG_DEBUG("%s", __FUNCTION_NAME__);

	// default settings from actual encoder.
	const AvxInterface* av1enc = get_aom_encoder_by_name("av1");
	if (!av1enc) {
		throw std::runtime_error("Encoder is not available.");
	}

	// Get default configuration.
	aom_codec_enc_cfg_t cfg;
	aom_codec_err_t res = aom_codec_enc_config_default(av1enc->codec_interface(), &cfg, 0);
	if (res != AOM_CODEC_OK) {
		throw std::runtime_error("Failed to get default encoder configuration.");
	}

	obs_data_set_default_int(data, P_USAGE, cfg.g_usage);
	obs_data_set_default_int(data, P_THREADS, cfg.g_threads);
	obs_data_set_default_int(data, P_PROFILE, cfg.g_profile);
	obs_data_set_default_int(data, P_ERRORRESILIENT, cfg.g_error_resilient);
	obs_data_set_default_int(data, P_LAGINFRAMES, cfg.g_lag_in_frames);
	obs_data_set_default_int(data, P_RC_DROPFRAMETHRESHOLD, cfg.rc_dropframe_thresh);
	obs_data_set_default_int(data, P_RC_RESIZE_MODE, cfg.rc_resize_mode);
	obs_data_set_default_int(data, P_RC_RESIZE_NUMERATOR, cfg.rc_resize_numerator);
	obs_data_set_default_int(data, P_RC_RESIZE_KEYFRAMENUMERATOR, cfg.rc_resize_kf_numerator);
	obs_data_set_default_int(data, P_RC_SUPERRES_MODE, cfg.rc_superres_mode);
	obs_data_set_default_int(data, P_RC_SUPERRES_NUMERATOR, cfg.rc_superres_numerator);
	obs_data_set_default_int(data, P_RC_SUPERRES_KEYFRAMENUMERATOR, cfg.rc_superres_kf_numerator);
	obs_data_set_default_int(data, P_RC_MODE, cfg.rc_end_usage);
	obs_data_set_default_int(data, P_RC_BITRATE, cfg.rc_target_bitrate);
	obs_data_set_default_int(data, P_RC_QUANTIZER_MIN, cfg.rc_min_quantizer);
	obs_data_set_default_int(data, P_RC_QUANTIZER_MAX, cfg.rc_max_quantizer);
	obs_data_set_default_int(data, P_RC_UNDERSHOOT, cfg.rc_undershoot_pct);
	obs_data_set_default_int(data, P_RC_OVERSHOOT, cfg.rc_overshoot_pct);
	obs_data_set_default_int(data, P_RC_BUFFER_SIZE, cfg.rc_buf_sz);
	obs_data_set_default_int(data, P_RC_BUFFER_INITIALSIZE, cfg.rc_buf_initial_sz);
	obs_data_set_default_int(data, P_RC_BUFFER_OPTIMALSIZE, cfg.rc_buf_optimal_sz);
	obs_data_set_default_int(data, P_KF_INTERVAL_MIN, cfg.kf_min_dist);
	obs_data_set_default_int(data, P_KF_INTERVAL_MAX, cfg.kf_max_dist);
}

obs_properties_t * AV1Encoder::get_properties(void *ptr) {
	obs_properties_t* pr = obs_properties_create();
	obs_property_t* p = nullptr;

	// g_usage
	p = obs_properties_add_list(pr, P_USAGE, P_TRANSLATE(P_USAGE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_DEFAULT), 0);

	// g_threads
	p = obs_properties_add_int_slider(pr, P_THREADS, P_TRANSLATE(P_THREADS),
		0, 16, 1);

	// g_profile
	p = obs_properties_add_list(pr, P_PROFILE, P_TRANSLATE(P_PROFILE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, "4:2:0 8-bit", 0);
	obs_property_list_add_int(p, "4:4:4, 4:2:2, 4:4:0 8-bit", 1);

	// g_error_resilient
	p = obs_properties_add_list(pr, P_ERRORRESILIENT, P_TRANSLATE(P_ERRORRESILIENT),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_DEFAULT), AOM_ERROR_RESILIENT_DEFAULT);
	obs_property_list_add_int(p, P_TRANSLATE(P_ERRORRESILIENT_PARTITION), AOM_ERROR_RESILIENT_PARTITIONS);

	// g_lag_in_frames
	p = obs_properties_add_int(pr, P_LAGINFRAMES, P_TRANSLATE(P_LAGINFRAMES),
		0, 1000, 1);

	// rc_dropframe_thresh
	p = obs_properties_add_int_slider(pr, P_RC_DROPFRAMETHRESHOLD, P_TRANSLATE(P_RC_DROPFRAMETHRESHOLD),
		0, 1000, 1);

	// rc_resize_mode
	p = obs_properties_add_list(pr, P_RC_RESIZE_MODE, P_TRANSLATE(P_RC_RESIZE_MODE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_DISABLED), 0);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_FIXED), 1);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_DYNAMIC), 2);

	// rc_resize_numerator
	p = obs_properties_add_int_slider(pr, P_RC_RESIZE_NUMERATOR, P_TRANSLATE(P_RC_RESIZE_NUMERATOR),
		8, 16, 1);

	// rc_resize_kf_numerator
	p = obs_properties_add_int_slider(pr, P_RC_RESIZE_KEYFRAMENUMERATOR, P_TRANSLATE(P_RC_RESIZE_KEYFRAMENUMERATOR),
		8, 16, 1);

	// rc_superres_mode
	p = obs_properties_add_list(pr, P_RC_SUPERRES_MODE, P_TRANSLATE(P_RC_SUPERRES_MODE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_DISABLED), 0);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_FIXED), 1);
	obs_property_list_add_int(p, P_TRANSLATE(P_COMMON_DYNAMIC), 2);

	// rc_superres_numerator
	p = obs_properties_add_int_slider(pr, P_RC_SUPERRES_NUMERATOR, P_TRANSLATE(P_RC_SUPERRES_NUMERATOR),
		8, 16, 1);

	// rc_superres_kf_numerator
	p = obs_properties_add_int_slider(pr, P_RC_SUPERRES_KEYFRAMENUMERATOR, P_TRANSLATE(P_RC_SUPERRES_KEYFRAMENUMERATOR),
		8, 16, 1);

	// rc_end_usage
	p = obs_properties_add_list(pr, P_RC_MODE, P_TRANSLATE(P_RC_MODE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, P_TRANSLATE(P_RC_MODE_VBR), AOM_VBR);
	obs_property_list_add_int(p, P_TRANSLATE(P_RC_MODE_CBR), AOM_CBR);
	obs_property_list_add_int(p, P_TRANSLATE(P_RC_MODE_CQ), AOM_CQ);
	obs_property_list_add_int(p, P_TRANSLATE(P_RC_MODE_Q), AOM_Q);

	// rc_target_bitrate
	p = obs_properties_add_int_slider(pr, P_RC_BITRATE, P_TRANSLATE(P_RC_BITRATE),
		1, INT32_MAX, 1);

	// rc_min_quantizer
	p = obs_properties_add_int_slider(pr, P_RC_QUANTIZER_MIN, P_TRANSLATE(P_RC_QUANTIZER_MIN),
		0, 63, 1);

	// rc_max_quantizer
	p = obs_properties_add_int_slider(pr, P_RC_QUANTIZER_MAX, P_TRANSLATE(P_RC_QUANTIZER_MAX),
		0, 63, 1);

	// rc_undershoot_pct
	p = obs_properties_add_int_slider(pr, P_RC_UNDERSHOOT, P_TRANSLATE(P_RC_UNDERSHOOT),
		0, 1000, 1);

	// rc_overshoot_pct
	p = obs_properties_add_int_slider(pr, P_RC_OVERSHOOT, P_TRANSLATE(P_RC_OVERSHOOT),
		0, 1000, 1);

	// rc_max_buffer_size
	p = obs_properties_add_int_slider(pr, P_RC_BUFFER_SIZE, P_TRANSLATE(P_RC_BUFFER_SIZE),
		1, INT32_MAX, 1);

	// rc_buffer_initial_size
	p = obs_properties_add_int_slider(pr, P_RC_BUFFER_INITIALSIZE, P_TRANSLATE(P_RC_BUFFER_INITIALSIZE),
		1, INT32_MAX, 1);

	// rc_buffer_optimal_size
	p = obs_properties_add_int_slider(pr, P_RC_BUFFER_OPTIMALSIZE, P_TRANSLATE(P_RC_BUFFER_OPTIMALSIZE),
		1, INT32_MAX, 1);

	// rc_min_quantizer
	p = obs_properties_add_int_slider(pr, P_KF_INTERVAL_MIN, P_TRANSLATE(P_KF_INTERVAL_MIN),
		0, 9999, 1);

	// rc_min_quantizer
	p = obs_properties_add_int_slider(pr, P_KF_INTERVAL_MAX, P_TRANSLATE(P_KF_INTERVAL_MAX),
		0, 9999, 1);

	// Instance specific settings.
	if (ptr != nullptr)
		reinterpret_cast<AV1Encoder*>(ptr)->get_properties(pr);

	return pr;
}

void AV1Encoder::get_properties(obs_properties_t *) {}

bool AV1Encoder::update(void *ptr, obs_data_t *data) {
	return reinterpret_cast<AV1Encoder*>(ptr)->update(data);
}

bool AV1Encoder::update(obs_data_t *data) {
	m_configuration.g_usage = (unsigned int)obs_data_get_int(data, P_USAGE);
	m_configuration.g_threads = (unsigned int)obs_data_get_int(data, P_THREADS);
	m_configuration.g_profile = (unsigned int)obs_data_get_int(data, P_PROFILE);
	m_configuration.g_error_resilient = (unsigned int)obs_data_get_int(data, P_ERRORRESILIENT);
	m_configuration.g_lag_in_frames = (unsigned int)obs_data_get_int(data, P_LAGINFRAMES);
	m_configuration.rc_dropframe_thresh = (unsigned int)obs_data_get_int(data, P_RC_DROPFRAMETHRESHOLD);
	m_configuration.rc_resize_mode = (unsigned int)obs_data_get_int(data, P_RC_RESIZE_MODE);
	m_configuration.rc_resize_numerator = (unsigned int)obs_data_get_int(data, P_RC_RESIZE_NUMERATOR);
	m_configuration.rc_resize_kf_numerator = (unsigned int)obs_data_get_int(data, P_RC_RESIZE_KEYFRAMENUMERATOR);
	m_configuration.rc_superres_mode = (unsigned int)obs_data_get_int(data, P_RC_SUPERRES_MODE);
	m_configuration.rc_superres_numerator = (unsigned int)obs_data_get_int(data, P_RC_SUPERRES_NUMERATOR);
	m_configuration.rc_superres_kf_numerator = (unsigned int)obs_data_get_int(data, P_RC_SUPERRES_KEYFRAMENUMERATOR);
	m_configuration.rc_end_usage = (aom_rc_mode)obs_data_get_int(data, P_RC_MODE);
	m_configuration.rc_target_bitrate = (unsigned int)obs_data_get_int(data, P_RC_BITRATE);
	m_configuration.rc_min_quantizer = (unsigned int)obs_data_get_int(data, P_RC_QUANTIZER_MIN);
	m_configuration.rc_max_quantizer = (unsigned int)obs_data_get_int(data, P_RC_QUANTIZER_MAX);
	m_configuration.rc_undershoot_pct = (unsigned int)obs_data_get_int(data, P_RC_UNDERSHOOT);
	m_configuration.rc_overshoot_pct = (unsigned int)obs_data_get_int(data, P_RC_OVERSHOOT);
	m_configuration.rc_buf_sz = (unsigned int)obs_data_get_int(data, P_RC_BUFFER_SIZE);
	m_configuration.rc_buf_initial_sz = (unsigned int)obs_data_get_int(data, P_RC_BUFFER_INITIALSIZE);
	m_configuration.rc_buf_optimal_sz = (unsigned int)obs_data_get_int(data, P_RC_BUFFER_OPTIMALSIZE);
	m_configuration.kf_min_dist = (unsigned int)obs_data_get_int(data, P_KF_INTERVAL_MIN);
	m_configuration.kf_max_dist = (unsigned int)obs_data_get_int(data, P_KF_INTERVAL_MAX);

	return false;
}

bool AV1Encoder::encode(void *ptr, struct encoder_frame *frame, struct encoder_packet *packet, bool *recframe) {
	return reinterpret_cast<AV1Encoder*>(ptr)->encode(frame, packet, recframe);
}

bool AV1Encoder::encode(struct encoder_frame *frame, struct encoder_packet *packet, bool *received_frame) {
	if (m_imageFormat == AOM_IMG_FMT_ARGB_LE) {
		std::memcpy(m_image.planes[AOM_PLANE_PACKED], frame->data[0], frame->linesize[0] * m_image.h);
	} else if (m_imageFormat == AOM_IMG_FMT_I444) {
		std::memcpy(m_image.planes[AOM_PLANE_Y], frame->data[0], frame->linesize[0] * m_image.h);
		std::memcpy(m_image.planes[AOM_PLANE_U], frame->data[1], frame->linesize[1] * m_image.h);
		std::memcpy(m_image.planes[AOM_PLANE_V], frame->data[2], frame->linesize[2] * m_image.h);
	} else if (m_imageFormat == AOM_IMG_FMT_I420) {
		std::memcpy(m_image.planes[AOM_PLANE_Y], frame->data[0], frame->linesize[0] * m_image.h);
		std::memcpy(m_image.planes[AOM_PLANE_U], frame->data[1], frame->linesize[1] * m_image.h / 2);
		std::memcpy(m_image.planes[AOM_PLANE_V], frame->data[2], frame->linesize[2] * m_image.h / 2);
	}

	// Encode
	aom_codec_err_t res = aom_codec_encode(&m_codec, &m_image, frame->pts, 1, 0, maxencodetime);
	if (res != AOM_CODEC_OK) {
		PLOG_ERROR("Encoding packet failed, code: %lld", res);
		return false;
	}

	// Get Packet
	aom_codec_iter_t iter = NULL;
	for (const aom_codec_cx_pkt_t *pkt = aom_codec_get_cx_data(&m_codec, &iter); pkt != NULL; pkt = aom_codec_get_cx_data(&m_codec, &iter)) {
		if (*received_frame == true)
			break;

		if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
			packet->pts = pkt->data.frame.pts;
			packet->size = pkt->data.frame.sz;
			packet->keyframe = pkt->data.frame.flags & AOM_FRAME_IS_KEY;
			packet->data = (uint8_t*)pkt->data.frame.buf;
			packet->dts = packet->pts - pkt->data.frame.duration;
			*received_frame = true;
		} // ToDo: determine live two-pass encoding, technically possible.
	}

	if (!*received_frame) {
		PLOG_WARNING("No frame for encode call.");
	} else {
		PLOG_DEBUG("Packet (PTS: %lld, Size: %lld, Keyframe: %s)",
			packet->pts,
			packet->size,
			packet->keyframe ? "y" : "n");
	}

	return true;
}

bool AV1Encoder::get_extra_data(void *ptr, uint8_t **data, size_t *size) {
	return reinterpret_cast<AV1Encoder*>(ptr)->get_extra_data(data, size);
}

bool AV1Encoder::get_extra_data(uint8_t **data, size_t *size) {
	aom_fixed_buf_t* buf = aom_codec_get_global_headers(&m_codec);
	if (!buf) {
		return false;
	}

	*data = (uint8_t*)buf->buf;
	*size = buf->sz;

	return true;
}

void AV1Encoder::get_video_info(void *ptr, struct video_scale_info *vsi) {
	return reinterpret_cast<AV1Encoder*>(ptr)->get_video_info(vsi);
}

void AV1Encoder::get_video_info(struct video_scale_info *vsi) {
	switch (m_imageFormat) {
		case AOM_IMG_FMT_ARGB_LE:
			vsi->format = VIDEO_FORMAT_BGRA;
			break;
		case AOM_IMG_FMT_YVYU:
			vsi->format = VIDEO_FORMAT_YVYU;
			break;
		case AOM_IMG_FMT_YUY2:
			vsi->format = VIDEO_FORMAT_YUY2;
			break;
		case AOM_IMG_FMT_UYVY:
			vsi->format = VIDEO_FORMAT_UYVY;
			break;
		case AOM_IMG_FMT_I420:
			vsi->format = VIDEO_FORMAT_I420;
			break;
		case AOM_IMG_FMT_I444:
			vsi->format = VIDEO_FORMAT_I444;
			break;
	}
}

// Taken from tools_common.c
#include "aom/aomcx.h"
#define AV1_FOURCC 0x31305641

static const AvxInterface aom_encoders[] = {
	{ "av1", AV1_FOURCC, &aom_codec_av1_cx },
};

int get_aom_encoder_count(void) {
	return sizeof(aom_encoders) / sizeof(aom_encoders[0]);
}

const AvxInterface *get_aom_encoder_by_index(int i) {
	return &aom_encoders[i];
}

const AvxInterface *get_aom_encoder_by_name(const char *name) {
	int i;

	for (i = 0; i < get_aom_encoder_count(); ++i) {
		const AvxInterface *encoder = get_aom_encoder_by_index(i);
		if (strcmp(encoder->name, name) == 0) return encoder;
	}

	return NULL;
}
