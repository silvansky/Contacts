/*
h264.h

(c) 2004 Paul Volkaerts

header for the H264 Container class
*/

#ifndef H264_CONTAINER_H_
#define H264_CONTAINER_H_


#ifndef WIN32
#include <mythtv/mythwidgets.h>
#include <mythtv/dialogbox.h>
#include <mythtv/volumecontrol.h>

#include "directory.h"
#endif

//#include "webcam.h"
//#include "sipfsm.h"
//#include "rtp.h"
#include "rtpheader.h"

extern "C"
{
#ifdef WIN32
#include "inttypes.h"
#define inline _inline
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "x264.h"
#else
#include "mythtv/ffmpeg/avcodec.h"
#endif
}

#define MAX_RGB_704_576     (704*576*4)
#define MAX_YUV_704_576     (800*576*3/2) // Add a little onto the width in case the stride is bigger than the width

void RGB24toYUV420p(uchar* srcRGB, int width, int height, uchar*& dstYUV420p);
// Declare static YUV and RGB handling fns.
void YUV422PtoYUV420P(int width, int height, unsigned char *image);
void RGB24toRGB32(const unsigned char *rgb24, unsigned char *rgb32, int len);
void YUV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize);
void YUV420PtoRGB32(const uchar *py, const uchar *pu, const uchar *pv, int width, int height, int stride, unsigned char *dst, int dstSize);
void YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize);
void scaleYuvImage(const uchar *yuvBuffer, int ow, int oh, int dw, int dh, uchar *dst);
void cropYuvImage(const uchar *yuvBuffer, int ow, int oh, int cx, int cy, int cw, int ch, uchar *dst);
void flipRgb32Image(const uchar *rgbBuffer, int w, int h, uchar *dst);
void flipYuv420pImage(const uchar *yuvBuffer, int w, int h, uchar *dst);
void flipYuv422pImage(const uchar *yuvBuffer, int w, int h, uchar *dst);
void flipRgb24Image(const uchar *rgbBuffer, int w, int h, uchar *dst);



class H264Container
{
public:
	H264Container(void);
	virtual ~H264Container(void);

	bool H264StartEncoder(int w, int h, int fps);
	bool H264StartDecoder(int w, int h);

	uchar *H264EncodeFrame(const uchar *yuvFrame, int *len);
	uchar *H264EncodeFrameRGB(const uchar *rgbFrame, int *len);
	uchar *H264DecodeFrame(const uchar *h264Frame, int h264FrameLen, uchar *rgbBuffer, int rgbBufferSize);

	void H264StopEncoder();
	void H264StopDecoder();

	int H264EncodeFrameToRTP(const uchar *rgbFrame, QList<RtpPacket*> **packets);

	void __cdecl rtp_callback(struct AVCodecContext *avctx, void *data, int size, int mb_nb);

private:
	//AVFrame pictureOut, *pictureIn;
	AVFrame *pictureOut, *pictureIn;
	AVCodec *h264Encoder, *h264Decoder;

	x264_t* x264Encoder;
	x264_picture_t pic_in, pic_out;
	struct SwsContext* convertCtx;
	int width, height;

	AVCodecContext *h264EncContext, *h264DecContext;
	int MaxPostEncodeSize, lastCompressedSize;
	unsigned char *PostEncodeFrame;//, *PreEncodeFrame;

	int _ssrc; // Источник синхронизации для видео
	quint32 _startTime;
	quint16 _senderSeq;
};

#endif
