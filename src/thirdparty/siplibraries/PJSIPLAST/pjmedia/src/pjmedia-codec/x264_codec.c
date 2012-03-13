/* $Id: x264_codec.c 1000 2012-02-20 04:20:47Z popov $ */
/* 
* Copyright (C) 2010-2011 Teluu Inc. (http://www.teluu.com)
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
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/
#include <pjmedia-codec/x264_codec.h>
//#include <pjmedia-codec/h263_packetizer.h>
#include <pjmedia-codec/h264_packetizer.h>
#include <pjmedia/errno.h>
#include <pjmedia/vid_codec_util.h>
#include <pj/assert.h>
#include <pj/list.h>
#include <pj/log.h>
#include <pj/math.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/os.h>


#include "../pjmedia/ffmpeg_util.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


/*
* Only build this file if PJMEDIA_HAS_X264_CODEC != 0 and 
* PJMEDIA_HAS_VIDEO != 0
*/
#if defined(PJMEDIA_HAS_X264_CODEC) && PJMEDIA_HAS_X264_CODEC != 0 && defined(PJMEDIA_HAS_VIDEO) && (PJMEDIA_HAS_VIDEO != 0)

#define THIS_FILE   "x264_codec.c"


#include "stdint.h"
#include "x264.h"


/* Prototypes for x264 codecs factory */
static pj_status_t x264_test_alloc( pjmedia_vid_codec_factory *factory,    const pjmedia_vid_codec_info *id );
static pj_status_t x264_default_attr( pjmedia_vid_codec_factory *factory,  const pjmedia_vid_codec_info *info, pjmedia_vid_codec_param *attr );
static pj_status_t x264_enum_codecs( pjmedia_vid_codec_factory *factory,   unsigned *count, pjmedia_vid_codec_info codecs[]);
static pj_status_t x264_alloc_codec( pjmedia_vid_codec_factory *factory,   const pjmedia_vid_codec_info *info, pjmedia_vid_codec **p_codec);
static pj_status_t x264_dealloc_codec( pjmedia_vid_codec_factory *factory, pjmedia_vid_codec *codec );

/* Prototypes for x264 codecs implementation. */
static pj_status_t  x264_codec_init( pjmedia_vid_codec *codec, pj_pool_t *pool );
static pj_status_t  x264_codec_open( pjmedia_vid_codec *codec, pjmedia_vid_codec_param *attr );
static pj_status_t  x264_codec_close( pjmedia_vid_codec *codec );
static pj_status_t  x264_codec_modify(pjmedia_vid_codec *codec, const pjmedia_vid_codec_param *attr );
static pj_status_t  x264_codec_get_param(pjmedia_vid_codec *codec, pjmedia_vid_codec_param *param);
static pj_status_t x264_codec_encode_begin(pjmedia_vid_codec *codec,
																						 const pjmedia_vid_encode_opt *opt,
																						 const pjmedia_frame *input,
																						 unsigned out_size,
																						 pjmedia_frame *output,
																						 pj_bool_t *has_more);
static pj_status_t x264_codec_encode_more(pjmedia_vid_codec *codec,
																						unsigned out_size,
																						pjmedia_frame *output,
																						pj_bool_t *has_more);
static pj_status_t x264_codec_decode( pjmedia_vid_codec *codec,
																			 pj_size_t pkt_count,
																			 pjmedia_frame packets[],
																			 unsigned out_size,
																			 pjmedia_frame *output);

/* Definition for x264 codecs operations. */
static pjmedia_vid_codec_op x264_op = 
{
	&x264_codec_init,
	&x264_codec_open,
	&x264_codec_close,
	&x264_codec_modify,
	&x264_codec_get_param,
	&x264_codec_encode_begin,
	&x264_codec_encode_more,
	NULL, //&x264_codec_decode,
	NULL
};

/* Definition for FFMPEG codecs factory operations. */
static pjmedia_vid_codec_factory_op x264_factory_op =
{
	&x264_test_alloc,
	&x264_default_attr,
	&x264_enum_codecs,
	&x264_alloc_codec,
	&x264_dealloc_codec
};


/* x264 codec factory */
static struct x264_factory
{
	pjmedia_vid_codec_factory    base;
	pjmedia_vid_codec_mgr	*mgr;
	pj_pool_factory             *pf;
	pj_pool_t		        *pool;
	pj_mutex_t		        *mutex;
} x264_factory;


typedef struct x264_codec_desc x264_codec_desc;


/* x264 codecs private data. */
typedef struct x264_private
{
	const x264_codec_desc	    *desc;
	pjmedia_vid_codec_param	     param;	/**< Codec param	    */
	pj_pool_t			    *pool;	/**< Pool for each instance */

	/* Format info and apply format param */
	const pjmedia_video_format_info *enc_vfi;
	pjmedia_video_apply_fmt_param    enc_vafp;
	const pjmedia_video_format_info *dec_vfi;
	pjmedia_video_apply_fmt_param    dec_vafp;

	/* Buffers, only needed for multi-packets */
	pj_bool_t			     whole;
	void							*enc_buf;
	unsigned			     enc_buf_size;
	pj_bool_t			     enc_buf_is_keyframe;
	unsigned			     enc_frame_len;
	unsigned					 enc_processed;
	void							*dec_buf;
	unsigned			     dec_buf_size;
	pj_timestamp		   last_dec_keyframe_ts; 

	x264_picture_t			pic_in;

	/* The x264 codec states. */
	//struct SwsContext			*convertCtx;
	x264_t								*enc;
	x264_param_t					*enc_ctx;
	//AVCodec			    *enc;
	//AVCodec			    *dec;
	//AVCodecContext		    *enc_ctx;
	//AVCodecContext		    *dec_ctx;

	/* The ffmpeg decoder cannot set the output format, so format conversion
	* may be needed for post-decoding.
	*/
	enum PixelFormat		     expected_dec_fmt;
	/**< Expected output format of 
	ffmpeg decoder	    */

	void			    *data;	/**< Codec specific data    */		    
} x264_private;


/* Shortcuts for packetize & unpacketize function declaration,
* as it has long params and is reused many times!
*/
#define FUNC_PACKETIZE(name) \
	pj_status_t(name)(x264_private *x264, pj_uint8_t *bits, \
	pj_size_t bits_len, unsigned *bits_pos, \
	const pj_uint8_t **payload, pj_size_t *payload_len)

#define FUNC_UNPACKETIZE(name) \
	pj_status_t(name)(x264_private *x264, const pj_uint8_t *payload, \
	pj_size_t payload_len, pj_uint8_t *bits, \
	pj_size_t bits_len, unsigned *bits_pos)

#define FUNC_FMT_MATCH(name) \
	pj_status_t(name)(pj_pool_t *pool, \
	pjmedia_sdp_media *offer, unsigned o_fmt_idx, \
	pjmedia_sdp_media *answer, unsigned a_fmt_idx, \
	unsigned option)


/* Type definition of codec specific functions */
typedef FUNC_PACKETIZE(*func_packetize);
typedef FUNC_UNPACKETIZE(*func_unpacketize);
typedef pj_status_t (*func_preopen)	(x264_private *x264);
typedef pj_status_t (*func_postopen)	(x264_private *x264);
typedef FUNC_FMT_MATCH(*func_sdp_fmt_match);


/* x264 codec info */
struct x264_codec_desc
{
	/* Predefined info */
	pjmedia_vid_codec_info       info;
	//pjmedia_format_id		 base_fmt_id;	/**< Some codecs may be exactly
	//																	same or compatible with
	//																	another codec, base format
	//																	will tell the initializer
	//																	to copy this codec desc
	//																	from its base format   */

	pjmedia_rect_size            size;
	pjmedia_ratio                fps;

