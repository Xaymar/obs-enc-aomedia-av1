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
#include "plugin.h"

#define P_DESCRIPTION(x)			x ".Description"
#define P_TRANSLATE(x)				obs_module_text(x)
#define P_TRANSLATE_DESCRIPTION(x)		P_TRANSLATE(P_DESCRIPTION(x))

#define P_NAME					"Name"
#define P_COMMON_DEFAULT			"Common.Default"
#define P_COMMON_ENABLED			"Common.Enabled"
#define P_COMMON_DISABLED			"Common.Disabled"
#define P_COMMON_FIXED				"Common.Fixed"
#define P_COMMON_DYNAMIC			"Common.Dynamic"

#define P_USAGE					"Usage"
#define P_THREADS				"Threads"
#define P_PROFILE				"Profile"
#define P_ERRORRESILIENT			"ErrorResilient"
#define P_ERRORRESILIENT_PARTITION		"ErrorResilient.Partition"
#define P_LAGINFRAMES				"LagInFrames"

// Rate Control
#define P_RC_DROPFRAMETHRESHOLD			"RateControl.DropFrameThreshold"
/// Super & Subresolution
#define P_RC_RESIZE_MODE			"RateControl.Resize.Mode"
#define P_RC_RESIZE_NUMERATOR			"RateControl.Resize.Numerator"
#define P_RC_RESIZE_KEYFRAMENUMERATOR		"RateControl.Resize.KeyframeNumerator"
#define P_RC_SUPERRES_MODE			"RateControl.SuperRes.Mode"
#define P_RC_SUPERRES_NUMERATOR			"RateControl.SuperRes.Numerator"
#define P_RC_SUPERRES_KEYFRAMENUMERATOR		"RateControl.SuperRes.KeyframeNumerator"
/// Mode
#define P_RC_MODE				"RateControl.Mode"
#define P_RC_MODE_VBR				"RateControl.Mode.VBR"
#define P_RC_MODE_CBR				"RateControl.Mode.CBR"
#define P_RC_MODE_CQ				"RateControl.Mode.CQ"
#define P_RC_MODE_Q				"RateControl.Mode.Q"
/// VBR, CBR
#define P_RC_BITRATE				"RateControl.Bitrate"
/// Quantizer
#define P_RC_QUANTIZER_MIN			"RateControl.Quantizer.Min"
#define P_RC_QUANTIZER_MAX			"RateControl.Quantizer.Max"
/// Bitrate Tolerance
#define P_RC_UNDERSHOOT				"RateControl.Undershoot"
#define P_RC_OVERSHOOT				"RateControl.Overshoot"
/// Decoder Buffer Model
#define P_RC_BUFFER_SIZE			"RateControl.Buffer.Size"
#define P_RC_BUFFER_INITIALSIZE			"RateControl.Buffer.InitialSize"
#define P_RC_BUFFER_OPTIMALSIZE			"RateControl.Buffer.OptimalSize"
/// Keyframe Mode
#define P_KF_INTERVAL_MIN			"Keyframe.Interval.Min"
#define P_KF_INTERVAL_MAX			"Keyframe.Interval.Max"
