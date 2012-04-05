#include "frameconverter.h"

#define RGB24_LEN(w,h)      ((w) * (h) * 3)
#define RGB32_LEN(w,h)      ((w) * (h) * 4)
#define YUV420P_LEN(w,h)   (((w) * (h) * 3) / 2)
#define YUV422P_LEN(w,h)    ((w) * (h) * 2)

// YUV --> RGB Conversion macros
#define _S(a)     ((a)>255 ? 255 : (a)<0 ? 0 : (a))
#define _R(y,u,v) ((0x2568*(y) + 0x3343*(u)) / 0x2000)
#define _G(y,u,v) ((0x2568*(y) - 0x0c92*(v) - 0x1a1e*(u)) / 0x2000)
#define _B(y,u,v) ((0x2568*(y) + 0x40cf*(v)) / 0x2000)

//#define _R(y,u,v) (1.164*(y)  			       + 1.596*(u))
//#define _G(y,u,v) (1.164*(y) - 0.813*(v) - 0.391*(u))
//#define _B(y,u,v) (1.164*(y) + 2.018*(v))



//void YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize)
//{
//	int h, w;
//	const unsigned char *py, *pu, *pv;
//	py = src;
//	pu = py + (stride * height);
//	pv = pu + (stride * height) / 4;
//
//	int yStrideDelta = stride - width;
//	int uvStride = stride >> 1;
//
//	if (dstSize < (width * height * 3))
//	{
//		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
//		return;
//	}
//
//	for (h=0; h<height; h++)
//	{
//		for (w=0; w<width; w++)
//		{
//			signed int _r,_g,_b;
//			signed int r, g, b;
//			signed int y, u, v;
//
//			y = *py++ - 16;
//			u = pu[w>>1] - 128;
//			v = pv[w>>1] - 128;
//
//			_r = _R(y,u,v);
//			_g = _G(y,u,v);
//			_b = _B(y,u,v);
//
//			r = _S(_r);
//			g = _S(_g);
//			b = _S(_b);
//
//			*dst++ = r;
//			*dst++ = g;
//			*dst++ = b;
//			//*dst++ = 0;
//		}
//
//		py += yStrideDelta;
//		if (h%2) {
//			pu += uvStride;
//			pv += uvStride;
//		}
//	}
//}

void FC::YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize)
{
	int h, w;
	const unsigned char *py, *pu, *pv;
	py = src;
	pv = py + (stride * height);
	pu = pv + (stride * height) / 4;

	int yStrideDelta = stride - width;
	int uvStride = stride >> 1;

	if (dstSize < (width * height * 3))
	{
		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
		return;
	}

	for (h=0; h<height; h++)
	{
		for (w=0; w<width; w++)
		{
			signed int _r,_g,_b;
			signed int r, g, b;
			signed int y, u, v;

			y = *py++ - 16;
			v = pv[w>>1] - 128;
			u = pu[w>>1] - 128;

			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			*dst++ = r;
			*dst++ = g;
			*dst++ = b;
			//*dst++ = 0;
		}

		py += yStrideDelta;
		if (h%2) {
			pu += uvStride;
			pv += uvStride;
		}
	}
}

void FC::YUV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize)
{
	int h, w;
	const unsigned char *py, *pu, *pv;
	py = src;
	pu = py + (width * height);
	pv = pu + (width * height) / 4;

	if (dstSize < (width*height*4))//if (dstSize < (width*height*4))
	{
		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
		return;
	}

	for (h=0; h<height; h++)
	{
		for (w=0; w<width; w++)
		{
			signed int _r,_g,_b;
			signed int r, g, b;
			signed int y, u, v;

			y = *py++ - 16;
			u = pu[w>>1] - 128;
			v = pv[w>>1] - 128;


			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			*dst++ = r;
			*dst++ = g;
			*dst++ = b;
			*dst++ = 0;
		}

		//pu += (width>>1);
		//pv += (width>>1);
	}
}

void FC::YUYV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize)
{
	int macropixel = 0;
	int len = height * width / 2;
	const unsigned char *py, *pu, *pv;
	py = src;
	pu = py + 1;//py + (width * height);
	pv = py + 3;//pu + (width * height) / 4;

	if (dstSize < (width*height*3))//if (dstSize < (width*height*4))
	{
		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
		return;
	}

	for (macropixel = 0; macropixel < len; macropixel++)
	{
		signed int _r,_g,_b;
		signed int r, g, b;
		signed int y0, y1, u, v;

		y0 = *py - 16;
		y1 = *(py+2) - 16;
		u = *pu - 128;
		v = *pv - 128;

		_b = _R(y0,u,v);
		_g = _G(y0,u,v);
		_r = _B(y0,u,v);

		r = _S(_r);
		g = _S(_g);
		b = _S(_b);

		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
		//*dst++ = 0;

		_b = _R(y1,u,v);
		_g = _G(y1,u,v);
		_r = _B(y1,u,v);

		r = _S(_r);
		g = _S(_g);
		b = _S(_b);

		*dst++ = r;
		*dst++ = g;
		*dst++ = b;

		py = py + 4; // new
		pu = py + 1;
		pv = py + 3;
	}
}