	pj_uint32_t			 avg_bps;
	pj_uint32_t			 max_bps;
	func_packetize		 packetize;
	func_unpacketize	 unpacketize;
	func_preopen		 preopen;
	func_preopen		 postopen;
	func_sdp_fmt_match		 sdp_fmt_match;
	pjmedia_codec_fmtp		 dec_fmtp;

	/* Init time defined info */
	pj_bool_t			 enabled;
	//x264_t								*enc;
	//AVCodec                     *enc;
	//AVCodec                     *dec;
};


/* H264 constants */
#define PROFILE_H264_BASELINE		66
#define PROFILE_H264_MAIN		77

/* Codec specific functions */
//#if PJMEDIA_HAS_FFMPEG_CODEC_H264
static pj_status_t h264_preopen(x264_private *x264);
static pj_status_t h264_postopen(x264_private *x264);
static FUNC_PACKETIZE(h264_packetize);
static FUNC_UNPACKETIZE(h264_unpacketize);
//#endif



/* Internal codec info */
static x264_codec_desc codec_desc[] =
{
	//{
	//	{PJMEDIA_FORMAT_H264, PJMEDIA_RTP_PT_H264, {"H264",4},
	//	{"Constrained Baseline (level=30, pack=1)", 39}},
	//	0,	256000,    512000,
	//	&h264_packetize, &h264_unpacketize, &h264_preopen, &h264_postopen,
	//	&pjmedia_vid_codec_h264_match_sdp,
	//	/* Leading space for better compatibility (strange indeed!) */
	//	{2, { {{"profile-level-id",16},    {"42e01e",6}}, 
	//	{{" packetization-mode",19},  {"1",1}}, } },
	//}
		{
		{PJMEDIA_FORMAT_H264, PJMEDIA_RTP_PT_H264, {"H264",4},
		{"Constrained Baseline (level=30, pack=1)", 39}},
		{720, 480},	{30, 1}, 256000,    512000,
		&h264_packetize, &h264_unpacketize, &h264_preopen, &h264_postopen,
		&pjmedia_vid_codec_h264_match_sdp,
		/* Leading space for better compatibility (strange indeed!) */
		{2, { {{"profile-level-id",16},    {"42e01e",6}}, 
		{{" packetization-mode",19},  {"1",1}}, } },
	}
};

//#if PJMEDIA_HAS_FFMPEG_CODEC_H264

typedef struct h264_data
{
	pjmedia_vid_codec_h264_fmtp	 fmtp;
	pjmedia_h264_packetizer	*pktz;
} h264_data;


static pj_status_t h264_preopen(x264_private *x264)
{
	h264_data *data;
	pjmedia_h264_packetizer_cfg pktz_cfg;
	pj_status_t status;
	pj_mutex_t *x264_mutex;


	data = PJ_POOL_ZALLOC_T(x264->pool, h264_data);
	x264->data = data;


	x264_mutex = ((struct x264_factory*)x264->pool->factory)->mutex;


	/* Parse remote fmtp */
	status = pjmedia_vid_codec_h264_parse_fmtp(&x264->param.enc_fmtp, &data->fmtp);
	if (status != PJ_SUCCESS)
		return status;

	/* Create packetizer */
	pktz_cfg.mtu = x264->param.enc_mtu;

#if 0
	if (data->fmtp.packetization_mode == 0)
		pktz_cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL;
	else if (data->fmtp.packetization_mode == 1)
		pktz_cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
	else
		return PJ_ENOTSUP;
#else
	if (data->fmtp.packetization_mode != PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL &&
		  data->fmtp.packetization_mode != PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED)
	{
		return PJ_ENOTSUP;
	}
	/* Better always send in single NAL mode for better compatibility */
	pktz_cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL;
#endif

	status = pjmedia_h264_packetizer_create(x264->pool, &pktz_cfg, &data->pktz);
	if (status != PJ_SUCCESS)
		return status;

	/* Apply SDP fmtp to format in codec param */
	if (!x264->param.ignore_fmtp)
	{
		status = pjmedia_vid_codec_h264_apply_fmtp(&x264->param);
		if (status != PJ_SUCCESS)
			return status;
	}

	if (x264->param.dir & PJMEDIA_DIR_ENCODING)
	{
		//////////pjmedia_video_format_detail *vfd;
		const char *profile = NULL;
		//AVCodecContext *ctx = x264->enc_ctx;
		x264_param_t *ctx = x264->enc_ctx;
		
		////////// Ïàðàìåòðû ïî óìîë÷àíèþ
		//////////x264_param_default( ctx );

		//////////vfd = pjmedia_format_get_video_format_detail(&x264->param.enc_fmt, PJ_TRUE);

		///////////* Override generic params after applying SDP fmtp */
		//////////ctx->i_width = vfd->size.w;
		//////////ctx->i_height = vfd->size.h;
		//////////ctx->i_timebase_num = vfd->fps.denum;
		//////////ctx->i_timebase_den = vfd->fps.num;

		

		/* Apply profile. */
		//ctx->profile  = data->fmtp.profile_idc;
		//switch (ctx->profile)
		switch (data->fmtp.profile_idc)
		{
			case PROFILE_H264_BASELINE:
				profile = "baseline";
				break;
			case PROFILE_H264_MAIN:
				profile = "main";
				break;
			default:
				break;
		}

		/* Limit NAL unit size as we prefer single NAL unit packetization */
		ctx->i_slice_max_size = 1300;//x264->param.enc_mtu;
		ctx->b_sliced_threads = 0;
		//param.i_slice_max_size = 1300;
		ctx->i_bframe = 0;
		ctx->i_threads = 0;
		ctx->i_fps_num = x264->desc->fps.num; //15; 
		ctx->i_fps_den = 1;//x264->desc->fps.denum; //1; 
		//ctx->i_keyint_max = 12;
		// Intra refres:
		ctx->i_keyint_max = ctx->i_fps_num / 2;
		ctx->b_intra_refresh = 1;
		//Rate control:
		ctx->rc.i_rc_method = X264_RC_CRF;
		ctx->rc.f_rf_constant = 25;
		ctx->rc.f_rf_constant_max = 30;
		//For streaming:
		//param.b_repeat_headers = 1;
		//param.b_annexb = 1;
		//ctx->analyse.b_dct_decimate = 0;
		//ctx->analyse.b_fast_pskip = 0;
		//ctx->analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;

		ctx->b_repeat_headers = 1; 
		ctx->b_annexb = 1; 


		x264_param_apply_profile(ctx, "baseline"); // ÏÎÏÎÂ
		//x264_param_apply_profile( ctx, profile );
		//ctx->i_level_idc = 13;

		//x264_encoder_reconfig( x264->enc, ctx );
		//enc = x264_encoder_open(ctx);


		/* Apply profile level. */
		//ctx->level    = data->fmtp.level;

		//x264->convertCtx = sws_getContext(ctx->i_width, ctx->i_height, PIX_FMT_RGB24, ctx->i_width, ctx->i_height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		//x264_picture_alloc(&x264->pic_in, X264_CSP_YV12, ctx->i_width, ctx->i_height);
		//pj_mutex_lock(x264_mutex);
		x264_picture_alloc(&x264->pic_in, X264_CSP_I420, ctx->i_width, ctx->i_height);
		//pj_mutex_unlock(x264_mutex);
		//x264_picture_alloc(&pic_in_test, X264_CSP_I420, ctx->i_width, ctx->i_height);
		//x264_picture_clean(&pic_in_test);
		
	}

	if (x264->param.dir & PJMEDIA_DIR_DECODING)
	{
		// x264 direct not support decoding. Decoding through ffmpeg

		//AVCodecContext *ctx = ff->dec_ctx;

		///* Apply the "sprop-parameter-sets" fmtp from remote SDP to
		//* extradata of ffmpeg codec context.
		//*/
		//if (data->fmtp.sprop_param_sets_len)
		//{
		//	ctx->extradata_size = data->fmtp.sprop_param_sets_len;
		//	ctx->extradata = data->fmtp.sprop_param_sets;
		//}
	}

	return PJ_SUCCESS;
}

