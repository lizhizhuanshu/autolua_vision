#ifndef SVISION_PNG_IMAGE_H
#define SVISION_PNG_IMAGE_H

#include"Bitmap.h"

#include <vector>


namespace autolua{
class CommonBitmap :public Bitmap
{
	std::vector<unsigned char> data_;
	const char* error_;
public:
	CommonBitmap();
	~CommonBitmap() override = default;
	bool toBoolResult(unsigned int error);
	bool load(const unsigned char* data, unsigned int size);
	bool load(const char* path);
	void load(Bitmap * source,int x,int y,int width,int height);
	const char* errorText(){
		return error_;
	}
};



} // namespace autolua



#endif //SVISION_PNG_IMAGE_H


