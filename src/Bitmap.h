#ifndef __VISION_BITMAP_H__
#define __VISION_BITMAP_H__
namespace autolua {
class Bitmap{
public:
	unsigned char* origin_;
	unsigned int width_;
	unsigned int height_;
	int rowShift_;
	int pixelStride_;
};

bool isImage(Bitmap*bitmap,int x,int y,Bitmap* templateImage,int shiftSum);

} //namespace autolua


#endif// __VISION_BITMAP_H__