static pj_status_t h264_postopen(x264_private *x264)
{
	h264_data *data = (h264_data*)x264->data;
	PJ_UNUSED_ARG(data);
	return PJ_SUCCESS;
}

static FUNC_PACKETIZE(h264_packetize)
{
	h264_data *data = (h264_data*)x264->data;
	return pjmedia_h264_packetize(data->pktz, bits, bits_len, bits_pos, payload, payload_len);
}

static FUNC_UNPACKETIZE(h264_unpacketize)
{
	h264_data *data = (h264_data*)x264->data;
	return pjmedia_h264_unpacketize(data->pktz, payload, payload_len, bits, bits_len, bits_pos);
}

//#endif /* PJMEDIA_HAS_FFMPEG_CODEC_H264 */



static const x264_codec_desc* find_codec_desc_by_info(const pjmedia_vid_codec_info *info)
{
	int i;

	for (i=0; i<PJ_ARRAY_SIZE(codec_desc); ++i)
	{
		x264_codec_desc *desc = &codec_desc[i];

		//if (desc->enabled &&
		//	(desc->info.fmt_id == info->fmt_id) &&
		//	((desc->info.dir & info->dir) == info->dir) &&
		//	(desc->info.pt == info->pt) &&
		//	(desc->info.packings & info->packings))
		//{
		//	return desc;
		//}
		if (desc->enabled &&
			(desc->info.fmt_id == info->fmt_id) &&
			(desc->info.pt == info->pt) &&
			(desc->info.packings & info->packings))
		{
			//pjmedia_dir codec_not_can = info->dir ^ desc->info.dir;
			return desc;
		}
	}

	return NULL;
}


//static int find_codec_idx_by_fmt_id(pjmedia_format_id fmt_id)
//{
//	int i;
//	for (i=0; i<PJ_ARRAY_SIZE(codec_desc); ++i)
//	{
//		if (codec_desc[i].info.fmt_id == fmt_id)
//			return i;
//	}
//
//	return -1;
//}


/*
* Initialize and register x264 codec factory to pjmedia endpoint.
*/
PJ_DEF(pj_status_t) pjmedia_codec_x264_init(pjmedia_vid_codec_mgr *mgr, pj_pool_factory *pf)
{
	pj_pool_t *pool;
	pj_status_t status;
	unsigned i;
	x264_codec_desc *desc;

	if (x264_factory.pool != NULL)
	{
		/* Already initialized. */
		return PJ_SUCCESS;
	}

	if (!mgr)
		mgr = pjmedia_vid_codec_mgr_instance();
	PJ_ASSERT_RETURN(mgr, PJ_EINVAL);

	/* Create x264 codec factory. */
	x264_factory.base.op = &x264_factory_op;
	x264_factory.base.factory_data = NULL;
	x264_factory.mgr = mgr;
	x264_factory.pf = pf;

	pool = pj_pool_create(pf, "x264 codec factory", 256, 256, NULL);
	if (!pool)
		return PJ_ENOMEM;

	/* Create mutex. */
	status = pj_mutex_create_simple(pool, "x264 codec factory", &x264_factory.mutex);
	if (status != PJ_SUCCESS)
		goto on_error;


	desc = &codec_desc[0];// x264 and only one

	desc->info.dec_fmt_id_cnt = 3;

	desc->info.dec_fmt_id[0] = PJMEDIA_FORMAT_I420;
	desc->info.dec_fmt_id[0] = PJMEDIA_FORMAT_I420JPEG;
	desc->info.dec_fmt_id[0] = PJMEDIA_FORMAT_YV12;

  desc->info.fps_cnt = 0;

	/* Get raw/decoded format ids in the encoder */
	//if (c->pix_fmts && c->encode)
	//{
	//	pjmedia_format_id raw_fmt[PJMEDIA_VID_CODEC_MAX_DEC_FMT_CNT];
	//	unsigned raw_fmt_cnt = 0;
	//	unsigned raw_fmt_cnt_should_be = 0;
	//	const enum PixelFormat *p = c->pix_fmts;

	//	for(;(p && *p != -1) && (raw_fmt_cnt < PJMEDIA_VID_CODEC_MAX_DEC_FMT_CNT); ++p)
	//	{
	//		pjmedia_format_id fmt_id;

	//		raw_fmt_cnt_should_be++;
	//		status = PixelFormat_to_pjmedia_format_id(*p, &fmt_id);
	//		if (status != PJ_SUCCESS)
	//		{
	//			PJ_LOG(6, (THIS_FILE, "Unrecognized ffmpeg pixel "
	//				"format %d", *p));
	//			continue;
	//		}

	//		//raw_fmt[raw_fmt_cnt++] = fmt_id;
	//		/* Disable some formats due to H.264 error:
	//		* x264 [error]: baseline profile doesn't support 4:4:4
	//		*/
	//		if (desc->info.pt != PJMEDIA_RTP_PT_H264 ||
	//			fmt_id != PJMEDIA_FORMAT_RGB24)
	//		{
	//			raw_fmt[raw_fmt_cnt++] = fmt_id;
	//		}
	//	}

	//	if (raw_fmt_cnt == 0)
	//	{
	//		PJ_LOG(5, (THIS_FILE, "No recognized raw format "
	//			"for codec [%s/%s], codec ignored",
	//			c->name, c->long_name));
	//		/* Skip this encoder */
	//		continue;
	//	}

	//	if (raw_fmt_cnt < raw_fmt_cnt_should_be)
	//	{
	//		PJ_LOG(6, (THIS_FILE, "Codec [%s/%s] have %d raw formats, "
	//			"recognized only %d raw formats",
	//			c->name, c->long_name,
	//			raw_fmt_cnt_should_be, raw_fmt_cnt));
	//	}

	//	desc->info.dec_fmt_id_cnt = raw_fmt_cnt;
	//	pj_memcpy(desc->info.dec_fmt_id, raw_fmt, sizeof(raw_fmt[0])*raw_fmt_cnt);
	//}

	/* Get supported framerates */
	//if (c->supported_framerates)
	//{
	//	const AVRational *fr = c->supported_framerates;
	//	while ((fr->num != 0 || fr->den != 0) && desc->info.fps_cnt < PJMEDIA_VID_CODEC_MAX_FPS_CNT)
	//	{
	//		desc->info.fps[desc->info.fps_cnt].num = fr->num;
	//		desc->info.fps[desc->info.fps_cnt].denum = fr->den;
	//		++desc->info.fps_cnt;
	//		++fr;
	//	}
	//}

	{
		//x264_param_t param;
		//x264_param_default( &param );
		//x264_param_default_preset(&param, "veryfast", "zerolatency");
		//	// Ïàðàìåòðû ïî óìîë÷àíèþ
		//param.i_width = 640;
		//param.i_height = 480;


		/* Get x264 encoder instance */
		desc->info.dir |= PJMEDIA_DIR_ENCODING;
		//desc->enc = x264_encoder_open(&param);
		desc->enabled = PJ_TRUE;
	}


	/* Normalize default value of clock rate */
	if (desc->info.clock_rate == 0)
		desc->info.clock_rate = 90000;

	/* Set supported packings */
	desc->info.packings |= PJMEDIA_VID_PACKING_WHOLE;
	if (desc->packetize && desc->unpacketize)
		desc->info.packings |= PJMEDIA_VID_PACKING_PACKETS;

	

	/* Registering format match for SDP negotiation.*/
	{
		/* Registering format match for SDP negotiation */
		if (desc->sdp_fmt_match)
		{
			status = pjmedia_sdp_neg_register_fmt_match_cb(&desc->info.encoding_name, desc->sdp_fmt_match);
			pj_assert(status == PJ_SUCCESS);
		}
	}

	/* Register codec factory to codec manager. */
	status = pjmedia_vid_codec_mgr_register_factory(mgr, &x264_factory.base);
	if (status != PJ_SUCCESS)
		goto on_error;

	x264_factory.pool = pool;

	/* Done. */
	return PJ_SUCCESS;

on_error:
	pj_pool_release(pool);
	return status;
}

