
#ifndef __VISION_FEATURE_H__
#define __VISION_FEATURE_H__

#include"vision_color.h"
#include <cstdint>

namespace autolua {

struct FeatureComposition{
  FeatureComposition *next;
  uint16_t x;
  uint16_t y;
  ColorComposition* color;
};

struct FeatureCompositionRoot{
  uint32_t count;
  FeatureComposition *data;
};


auto decodeFeature(const char* str,int size,FeatureCompositionRoot*feature)->bool;
auto encodeFeature(FeatureCompositionRoot*feature)->std::string;

void freeFeatureComposition(FeatureCompositionRoot *feature);


auto isFeature(Bitmap*bitmap,FeatureCompositionRoot*feature,int shiftSum)->bool;
auto isFeature(Bitmap *bitmap,int x,int y, FeatureCompositionRoot *feature, int shiftSum)->bool;

} // namespace autolua

#endif // __VISION_FEATURE_H__