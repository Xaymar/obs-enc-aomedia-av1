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

#include "plugin.h"
#include "av1-encoder.h"

#include "libobs/obs-module.h"

OBS_DECLARE_MODULE();
OBS_MODULE_AUTHOR("Michael Fabian Dirks");
OBS_MODULE_USE_DEFAULT_LOCALE("enc-aomedia-av1", "en-US");

obs_encoder_info g_av1encoder;

MODULE_EXPORT bool obs_module_load(void) {
	// Only register if it is actually available.
	const AvxInterface* encoder = get_aom_encoder_by_name("av1");
	if (encoder) {
		memset(&g_av1encoder, 0, sizeof(g_av1encoder));
		g_av1encoder.id = "enc-aomedia-av1";
		g_av1encoder.type = obs_encoder_type::OBS_ENCODER_VIDEO;
		g_av1encoder.codec = "av01";
		g_av1encoder.get_name = AV1Encoder::get_name;
		g_av1encoder.create = AV1Encoder::create;
		g_av1encoder.destroy = AV1Encoder::destroy;
		g_av1encoder.encode = AV1Encoder::encode;
		g_av1encoder.get_defaults = AV1Encoder::get_defaults;
		g_av1encoder.get_properties = AV1Encoder::get_properties;
		g_av1encoder.update = AV1Encoder::update;
		g_av1encoder.get_extra_data = AV1Encoder::get_extra_data;
		g_av1encoder.get_video_info = AV1Encoder::get_video_info;

		obs_register_encoder(&g_av1encoder);
	}
	return true;
}

MODULE_EXPORT void obs_module_unload(void) {}

MODULE_EXPORT const char* obs_module_name() {
	return PLUGIN_NAME;
}

MODULE_EXPORT const char* obs_module_description() {
	return PLUGIN_NAME;
}

#ifdef _WIN32
#define NOMINMAX
#define NOINOUT
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
	return TRUE;
}
#endif