/*
* Unregister x264 codecs factory from pjmedia endpoint.
*/
PJ_DEF(pj_status_t) pjmedia_codec_x264_deinit(void)
{
	pj_status_t status = PJ_SUCCESS;

	if (x264_factory.pool == NULL)
	{
		/* Already deinitialized */
		return PJ_SUCCESS;
	}

	pj_mutex_lock(x264_factory.mutex);

	/* Unregister x264 codecs factory. */
	status = pjmedia_vid_codec_mgr_unregister_factory(x264_factory.mgr, &x264_factory.base);

	/* Destroy mutex. */
	pj_mutex_destroy(x264_factory.mutex);

	/* Destroy pool. */
	pj_pool_release(x264_factory.pool);
	x264_factory.pool = NULL;

	return status;
}


/* 
* Check if factory can allocate the specified codec. 
*/
static pj_status_t x264_test_alloc( pjmedia_vid_codec_factory *factory, const pjmedia_vid_codec_info *info )
{
	const x264_codec_desc *desc;

	PJ_ASSERT_RETURN(factory == &x264_factory.base, PJ_EINVAL);
	PJ_ASSERT_RETURN(info, PJ_EINVAL);

	desc = find_codec_desc_by_info(info);
	if (!desc)
	{
		return PJMEDIA_CODEC_EUNSUP;
	}

	if(desc->info.dir == PJMEDIA_DIR_ENCODING)
		return PJMEDIA_CODEC_DIR_ENCODE;
	if(desc->info.dir == PJMEDIA_DIR_DECODING)
		return PJMEDIA_CODEC_DIR_DECODE;

	return PJ_SUCCESS;
}

/*
* Generate default attribute.
*/
static pj_status_t x264_default_attr( pjmedia_vid_codec_factory *factory, const pjmedia_vid_codec_info *info, pjmedia_vid_codec_param *attr )
{
	const x264_codec_desc *desc;
	unsigned i;

	PJ_ASSERT_RETURN(factory==&x264_factory.base, PJ_EINVAL);
	PJ_ASSERT_RETURN(info && attr, PJ_EINVAL);

	desc = find_codec_desc_by_info(info);
	if (!desc)
	{
		return PJMEDIA_CODEC_EUNSUP;
	}

	pj_bzero(attr, sizeof(pjmedia_vid_codec_param));

	/* Scan the requested packings and use the lowest number */
	attr->packing = 0;
	for (i=0; i<15; ++i)
	{
		unsigned packing = (1 << i);
		if ((desc->info.packings & info->packings) & packing)
		{
			attr->packing = (pjmedia_vid_packing)packing;
			break;
		}
	}
	if (attr->packing == 0)
	{
		/* No supported packing in info */
		return PJMEDIA_CODEC_EUNSUP;
	}

	/* Direction */
	attr->dir = desc->info.dir;

	/* Encoded format */
	//pjmedia_format_init_video(&attr->enc_fmt, desc->info.fmt_id, 720, 480, 30000, 1001);
	pjmedia_format_init_video(&attr->enc_fmt, desc->info.fmt_id, desc->size.w, desc->size.h, desc->fps.num, desc->fps.denum);

	/* Decoded format */
	//pjmedia_format_init_video(&attr->dec_fmt, desc->info.dec_fmt_id[0],
	//	//352, 288, 30000, 1001);
	//	720, 576, 30000, 1001);
	pjmedia_format_init_video(&attr->dec_fmt, desc->info.dec_fmt_id[0],
		desc->size.w, desc->size.h,
		desc->fps.num*3/2, desc->fps.denum);

	///* Decoding fmtp */
	attr->dec_fmtp = desc->dec_fmtp;

	/* Bitrate */
	attr->enc_fmt.det.vid.avg_bps = desc->avg_bps;
	attr->enc_fmt.det.vid.max_bps = desc->max_bps;

	/* MTU */
	attr->enc_mtu = PJMEDIA_MAX_MTU;

	return PJ_SUCCESS;
}

/*
* Enum codecs supported by this factory.
*/
static pj_status_t x264_enum_codecs( pjmedia_vid_codec_factory *factory, unsigned *count, pjmedia_vid_codec_info codecs[])
{
	unsigned i, max_cnt;

	PJ_ASSERT_RETURN(codecs && *count > 0, PJ_EINVAL);
	PJ_ASSERT_RETURN(factory == &x264_factory.base, PJ_EINVAL);

	max_cnt = PJ_MIN(*count, PJ_ARRAY_SIZE(codec_desc));
	*count = 0;

	for (i=0; i<max_cnt; ++i)
	{
		if (codec_desc[i].enabled)
		{
			pj_memcpy(&codecs[*count], &codec_desc[i].info, sizeof(pjmedia_vid_codec_info));
			(*count)++;
		}
	}

	return PJ_SUCCESS;
}

/*
* Allocate a new codec instance.
*/
static pj_status_t x264_alloc_codec( pjmedia_vid_codec_factory *factory, 
																			const pjmedia_vid_codec_info *info,
																			pjmedia_vid_codec **p_codec)
{
	x264_private *x264;
	const x264_codec_desc *desc;
	pjmedia_vid_codec *codec;
	pj_pool_t *pool = NULL;
	pj_status_t status = PJ_SUCCESS;

	PJ_ASSERT_RETURN(factory && info && p_codec, PJ_EINVAL);
	PJ_ASSERT_RETURN(factory == &x264_factory.base, PJ_EINVAL);

	desc = find_codec_desc_by_info(info);
	if (!desc)
	{
		return PJMEDIA_CODEC_EUNSUP;
	}

	/* Create pool for codec instance */
	pool = pj_pool_create(x264_factory.pf, "x264 codec", 512, 512, NULL);
	codec = PJ_POOL_ZALLOC_T(pool, pjmedia_vid_codec);
	if (!codec)
	{
		status = PJ_ENOMEM;
		goto on_error;
	}
	codec->op = &x264_op;
	codec->factory = factory;
	x264 = PJ_POOL_ZALLOC_T(pool, x264_private);
	if (!x264)
	{
		status = PJ_ENOMEM;
		goto on_error;
	}
	codec->codec_data = x264;
	x264->pool = pool;
	//x264->enc = desc->enc;   // ÏÎÏÎÂ ???????????????? ÅÍÊÎÄÅÐ ÄÎ ÝÒÎÃÎ ÍÅ ÇÀÄÀÂÀËÑß
	//x264->dec = desc->dec;
	x264->desc = desc;

	*p_codec = codec;
	return PJ_SUCCESS;

on_error:
	if (pool)
		pj_pool_release(pool);
	return status;
}

/*
* Free codec.
*/
static pj_status_t x264_dealloc_codec( pjmedia_vid_codec_factory *factory, pjmedia_vid_codec *codec )
{
	x264_private *x264;
	pj_pool_t *pool;

	PJ_ASSERT_RETURN(factory && codec, PJ_EINVAL);
	PJ_ASSERT_RETURN(factory == &x264_factory.base, PJ_EINVAL);

	/* Close codec, if it's not closed. */
	x264 = (x264_private*) codec->codec_data;
	pool = x264->pool;
	codec->codec_data = NULL;
	pj_pool_release(pool);

	return PJ_SUCCESS;
}

