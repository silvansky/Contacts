#ifndef FRAMECONVERTER_H
#define FRAMECONVERTER_H

class FC
{
public:
	static void YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize);
	static void YUV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize);
	static void YUYV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize);
};

#endif // FRAMECONVERTER_H
