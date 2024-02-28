/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Authors: Romain Bouqueau
 *			Copyright (c) Motion Spell 2024
 *					All rights reserved
 *
 *  This file is part of GPAC / SCTE35 property decode filter
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <gpac/filters.h>

typedef struct {
	GF_FilterPid *ipid;
	GF_FilterPid *opid;
	u64 last_cts;
} SCTE35DecCtx;

GF_Err scte35dec_configure_pid(GF_Filter *filter, GF_FilterPid *pid, Bool is_remove)
{
	SCTE35DecCtx *ctx = gf_filter_get_udta(filter);

	if (is_remove) {
		GF_FilterPid *out_pid = gf_filter_pid_get_udta(pid);
		if (out_pid == ctx->opid)
			ctx->opid = NULL;
		if (out_pid)
			gf_filter_pid_remove(out_pid);
		return GF_OK;
	}

	if (!gf_filter_pid_check_caps(pid))
		return GF_NOT_SUPPORTED;

	if (!ctx->opid) {
		ctx->opid = gf_filter_pid_new(filter);
		if (!ctx->opid) return GF_OUT_OF_MEM;
		gf_filter_pid_set_udta(pid, ctx->opid);
	}
	ctx->ipid = pid;
	gf_filter_pid_set_framing_mode(pid, GF_TRUE);

	//copy properties at init or reconfig
	gf_filter_pid_copy_properties(ctx->opid, pid);
	gf_filter_pid_set_property(ctx->opid, GF_PROP_PID_STREAM_TYPE, &PROP_UINT(GF_STREAM_METADATA) );
	gf_filter_pid_set_property(ctx->opid, GF_PROP_PID_CODECID, &PROP_UINT(GF_CODECID_SCTE35) );
	gf_filter_pid_set_property(ctx->opid, GF_PROP_PID_SPARSE, &PROP_BOOL(GF_TRUE) );

	return GF_OK;
}

GF_Err scte35dec_process(GF_Filter *filter)
{
	SCTE35DecCtx *ctx = gf_filter_get_udta(filter);
	GF_FilterPacket *pck = gf_filter_pid_get_packet(ctx->ipid);
	if (!pck) {
		if (gf_filter_pid_is_eos(ctx->ipid)) {
			gf_filter_pid_set_eos(ctx->opid);
			return GF_EOS;
		}
		return GF_OK;
	}

	GF_FilterPacket *pck_dst = NULL;
	const GF_PropertyValue *emsg = gf_filter_pck_get_property_str(pck, "scte35");
	if (emsg && (emsg->type == GF_PROP_DATA) && emsg->value.data.ptr) {
		u8 *output = NULL;
		pck_dst = gf_filter_pck_new_alloc(ctx->opid, emsg->value.data.size, &output);
		if (!pck_dst) return GF_OUT_OF_MEM;

		memcpy(output, emsg->value.data.ptr, emsg->value.data.size);
		gf_filter_pck_set_framing(pck_dst, GF_TRUE, GF_TRUE);
		gf_filter_pck_set_sap(pck_dst, GF_FILTER_SAP_1);
		//TODO: compute duration when available in the SCTE35 payload
		gf_filter_pck_set_duration(pck_dst, (u32)(gf_filter_pck_get_cts(pck) - ctx->last_cts));
		ctx->last_cts = gf_filter_pck_get_cts(pck);
		gf_filter_pck_set_cts(pck_dst, ctx->last_cts);
		gf_filter_pck_send(pck_dst);
	}

	gf_filter_pid_drop_packet(ctx->ipid);
	return GF_OK;
}

static const GF_FilterCapability SCTE35DecCaps[] =
{
	CAP_UINT(GF_CAPS_INPUT, GF_PROP_PID_STREAM_TYPE, GF_STREAM_VISUAL),
	CAP_UINT(GF_CAPS_INPUT, GF_PROP_PID_STREAM_TYPE, GF_STREAM_AUDIO),
	CAP_BOOL(GF_CAPS_INPUT_EXCLUDED, GF_PROP_PID_UNFRAMED, GF_TRUE),
	CAP_UINT(GF_CAPS_INPUT_EXCLUDED, GF_PROP_PID_CODECID, GF_CODECID_NONE),
	CAP_BOOL(GF_CAPS_OUTPUT_STATIC_EXCLUDED, GF_PROP_PID_UNFRAMED, GF_TRUE),
};

GF_FilterRegister SCTE35DecRegister = {
	.name = "scte35dec",
	GF_FS_SET_DESCRIPTION("SCTE35 decoder")
	GF_FS_SET_HELP("This filter extracts SCTE-35 markers attached as properties to audio and video packets.\n")
	.private_size = sizeof(SCTE35DecCtx),
	.flags = GF_FS_REG_EXPLICIT_ONLY,
	SETCAPS(SCTE35DecCaps),
	.process = scte35dec_process,
	.configure_pid = scte35dec_configure_pid,
};

const GF_FilterRegister *scte35dec_register(GF_FilterSession *session)
{
	return &SCTE35DecRegister;
}