/*
* Init codec.
*/
static pj_status_t x264_codec_init( pjmedia_vid_codec *codec, pj_pool_t *pool )
{
	PJ_UNUSED_ARG(codec);
	PJ_UNUSED_ARG(pool);
	return PJ_SUCCESS;
}

static void print_x264_err(int err)
{
//#if LIBAVCODEC_VER_AT_LEAST(52,72)
//	char errbuf[512];
//	if (av_strerror(err, errbuf, sizeof(errbuf)) >= 0)
//		PJ_LOG(5, (THIS_FILE, "ffmpeg err %d: %s", err, errbuf));
//#else
//	PJ_LOG(5, (THIS_FILE, "x264 err %d", err));
//#endif
	PJ_LOG(5, (THIS_FILE, "x264 err %d", err));

}

static pj_status_t open_x264_codec(x264_private *x264, pj_mutex_t *x264_mutex)
{
	enum PixelFormat pix_fmt;
	pjmedia_video_format_detail *vfd;
	pj_bool_t enc_opened = PJ_FALSE, dec_opened = PJ_FALSE;
	pj_status_t status;

	/* Get decoded pixel format */
	//status = pjmedia_format_id_to_PixelFormat(x264->param.dec_fmt.id, &pix_fmt);
	//if (status != PJ_SUCCESS)
	//	return status;
	//x264->expected_dec_fmt = pix_fmt;

	/* Get video format detail for shortcut access to encoded format */
	vfd = pjmedia_format_get_video_format_detail(&x264->param.enc_fmt, PJ_TRUE);

	/* Allocate x264 codec context */
	if (x264->param.dir & PJMEDIA_DIR_ENCODING)
	{
		x264_param_t *ctx;
		x264->enc_ctx = PJ_POOL_ZALLOC_T(x264->pool, x264_param_t);
		//x264->enc_ctx = new x264_param_t();
		//x264_param_default( x264->enc_ctx );

		ctx = x264->enc_ctx;
		if (x264->enc_ctx == NULL)
			goto on_error;


		x264_param_default_preset(ctx, "ultrafast", "zerolatency");

		/* Init generic encoder params */

		//ctx->pix_fmt = pix_fmt;
		ctx->i_width = vfd->size.w;
		ctx->i_height = vfd->size.h;
		ctx->i_timebase_num = vfd->fps.denum;
		ctx->i_timebase_den = vfd->fps.num;
		//if (vfd->avg_bps)
		//{
		//	ctx->bit_rate = vfd->avg_bps;
		//	if (vfd->max_bps > vfd->avg_bps)
		//		ctx->bit_rate_tolerance = vfd->max_bps - vfd->avg_bps;
		//}
		//ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;
		//ctx->workaround_bugs = FF_BUG_AUTODETECT;
		//ctx->opaque = ff;
	}

//
//	/* Init generic decoder params */
//	if (ff->param.dir & PJMEDIA_DIR_DECODING)
//	{
//		AVCodecContext *ctx = ff->dec_ctx;
//
//		/* Width/height may be overriden by ffmpeg after first decoding. */
//		ctx->width  = ctx->coded_width  = ff->param.dec_fmt.det.vid.size.w;
//		ctx->height = ctx->coded_height = ff->param.dec_fmt.det.vid.size.h;
//		ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
//		ctx->workaround_bugs = FF_BUG_AUTODETECT;
//		ctx->opaque = ff;
//	}
//
	/* Override generic params or apply specific params before opening
	* the codec.
	*/
	if (x264->desc->preopen)
	{
		status = (*x264->desc->preopen)(x264);
		if (status != PJ_SUCCESS)
			goto on_error;
	}

	/* Open encoder */
	if (x264->param.dir & PJMEDIA_DIR_ENCODING)
	{
		int err;

		pj_mutex_lock(x264_mutex);
		x264->enc = x264_encoder_open(x264->enc_ctx);
		//err = avcodec_open(x264->enc_ctx, x264->enc);
		pj_mutex_unlock(x264_mutex);
		if (x264->enc == 0)
		{
			//print_x264_err(err);
			status = PJMEDIA_CODEC_EFAILED;
			goto on_error;
		}
		enc_opened = PJ_TRUE;
	}
//
//	/* Open decoder */
//	if (ff->param.dir & PJMEDIA_DIR_DECODING)
//	{
//		int err;
//
//		pj_mutex_lock(ff_mutex);
//		err = avcodec_open(ff->dec_ctx, ff->dec);
//		pj_mutex_unlock(ff_mutex);
//		if (err < 0)
//		{
//			print_ffmpeg_err(err);
//			status = PJMEDIA_CODEC_EFAILED;
//			goto on_error;
//		}
//		dec_opened = PJ_TRUE;
//	}
//
	/* Let the codec apply specific params after the codec opened */
	if (x264->desc->postopen)
	{
		status = (*x264->desc->postopen)(x264);
		if (status != PJ_SUCCESS)
			goto on_error;
	}

	return PJ_SUCCESS;

on_error:
	if (x264->enc_ctx)
	{
		if (enc_opened)
			x264_encoder_close(x264->enc);
		//av_free(ff->enc_ctx);
		//delete x264->enc_ctx;
		//x264->enc_ctx = NULL;
	}
	//if (ff->dec_ctx)
	//{
	//	if (dec_opened)
	//		avcodec_close(ff->dec_ctx);
	//	av_free(ff->dec_ctx);
	//	ff->dec_ctx = NULL;
	//}
	return status;
}

/*
* Open codec.
*/
static pj_status_t x264_codec_open( pjmedia_vid_codec *codec, pjmedia_vid_codec_param *attr )
{
	x264_private *x264;
	pj_status_t status;
	pj_mutex_t *x264_mutex;

	PJ_ASSERT_RETURN(codec && attr, PJ_EINVAL);
	x264 = (x264_private*)codec->codec_data;

	pj_memcpy(&x264->param, attr, sizeof(*attr));

	/* Open the codec */
	x264_mutex = ((struct x264_factory*)codec->factory)->mutex;
	status = open_x264_codec(x264, x264_mutex);
	if (status != PJ_SUCCESS)
		goto on_error;

	/* Init format info and apply-param of decoder */
	//////////x264->dec_vfi = pjmedia_get_video_format_info(NULL, x264->param.dec_fmt.id);
	//////////if (!x264->dec_vfi)
	//////////{
	//////////	status = PJ_EINVAL;
	//////////	goto on_error;
	//////////}
	//////////pj_bzero(&x264->dec_vafp, sizeof(x264->dec_vafp));
	//////////x264->dec_vafp.size = x264->param.dec_fmt.det.vid.size;
	//////////x264->dec_vafp.buffer = NULL;
	//////////status = (*x264->dec_vfi->apply_fmt)(x264->dec_vfi, &x264->dec_vafp);
	//////////if (status != PJ_SUCCESS)
	//////////{
	//////////	goto on_error;
	//////////}

	/* Init format info and apply-param of encoder */
	x264->enc_vfi = pjmedia_get_video_format_info(NULL, x264->param.dec_fmt.id);
	if (!x264->enc_vfi)
	{
		status = PJ_EINVAL;
		goto on_error;
	}
	pj_bzero(&x264->enc_vafp, sizeof(x264->enc_vafp));
	x264->enc_vafp.size = x264->param.enc_fmt.det.vid.size;
	x264->enc_vafp.buffer = NULL;
	status = (*x264->enc_vfi->apply_fmt)(x264->enc_vfi, &x264->enc_vafp);
	if (status != PJ_SUCCESS)
	{
		goto on_error;
	}

	/* Alloc buffers if needed */
	x264->whole = (x264->param.packing == PJMEDIA_VID_PACKING_WHOLE);
	if (!x264->whole)
	{
		x264->enc_buf_size = x264->enc_vafp.framebytes;
		x264->enc_buf = pj_pool_alloc(x264->pool, x264->enc_buf_size);

		//x264->dec_buf_size = x264->dec_vafp.framebytes;
		//x264->dec_buf = pj_pool_alloc(x264->pool, x264->dec_buf_size);
	}

	/* Update codec attributes, e.g: encoding format may be changed by
	* SDP fmtp negotiation.
	*/
	pj_memcpy(attr, &x264->param, sizeof(*attr));

	return PJ_SUCCESS;

on_error:
	x264_codec_close(codec);
	return status;
}

