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

#pragma once
extern "C" {
#pragma warning(push)
#pragma warning(disable:4201)
#include "libobs/obs-module.h"
#include <aom/aom.h>
#include <aom/aom_encoder.h>
#pragma warning(pop)
}

class AV1Encoder {
	public:
	static const char *get_name(void *);

	static void *create(obs_data_t *, obs_encoder_t *);
	AV1Encoder(obs_data_t *, obs_encoder_t *);

	static void destroy(void *);
	~AV1Encoder();

	static void get_defaults(obs_data_t *);

	static obs_properties_t *get_properties(void *);
	void get_properties(obs_properties_t *);

	static bool update(void *, obs_data_t *);
	bool update(obs_data_t *);

	/// Called after encoder create, but before encode.
	static void get_video_info(void *, struct video_scale_info *);
	void get_video_info(struct video_scale_info *);

	/// Encode a frame.
	static bool encode(void *, struct encoder_frame *, struct encoder_packet *, bool *);
	bool encode(struct encoder_frame *, struct encoder_packet *, bool *);

	/// Retrieve encoder extra data.
	static bool get_extra_data(void *, uint8_t **, size_t *);
	bool get_extra_data(uint8_t **, size_t *);

	private:
	obs_encoder_t* m_self;

	//AV1
	aom_img_fmt m_imageFormat;
	aom_image_t m_image;
	aom_codec_enc_cfg_t m_configuration;
	aom_codec_ctx_t m_codec;

	uint32_t width, height;
	uint32_t maxencodetime;
};

// Taken from tools_common.h
typedef struct AvxInterface {
	const char *const name;
	const uint32_t fourcc;
	aom_codec_iface_t *(*const codec_interface)();
} AvxInterface;

int get_aom_encoder_count(void);
const AvxInterface *get_aom_encoder_by_index(int i);
const AvxInterface *get_aom_encoder_by_name(const char *name);