/*
* Close codec.
*/
static pj_status_t x264_codec_close( pjmedia_vid_codec *codec )
{
	x264_private *x264;
	pj_mutex_t *x264_mutex;

	PJ_ASSERT_RETURN(codec, PJ_EINVAL);
	x264 = (x264_private*)codec->codec_data;
	x264_mutex = ((struct x264_factory*)codec->factory)->mutex;

	pj_mutex_lock(x264_mutex);


	if (x264->enc)
	{
		x264_encoder_close(x264->enc);
		x264->enc = NULL;

		//av_free(ff->enc_ctx);
		//delete x264->enc_ctx;
		//avcodec_close(x264->enc_ctx);
		//av_free(ff->enc_ctx);
	}

	//x264_free( x264->pic_in.img.plane[0] );
	//x264_picture_clean( &x264->pic_in );
	pj_mutex_unlock(x264_mutex);

	//x264_picture_clean( &x264->pic_in );


	//if (x264->enc && x264->enc_ctx)
	//{
	//	x264_encoder_close(x264->enc);
	//	//av_free(ff->enc_ctx);
	//	delete x264->enc_ctx;
	//	//avcodec_close(x264->enc_ctx);
	//	//av_free(ff->enc_ctx);
	//}
	//if (ff->dec_ctx && ff->dec_ctx!=ff->enc_ctx)
	//{
	//	avcodec_close(ff->dec_ctx);
	//	av_free(ff->dec_ctx);
	//}
	x264->enc_ctx = NULL;
	//ff->dec_ctx = NULL;
	//pj_mutex_unlock(x264_mutex);

	return PJ_SUCCESS;
}


/*
* Modify codec settings.
*/
static pj_status_t  x264_codec_modify( pjmedia_vid_codec *codec, const pjmedia_vid_codec_param *attr)
{
	x264_private *x264 = (x264_private*)codec->codec_data;

	PJ_UNUSED_ARG(attr);
	PJ_UNUSED_ARG(x264);

	return PJ_ENOTSUP;
}

static pj_status_t  x264_codec_get_param(pjmedia_vid_codec *codec, pjmedia_vid_codec_param *param)
{
	x264_private *x264;

	PJ_ASSERT_RETURN(codec && param, PJ_EINVAL);

	x264 = (x264_private*)codec->codec_data;
	pj_memcpy(param, &x264->param, sizeof(*param));

	return PJ_SUCCESS;
}


static pj_status_t  x264_packetize ( pjmedia_vid_codec *codec,
																			pj_uint8_t *bits,
																			pj_size_t bits_len,
																			unsigned *bits_pos,
																			const pj_uint8_t **payload,
																			pj_size_t *payload_len)
{
	x264_private *x264 = (x264_private*)codec->codec_data;

	if (x264->desc->packetize)
	{
		return (*x264->desc->packetize)(x264, bits, bits_len, bits_pos, payload, payload_len);
	}

	return PJ_ENOTSUP;
}

static pj_status_t  x264_unpacketize(pjmedia_vid_codec *codec,
																			 const pj_uint8_t *payload,
																			 pj_size_t   payload_len,
																			 pj_uint8_t *bits,
																			 pj_size_t   bits_len,
																			 unsigned   *bits_pos)
{
	x264_private *x264 = (x264_private*)codec->codec_data;

	if (x264->desc->unpacketize)
	{
		return (*x264->desc->unpacketize)(x264, payload, payload_len, bits, bits_len, bits_pos);
	}

	return PJ_ENOTSUP;
}


/*
* Encode frames.
*/
static pj_status_t x264_codec_encode_whole(pjmedia_vid_codec *codec,
																						 const pjmedia_vid_encode_opt *opt,
																						 const pjmedia_frame *input,
																						 unsigned output_buf_len,
																						 pjmedia_frame *output)
{
	x264_private *x264 = (x264_private*)codec->codec_data;
	pj_uint8_t *p = (pj_uint8_t*)input->buf;
	//AVFrame avframe;
	pj_uint8_t *out_buf = (pj_uint8_t*)output->buf;
	int out_buf_len = output_buf_len;
	int err;
	x264_picture_t pic_out;
	int srcstride;
	x264_nal_t *nals = 0;
	int i_nals = 0;
	
	
	//AVRational src_timebase;
	/* For some reasons (e.g: SSE/MMX usage), the avcodec_encode_video() must
	* have stack aligned to 16 bytes. Let's try to be safe by preparing the
	* 16-bytes aligned stack here, in case it's not managed by the ffmpeg.
	*/
	PJ_ALIGN_DATA(pj_uint32_t i[4], 16);

	if ((long)i & 0xF)
	{
		PJ_LOG(2,(THIS_FILE, "Stack alignment fails"));
	}

	
	/* Check if encoder has been opened */
	PJ_ASSERT_RETURN(x264->enc_ctx, PJ_EINVALIDOP);
	PJ_ASSERT_RETURN(x264->enc, PJ_EINVALIDOP);

	//avcodec_get_frame_defaults(&avframe);

	// Let ffmpeg manage the timestamps
	/*
	src_timebase.num = 1;
	src_timebase.den = ff->desc->info.clock_rate;
	avframe.pts = av_rescale_q(input->timestamp.u64, src_timebase,
	ff->enc_ctx->time_base);
	*/

	for (i[0] = 0; i[0] < x264->enc_vfi->plane_cnt; ++i[0])
	{
		x264->pic_in.img.plane[i[0]] = p;//x264->enc_vafp.planes[i[0]];
		x264->pic_in.img.i_stride[i[0]] = x264->enc_vafp.strides[i[0]];
		//int     i_csp;       /* Colorspace */
		//int     i_plane;     /* Number of image planes */
		//int     i_stride[4]; /* Strides for each plane */
		//uint8_t *plane[4];   /* Pointers to each plane */
		p += x264->enc_vafp.plane_bytes[i[0]];
	}

	/* Force keyframe */
//////	if (opt && opt->force_keyframe)
//////	{
//////#if LIBAVCODEC_VER_AT_LEAST(53,20)
//////		avframe.pict_type = AV_PICTURE_TYPE_I;
//////#else
//////		avframe.pict_type = FF_I_TYPE;
//////#endif
//////	}

	//x264_encoder_encode( x264_t *, x264_nal_t **pp_nal, int *pi_nal, x264_picture_t *pic_in, x264_picture_t *pic_out );

	//x264_picture_alloc(&pic_out, X264_CSP_I420, x264->enc_ctx->i_width, x264->enc_ctx->i_height);

	//srcstride = x264->enc_ctx->i_width*3; //RGB stride is just 3*width
	//sws_scale(x264->convertCtx, &p, &srcstride, 0, x264->enc_ctx->i_height, x264->pic_in.img.plane, x264->pic_in.img.i_stride);

	{
		int ret = 0;
		int i = 0;
		int nalsize = 0;
		pj_int32_t left = out_buf_len;//input->size;
		int realOutputSize = 0;
		//output->size = 0;
		do 
		{
			ret = x264_encoder_encode(x264->enc, &nals, &i_nals, &x264->pic_in, &pic_out);
			if(ret < 0)
				return PJMEDIA_CODEC_EFAILED;
			
			
			for (i = 0; i < i_nals; i++)
			{
				nalsize = nals[i].i_payload;
				if (nalsize > left)
					return input->size - left;
				memcpy(out_buf, nals[i].p_payload, nalsize);
				out_buf += nalsize;
				left -= nalsize;
			}

			realOutputSize += ret;

		}
		while (x264_encoder_delayed_frames(x264->enc));

		output->size = realOutputSize;
		output->bit_info = 0;
		if (pic_out.b_keyframe)
			output->bit_info |= PJMEDIA_VID_FRM_KEYFRAME;
	}

	//err = avcodec_encode_video(ff->enc_ctx, out_buf, out_buf_len, &avframe);
	//if (err < 0)
	//{
	//	print_ffmpeg_err(err);
	//	return PJMEDIA_CODEC_EFAILED;
	//}
	//else
	//{
	//	output->size = err;
	//	output->bit_info = 0;
	//	if (ff->enc_ctx->coded_frame->key_frame)
	//		output->bit_info |= PJMEDIA_VID_FRM_KEYFRAME;
	//}

	return PJ_SUCCESS;
}

static pj_status_t x264_codec_encode_begin(pjmedia_vid_codec *codec,
																						 const pjmedia_vid_encode_opt *opt,
																						 const pjmedia_frame *input,
																						 unsigned out_size,
																						 pjmedia_frame *output,
																						 pj_bool_t *has_more)
{
	

	x264_private *x264 = (x264_private*)codec->codec_data;
	pj_status_t status;
	//pj_mutex_t *x264_mutex;

	*has_more = PJ_FALSE;

	//x264_mutex = ((struct x264_factory*)codec->factory)->mutex;

	//pj_mutex_lock(x264_mutex);

	if (x264->whole)
	{
		status = x264_codec_encode_whole(codec, opt, input, out_size, output);
	}
	else
	{
		pjmedia_frame whole_frm;
		const pj_uint8_t *payload;
		pj_size_t payload_len;

		pj_bzero(&whole_frm, sizeof(whole_frm));
		whole_frm.buf = x264->enc_buf;
		whole_frm.size = x264->enc_buf_size;
		status = x264_codec_encode_whole(codec, opt, input, whole_frm.size, &whole_frm);
		if (status != PJ_SUCCESS)
		{
			//pj_mutex_unlock(x264_mutex);
			return status;
		}

		x264->enc_buf_is_keyframe = (whole_frm.bit_info & PJMEDIA_VID_FRM_KEYFRAME);
		x264->enc_frame_len = (unsigned)whole_frm.size;
		x264->enc_processed = 0;
		status = x264_packetize(codec, (pj_uint8_t*)whole_frm.buf, whole_frm.size, &x264->enc_processed, &payload, &payload_len);
		if (status != PJ_SUCCESS)
		{
			//pj_mutex_unlock(x264_mutex);
			return status;
		}

		if (out_size < payload_len)
		{
			//pj_mutex_unlock(x264_mutex);
			return PJMEDIA_CODEC_EFRMTOOSHORT;
		}

		output->type = PJMEDIA_FRAME_TYPE_VIDEO;
		pj_memcpy(output->buf, payload, payload_len);
		output->size = payload_len;

		if (x264->enc_buf_is_keyframe)
			output->bit_info |= PJMEDIA_VID_FRM_KEYFRAME;

		*has_more = (x264->enc_processed < x264->enc_frame_len);
	}

	//pj_mutex_unlock(x264_mutex);

	return status;
}

static pj_status_t x264_codec_encode_more(pjmedia_vid_codec *codec,
																						unsigned out_size,
																						pjmedia_frame *output,
																						pj_bool_t *has_more)
{
	x264_private *x264 = (x264_private*)codec->codec_data;
	const pj_uint8_t *payload;
	pj_size_t payload_len;
	pj_status_t status;

	*has_more = PJ_FALSE;

	if (x264->enc_processed >= x264->enc_frame_len) {
		/* No more frame */
		return PJ_EEOF;
	}

	status = x264_packetize(codec, (pj_uint8_t*)x264->enc_buf,
		x264->enc_frame_len, &x264->enc_processed,
		&payload, &payload_len);
	if (status != PJ_SUCCESS)
		return status;

	if (out_size < payload_len)
		return PJMEDIA_CODEC_EFRMTOOSHORT;

	output->type = PJMEDIA_FRAME_TYPE_VIDEO;
	pj_memcpy(output->buf, payload, payload_len);
	output->size = payload_len;

	if (x264->enc_buf_is_keyframe)
		output->bit_info |= PJMEDIA_VID_FRM_KEYFRAME;

	*has_more = (x264->enc_processed < x264->enc_frame_len);

	return PJ_SUCCESS;
}


static pj_status_t check_decode_result(pjmedia_vid_codec *codec,
																			 const pj_timestamp *ts,
																			 pj_bool_t got_keyframe)
{
	//ffmpeg_private *ff = (ffmpeg_private*)codec->codec_data;
	//pjmedia_video_apply_fmt_param *vafp = &ff->dec_vafp;
	//pjmedia_event event;

	///* Check for format change.
	//* Decoder output format is set by libavcodec, in case it is different
	//* to the configured param.
	//*/
	//if (ff->dec_ctx->pix_fmt != ff->expected_dec_fmt ||
	//	ff->dec_ctx->width != (int)vafp->size.w ||
	//	ff->dec_ctx->height != (int)vafp->size.h)
	//{
	//	pjmedia_format_id new_fmt_id;
	//	pj_status_t status;

	//	/* Get current raw format id from ffmpeg decoder context */
	//	status = PixelFormat_to_pjmedia_format_id(ff->dec_ctx->pix_fmt, 
	//		&new_fmt_id);
	//	if (status != PJ_SUCCESS)
	//		return status;

	//	/* Update decoder format in param */
	//	ff->param.dec_fmt.id = new_fmt_id;
	//	ff->param.dec_fmt.det.vid.size.w = ff->dec_ctx->width;
	//	ff->param.dec_fmt.det.vid.size.h = ff->dec_ctx->height;
	//	ff->expected_dec_fmt = ff->dec_ctx->pix_fmt;

	//	/* Re-init format info and apply-param of decoder */
	//	ff->dec_vfi = pjmedia_get_video_format_info(NULL, ff->param.dec_fmt.id);
	//	if (!ff->dec_vfi)
	//		return PJ_ENOTSUP;
	//	pj_bzero(&ff->dec_vafp, sizeof(ff->dec_vafp));
	//	ff->dec_vafp.size = ff->param.dec_fmt.det.vid.size;
	//	ff->dec_vafp.buffer = NULL;
	//	status = (*ff->dec_vfi->apply_fmt)(ff->dec_vfi, &ff->dec_vafp);
	//	if (status != PJ_SUCCESS)
	//		return status;

	//	/* Realloc buffer if necessary */
	//	if (ff->dec_vafp.framebytes > ff->dec_buf_size) {
	//		PJ_LOG(5,(THIS_FILE, "Reallocating decoding buffer %u --> %u",
	//			(unsigned)ff->dec_buf_size,
	//			(unsigned)ff->dec_vafp.framebytes));
	//		ff->dec_buf_size = ff->dec_vafp.framebytes;
	//		ff->dec_buf = pj_pool_alloc(ff->pool, ff->dec_buf_size);
	//	}

	//	/* Broadcast format changed event */
	//	pjmedia_event_init(&event, PJMEDIA_EVENT_FMT_CHANGED, ts, codec);
	//	event.data.fmt_changed.dir = PJMEDIA_DIR_DECODING;
	//	pj_memcpy(&event.data.fmt_changed.new_fmt, &ff->param.dec_fmt,
	//		sizeof(ff->param.dec_fmt));
	//	pjmedia_event_publish(NULL, codec, &event, 0);
	//}

	///* Check for missing/found keyframe */
	//if (got_keyframe) {
	//	pj_get_timestamp(&ff->last_dec_keyframe_ts);

	//	/* Broadcast keyframe event */
	//	pjmedia_event_init(&event, PJMEDIA_EVENT_KEYFRAME_FOUND, ts, codec);
	//	pjmedia_event_publish(NULL, codec, &event, 0);
	//} else if (ff->last_dec_keyframe_ts.u64 == 0) {
	//	/* Broadcast missing keyframe event */
	//	pjmedia_event_init(&event, PJMEDIA_EVENT_KEYFRAME_MISSING, ts, codec);
	//	pjmedia_event_publish(NULL, codec, &event, 0);
	//}

	return PJ_SUCCESS;
}

/*
* Decode frame.
*/
static pj_status_t x264_codec_decode_whole(pjmedia_vid_codec *codec,
																						 const pjmedia_frame *input,
																						 unsigned output_buf_len,
																						 pjmedia_frame *output)
{
//	ffmpeg_private *ff = (ffmpeg_private*)codec->codec_data;
//	AVFrame avframe;
//	AVPacket avpacket;
//	int err, got_picture;
//
//	/* Check if decoder has been opened */
//	PJ_ASSERT_RETURN(ff->dec_ctx, PJ_EINVALIDOP);
//
//	/* Reset output frame bit info */
//	output->bit_info = 0;
//
//	/* Validate output buffer size */
//	// Do this validation later after getting decoding result, where the real
//	// decoded size will be assured.
//	//if (ff->dec_vafp.framebytes > output_buf_len)
//	//return PJ_ETOOSMALL;
//
//	/* Init frame to receive the decoded data, the ffmpeg codec context will
//	* automatically provide the decoded buffer (single buffer used for the
//	* whole decoding session, and seems to be freed when the codec context
//	* closed).
//	*/
//	avcodec_get_frame_defaults(&avframe);
//
//	/* Init packet, the container of the encoded data */
//	av_init_packet(&avpacket);
//	avpacket.data = (pj_uint8_t*)input->buf;
//	avpacket.size = input->size;
//
//	/* ffmpeg warns:
//	* - input buffer padding, at least FF_INPUT_BUFFER_PADDING_SIZE
//	* - null terminated
//	* Normally, encoded buffer is allocated more than needed, so lets just
//	* bzero the input buffer end/pad, hope it will be just fine.
//	*/
//	pj_bzero(avpacket.data+avpacket.size, FF_INPUT_BUFFER_PADDING_SIZE);
//
//	output->bit_info = 0;
//	output->timestamp = input->timestamp;
//
//#if LIBAVCODEC_VER_AT_LEAST(52,72)
//	//avpacket.flags = AV_PKT_FLAG_KEY;
//#else
//	avpacket.flags = 0;
//#endif
//
//#if LIBAVCODEC_VER_AT_LEAST(52,72)
//	err = avcodec_decode_video2(ff->dec_ctx, &avframe, 
//		&got_picture, &avpacket);
//#else
//	err = avcodec_decode_video(ff->dec_ctx, &avframe,
//		&got_picture, avpacket.data, avpacket.size);
//#endif
//	if (err < 0) {
//		pjmedia_event event;
//
//		output->type = PJMEDIA_FRAME_TYPE_NONE;
//		output->size = 0;
//		print_ffmpeg_err(err);
//
//		/* Broadcast missing keyframe event */
//		pjmedia_event_init(&event, PJMEDIA_EVENT_KEYFRAME_MISSING,
//			&input->timestamp, codec);
//		pjmedia_event_publish(NULL, codec, &event, 0);
//
//		return PJMEDIA_CODEC_EBADBITSTREAM;
//	} else if (got_picture) {
//		pjmedia_video_apply_fmt_param *vafp = &ff->dec_vafp;
//		pj_uint8_t *q = (pj_uint8_t*)output->buf;
//		unsigned i;
//		pj_status_t status;
//
//		/* Check decoding result, e.g: see if the format got changed,
//		* keyframe found/missing.
//		*/
//		status = check_decode_result(codec, &input->timestamp,
//			avframe.key_frame);
//		if (status != PJ_SUCCESS)
//			return status;
//
//		/* Check provided buffer size */
//		if (vafp->framebytes > output_buf_len)
//			return PJ_ETOOSMALL;
//
//		/* Get the decoded data */
//		for (i = 0; i < ff->dec_vfi->plane_cnt; ++i) {
//			pj_uint8_t *p = avframe.data[i];
//
//			/* The decoded data may contain padding */
//			if (avframe.linesize[i]!=vafp->strides[i]) {
//				/* Padding exists, copy line by line */
//				pj_uint8_t *q_end;
//
//				q_end = q+vafp->plane_bytes[i];
//				while(q < q_end) {
//					pj_memcpy(q, p, vafp->strides[i]);
//					q += vafp->strides[i];
//					p += avframe.linesize[i];
//				}
//			} else {
//				/* No padding, copy the whole plane */
//				pj_memcpy(q, p, vafp->plane_bytes[i]);
//				q += vafp->plane_bytes[i];
//			}
//		}
//
//		output->type = PJMEDIA_FRAME_TYPE_VIDEO;
//		output->size = vafp->framebytes;
//	} else {
//		output->type = PJMEDIA_FRAME_TYPE_NONE;
//		output->size = 0;
//	}
//
	return PJ_SUCCESS;
}

static pj_status_t x264_codec_decode( pjmedia_vid_codec *codec,
																			 pj_size_t pkt_count,
																			 pjmedia_frame packets[],
																			 unsigned out_size,
																			 pjmedia_frame *output)
{
	//ffmpeg_private *ff = (ffmpeg_private*)codec->codec_data;
	//pj_status_t status;

	//PJ_ASSERT_RETURN(codec && pkt_count > 0 && packets && output, PJ_EINVAL);

	//if (ff->whole)
	//{
	//	pj_assert(pkt_count==1);
	//	return ffmpeg_codec_decode_whole(codec, &packets[0], out_size, output);
	//}
	//else
	//{
	//	pjmedia_frame whole_frm;
	//	unsigned whole_len = 0;
	//	unsigned i;

	//	for (i=0; i<pkt_count; ++i)
	//	{
	//		if (whole_len + packets[i].size > ff->dec_buf_size)
	//		{
	//			PJ_LOG(5,(THIS_FILE, "Decoding buffer overflow"));
	//			break;
	//		}

	//		status = ffmpeg_unpacketize(codec, packets[i].buf, packets[i].size,
	//			ff->dec_buf, ff->dec_buf_size,
	//			&whole_len);
	//		if (status != PJ_SUCCESS)
	//		{
	//			PJ_PERROR(5,(THIS_FILE, status, "Unpacketize error"));
	//			continue;
	//		}
	//	}

	//	whole_frm.buf = ff->dec_buf;
	//	whole_frm.size = whole_len;
	//	whole_frm.timestamp = output->timestamp = packets[i].timestamp;
	//	whole_frm.bit_info = 0;

	//	return ffmpeg_codec_decode_whole(codec, &whole_frm, out_size, output);
	//}

	return PJ_SUCCESS; // Óáðàòü
}



#ifdef _MSC_VER
	#ifdef NDEBUG
	#   pragma comment( lib, "libx264-120.lib")
	#else
	#   pragma comment( lib, "libx264d-120.lib")
	#endif
//#   pragma comment( lib, "avcodec.lib")
//#   pragma comment( lib, "avformat.lib")
//#   pragma comment( lib, "avutil.lib")
//#   pragma comment( lib, "swscale.lib")
//#   pragma comment( lib, "avcore.lib")
#endif

#endif	/* PJMEDIA_HAS_X264_CODEC */

