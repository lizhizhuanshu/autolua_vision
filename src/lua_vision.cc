

#include "lua_vision.h"

#include "CommonBitmap.h"
#include "lodepng.h"
#include "lua_util.h"
#include <filesystem>
#include <lauxlib.h>
#include <lua.h>
#include <lua.hpp>
#include <vector>

#include "Bitmap.h"
#include "vision.h"
#include "vision_color.h"
#include "vision_feature.h"
#include "vision_util.h"

using namespace autolua;

#define PUSH_FIND_ORDER(L,i,name) lua_pushstring(L,#name);\
lua_pushinteger(L,READ_ORDER::name);\
lua_settable(L,i)


static constexpr char COORDINATES_OVERFLOW[] = "The coordinates are off screen";
static ResourceProvider resourceProvider = nullptr;


static auto getColor(lua_State*L)->int;
static auto getColorCount(lua_State*L)->int;
static auto isColor(lua_State*L)->int;
static auto whichColor(lua_State*L)->int;
static auto findColor(lua_State*L)->int;
static auto isFeature(lua_State*L)->int;
static auto findFeature(lua_State*L)->int;
static auto isImage(lua_State*L)->int;
static auto whichImage(lua_State*L)->int;
static auto findImage(lua_State*L)->int;

static auto loadImage(lua_State*L)->int;
static auto indexMethod(lua_State*L)->int;
static auto saveImageTo(lua_State*L)->int;
static auto cloneImage(lua_State*L)->int;
static auto getImageSize(lua_State*L)->int;



#define BASE_METHODS \
  {"getColor", getColor},\
  {"getColorCount", getColorCount},\
  {"isColor", isColor},\
  {"whichColor", whichColor},\
  {"findColor", findColor},\
  {"isFeature", isFeature},\
  {"findFeature", findFeature},\
  {"isImage", isImage},\
  {"whichImage", whichImage},\
  {"findImage", findImage},\

#define COMMON_BITMAP_METHODS \
  BASE_METHODS \
  {"save",saveImageTo},\
  {"clone",cloneImage},\
  {"getSize",getImageSize},\





auto luaopen_alv(struct lua_State *L) -> int{
  luaL_checkversion(L);

  if(luaL_newClassMetatable(Bitmap, L)){
    luaL_Reg methods[] = {
      COMMON_BITMAP_METHODS
      
      {nullptr, nullptr}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
  }

  if(luaL_newClassMetatable(CommonBitmap, L)){
    lua_pushvalue(L, -2);
    lua_setmetatable(L, -2);
    luaL_Reg methods[] = {
      {"__index",indexMethod},
      {"__gc",lua::finish<CommonBitmap>},
      {nullptr, nullptr}
    };
    luaL_setfuncs(L, methods, 0);
  }
  lua_pop(L,2);


  luaL_Reg methods[] = {
    BASE_METHODS

    {"saveImage",saveImageTo},
    {"cloneImage",cloneImage},
    {"getImageSize",getImageSize},
    {"loadImage",loadImage},
    {nullptr, nullptr}
  };
  luaL_newlib(L, methods);


	lua_newtable(L);
	PUSH_FIND_ORDER(L, -3, UP_DOWN_LEFT_RIGHT);
	PUSH_FIND_ORDER(L, -3, UP_DOWN_RIGHT_LEFT);
	PUSH_FIND_ORDER(L, -3, DOWN_UP_LEFT_RIGHT);
	PUSH_FIND_ORDER(L, -3, DOWN_UP_RIGHT_LEFT);
	PUSH_FIND_ORDER(L, -3, LEFT_RIGHT_UP_DOWN);
	PUSH_FIND_ORDER(L, -3, LEFT_RIGHT_DOWN_UP);
	PUSH_FIND_ORDER(L, -3, RIGHT_LEFT_UP_DOWN);
	PUSH_FIND_ORDER(L, -3, RIGHT_LEFT_DOWN_UP);
	lua_setfield(L, -2, "FIND_ORDER");

  return 1;
}

static inline void checkUserData(lua_State* L,int index)
{
	if(!lua_isuserdata(L,index))
		luaL_error(L,"arg %d , export userdata , now %s",index,luaL_typename(L,index));
};

template<class...Args>
inline static void checkCoordinates(Bitmap* bitmap,lua_State*L,Args && ...args)
{
	if (!isInBitmapScope(bitmap,std::forward<Args>(args)...))
		luaL_error(L,COORDINATES_OVERFLOW);
}

static inline auto ensureSimilarityAndToShift(lua_State*L,int index){
  auto sim = luaL_optnumber(L, index, 1);
  if(sim < 0 || sim > 1){
    luaL_error(L, "Similarity must be between 0 and 1");
  }
  return static_cast<int>((1-sim) * MAX_COLOR_SHIFT);
}

static auto ensureSimilarity(lua_State*L,int index){
  auto sim = luaL_optnumber(L, index, 1);
  if(sim < 0 || sim > 1){
    luaL_error(L, "Similarity must be between 0 and 1");
  }
  return 0.0;
}

static auto ensureFindOrder(lua_State*L,int index)->int{
  auto order = luaL_optinteger(L, index, 1);
  if(order >= 0 && order <= 8){
    return static_cast<int>(order);
  }
  luaL_error(L, "Order must be between 0 and 8");
  return 0;
}



static auto checkIntColor(lua_State*L,int index)->Color{
  auto v = luaL_checkinteger(L, index);
  if(v < 0 || v > 0xFFFFFF){
    luaL_error(L, "Invalid color value");
  }
  return v;
}


auto getColor(lua_State*L)->int{
	checkUserData(L, 1);
  auto bitmap = lua::toObject<Bitmap>(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	checkCoordinates(bitmap, L, x, y);
	Color color = autolua::getColor(bitmap, x, y);
	lua_pushinteger(L, (int)color);
	return 1;
}

auto getColorCount(lua_State*L)->int{
  checkUserData(L, 1);
  auto bitmap = lua::toObject<Bitmap>(L, 1);
  int x1 = luaL_checkinteger(L, 2);
  int y1 = luaL_checkinteger(L, 3);
  int x2 = luaL_checkinteger(L, 4);
  int y2 = luaL_checkinteger(L, 5);
  checkCoordinates(bitmap, L, x1, y1,x2,y2);
  auto shiftSum = ensureSimilarityAndToShift(L, 7);
  int count =0;
  if(lua_isinteger(L, 6)){
    Color color = checkIntColor(L, 6);
    count = getColorCount(bitmap, x1, y1, x2, y2, &color, shiftSum);
  }else if(lua_isstring(L,6)){
		size_t size = 0;
		auto *str = lua_tolstring(L, 6, &size);
    auto color = decodeColor(str , size);
    if(color == nullptr){
      luaL_error(L, "Invalid color string");
    }
    if(color->next == nullptr){
      switch (color->color.type) {
        case TColorType::ALONE:
          count = getColorCount(bitmap, x1, y1, x2, y2, (Color*)color->color.data, shiftSum);
          break;
        case TColorType::COLOR_GAMUT:
          count = getColorCount(bitmap, x1, y1, x2, y2, (ColorGamut*)color->color.data, shiftSum);
          break;
        case TColorType::NOT:
          count = getColorCount(bitmap, x1, y1, x2, y2, (ColorNot*)color->color.data, shiftSum);
          break;
        case TColorType::COLOR_GAMUT_NOT:
          count = getColorCount(bitmap, x1, y1, x2, y2, (ColorGamutNot*)color->color.data, shiftSum);
          break;
        default:
          break;
      }
    }else{
      count = getColorCount(bitmap, x1, y1, x2, y2, color, shiftSum);
    }
    freeColorComposition(color);
  }else{
    luaL_error(L, "Invalid color type");
  }
  lua_pushinteger(L, count);
  return 1;
}

auto isColor(lua_State*L)->int{
  checkUserData(L, 1);
  auto bitmap = lua::toObject<Bitmap>(L, 1);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  checkCoordinates(bitmap, L, x, y);
  auto shiftSum = ensureSimilarityAndToShift(L, 5);
  bool result = false;
  if(lua_isinteger(L, 4)){
    Color color = checkIntColor(L, 4);
    result = compareColor(bitmap, x, y, &color, shiftSum);
  }else if(lua_isstring(L,4)){
    size_t size = 0;
    auto *str = lua_tolstring(L, 4, &size);
    auto color = decodeColor(str , size);
    if(color == nullptr){
      luaL_error(L, "Invalid color string");
    }
    if(color->next == nullptr){
      switch (color->color.type) {
        case TColorType::ALONE:
          result = compareColor(bitmap, x, y, (Color*)color->color.data, shiftSum);
          break;
        case TColorType::COLOR_GAMUT:
          result = compareColor(bitmap, x, y, (ColorGamut*)color->color.data, shiftSum);
          break;
        case TColorType::NOT:
          result = compareColor(bitmap, x, y, (ColorNot*)color->color.data, shiftSum);
          break;
        case TColorType::COLOR_GAMUT_NOT:
          result = compareColor(bitmap, x, y, (ColorGamutNot*)color->color.data, shiftSum);
          break;
        default:
          break;
      }
    }else{
      result = compareColor(bitmap, x, y, color, shiftSum);
    }
    freeColorComposition(color);
  }else{
    luaL_error(L, "Invalid color type");
  }
  lua_pushboolean(L, result);
  return 1;
}


auto whichColor(lua_State*L)->int{
	checkUserData(L, 1);
	Bitmap* bitmap = lua::toObject<Bitmap>(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	checkCoordinates(bitmap, L, x, y);
  int shift = ensureSimilarityAndToShift(L, 5);
	int result = 0;
  if(lua_isinteger(L, 4)){
    Color color = checkIntColor(L, 4);
    result = compareColor(bitmap, x, y, &color, shift);
  }else if(lua_isstring(L,4)){
    size_t size = 0;
    auto *str = lua_tolstring(L, 4, &size);
    auto color = decodeColor(str , size);
    if(color == nullptr){
      luaL_error(L, "Invalid color string");
    }
    if(color->next == nullptr){
      switch (color->color.type) {
        case TColorType::ALONE:
          result = compareColor(bitmap, x, y, (Color*)color->color.data, shift);
          break;
        case TColorType::COLOR_GAMUT:
          result = compareColor(bitmap, x, y, (ColorGamut*)color->color.data, shift);
          break;
        case TColorType::NOT:
          result = compareColor(bitmap, x, y, (ColorNot*)color->color.data, shift);
          break;
        case TColorType::COLOR_GAMUT_NOT:
          result = compareColor(bitmap, x, y, (ColorGamutNot*)color->color.data, shift);
          break;
        default:
          break;
      }
    }else{
      result = compareColor(bitmap, x, y, color, shift);
    }
    freeColorComposition(color);
  }else{
    luaL_error(L, "Invalid color type");
  }

	lua_pushinteger(L, result);
	return 1;
}

auto findColor(lua_State*L)->int{
	checkUserData(L, 1);
	auto bitmap = lua::toObject<Bitmap>(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int x1 = luaL_checkinteger(L, 4);
	int y1 = luaL_checkinteger(L, 5);
	checkCoordinates(bitmap, L, x, y,x1,y1);
	int shift = ensureSimilarityAndToShift(L, 7);
	int order = ensureFindOrder(L, 8);
	Point out(-1,-1);
  bool result = false;
  if(lua_isinteger(L, 6)){
    Color color = checkIntColor(L, 6);
    result = findColor(bitmap, x, y, x1, y1, &color, shift, order, &out);
  }else if(lua_isstring(L,6)){
    size_t size = 0;
    auto *str = lua_tolstring(L, 6, &size);
    auto color = decodeColor(str , size);
    if(color == nullptr){
      luaL_error(L, "Invalid color string");
    }
    if(color->next == nullptr){
      switch (color->color.type) {
        case TColorType::ALONE:
          result = findColor(bitmap, x, y, x1, y1, (Color*)color->color.data, shift, order, &out);
          break;
        case TColorType::COLOR_GAMUT:
          result = findColor(bitmap, x, y, x1, y1, (ColorGamut*)color->color.data, shift, order, &out);
          break;
        case TColorType::NOT:
          result = findColor(bitmap, x, y, x1, y1, (ColorNot*)color->color.data, shift, order, &out);
          break;
        case TColorType::COLOR_GAMUT_NOT:
          result = findColor(bitmap, x, y, x1, y1, (ColorGamutNot*)color->color.data, shift, order, &out);
          break;
        default:
          break;
      }
    }else{
      result = findColor(bitmap, x, y, x1, y1, color, shift, order, &out);
    }
    freeColorComposition(color);
  }else{
    luaL_error(L, "Invalid color type");
  }
  if(!result){
    out.x = -1;
    out.y = -1;
  }
	lua_pushinteger(L, out.x);
	lua_pushinteger(L, out.y);
	return 2;
}

auto isFeature(lua_State*L)->int{
	checkUserData(L, 1);
	auto bitmap = lua::toObject<Bitmap>(L, 1);
	auto sim = ensureSimilarity(L, 3);
	size_t size = 0;
	const char * featureString = luaL_checklstring(L,2,&size);
  FeatureCompositionRoot feature;
  if(!decodeFeature(featureString, size, &feature)){
    luaL_error(L, "Invalid feature string");
  }
  auto shiftSum = (1-sim)*255*feature.count;
  bool result = isFeature(bitmap, &feature, shiftSum);
  freeFeatureComposition(&feature);
  lua_pushboolean(L, result);
	return 1;
}


auto findFeature(lua_State*L)->int{
  	luaL_checktype(L, 1, LUA_TUSERDATA);
	Bitmap* bitmap = lua::toObject<Bitmap>(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int x1 = luaL_checkinteger(L, 4);
	int y1 = luaL_checkinteger(L, 5);
	checkCoordinates(bitmap, L, x, y, x1, y1);
  auto sim = ensureSimilarity(L, 7);
  int direction = ensureFindOrder(L, 8);
	size_t featureSize = 0;
	const char * featureString = luaL_checklstring(L,6,&featureSize);
  FeatureCompositionRoot feature;
  if(!decodeFeature(featureString, featureSize, &feature)){
    luaL_error(L, "Invalid feature string");
  }
  auto shiftSum = (1-sim)*MAX_COLOR_SHIFT*feature.count;
  Point out(-1,-1);
  bool result = findFeature(bitmap, x, y, x1, y1, &feature, shiftSum, direction, &out);
  freeFeatureComposition(&feature);
  if(!result){
    out.x = -1;
    out.y = -1;
  }
	return 2;
}

auto loadImage(const std::string&path,std::string &cache, CommonBitmap*image)->bool{
  if(path.empty()){
    return false;
  }
  if(path[0] == std::filesystem::path::preferred_separator){
    return image->load(path.c_str());
  }
  if(resourceProvider == nullptr || !resourceProvider(path, cache)){
    return image->load(path.c_str());
  }
  return image->load((const unsigned char*)cache.data(), cache.size());
}

auto loadImages(const char*names,size_t size,std::vector<CommonBitmap>&images)->bool{
  size_t start = 0;
  std::string name;
  std::string data;
  for(size_t i = 0; i < size; i++){
    if(names[i] == '|'){
      auto &p = images.emplace_back();
      name = std::string(names + start, i - start);
      if(!loadImage(name, data, &p)){
        return false;
      }
      start = i + 1;
    }
  }
  if(start < size){
    auto &p = images.emplace_back();
    name = std::string(names + start, size - start);
    if(!loadImage(name, data, &p)){
      return false;
    }
  }
  return true;
}


auto isImage(lua_State*L)->int{
  checkUserData(L, 1);
  auto bitmap = lua::toObject<Bitmap>(L, 1);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  checkCoordinates(bitmap, L, x, y);
  auto sim = ensureSimilarity(L, 5);
  if(lua_isstring(L, 4)){
    size_t size = 0;
    const char*imageNames = luaL_checklstring(L, 4, &size);
    std::vector<CommonBitmap> images;
    if(!loadImages(imageNames, size, images)){
      images.~vector();
      luaL_error(L, "Invalid image string");
    }
    auto onePointShiftSum = (1-sim)*MAX_COLOR_SHIFT;
    for(auto&image:images){
      if(isImage(bitmap, x, y, &image ,image.width_*image.height_*onePointShiftSum)){
        lua_pushboolean(L, true);
        return 1;
      }
    }
    lua_pushboolean(L, false);
  }else{
    luaL_error(L, "Invalid image type");
  }
  return 1;
}

auto whichImage(lua_State*L)->int{
  checkUserData(L, 1);
  auto bitmap = lua::toObject<Bitmap>(L, 1);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  checkCoordinates(bitmap, L, x, y);
  auto sim = ensureSimilarity(L, 5);
  if(lua_isstring(L, 4)){
    size_t size = 0;
    const char*imageNames = luaL_checklstring(L, 4, &size);
    std::vector<CommonBitmap> images;
    if(!loadImages(imageNames, size, images)){
      images.~vector();
      luaL_error(L, "Invalid image string");
    }
    auto onePointShiftSum = (1-sim)*MAX_COLOR_SHIFT;
    for(size_t i = 0; i < images.size(); i++){
      auto& image = images.at(1);
      if(isImage(bitmap, x, y, &image ,image.width_*image.height_*onePointShiftSum)){
        lua_pushinteger(L, i+1);
        return 1;
      }
    }
    lua_pushinteger(L, -1);
  }else{
    luaL_error(L, "Invalid image type");
  }
  return 1;
}


class BitmapFinder{
  Bitmap * mBitmap;
  Bitmap* tBitmap;
  int mShiftSum;
  Point result;
public:
  BitmapFinder(Bitmap*bitmap,Bitmap*templateBitmap,int shiftSum)
    :mBitmap(bitmap),tBitmap(templateBitmap),mShiftSum(shiftSum){}
  bool compare(int x, int y, const unsigned char* color){
    if(isImage(mBitmap, x, y, tBitmap, mShiftSum)){
      result.x = x;
      result.y = y;
      return true;
    }
    return false;
  }
	Point& getResult(){
    return result;
  }
};

class BitmapsFinder{
  Bitmap * mBitmap;
  std::vector<CommonBitmap>* mImages;
  std::vector<int> mShiftSums;
  Point result;
public:
  BitmapsFinder(Bitmap*bitmap,std::vector<CommonBitmap>*images,int onePointShiftSum)
    :mBitmap(bitmap),mImages(images){
      for(auto&image:*images){
        mShiftSums.push_back(image.width_*image.height_*onePointShiftSum);
      }
    }
  bool compare(int x, int y, const unsigned char* color){
    for(size_t i = 0; i < mImages->size(); i++){
      if(isImage(mBitmap, x, y, &mImages->at(i), mShiftSums.at(i))){
        result.x = x;
        result.y = y;
        return true;
      }
    }
    return false;
  }
  Point& getResult(){
    return result;
  }
};

auto findImage(Bitmap*bitmap,int x,int y,int x1,int y1,CommonBitmap*image,int shiftSum,int direction,Point*out)->bool{
  BitmapFinder finder(bitmap, image, shiftSum);
	bool result = orderFindColor(bitmap, x, y, x1, y1, direction, &finder);
	if (result && out)
	{
		Point& point = finder.getResult();
		out->x = point.x;
		out->y = point.y;
	}
	return result;
}

auto findImage(Bitmap*bitmap,int x,int y,int x1,int y1,std::vector<CommonBitmap>*images,int onePointShift,int direction,Point*out)->bool{
  BitmapsFinder finder(bitmap, images, onePointShift);
  bool result = orderFindColor(bitmap, x, y, x1, y1, direction, &finder);
  if (result && out)
  {
    Point& point = finder.getResult();
    out->x = point.x;
    out->y = point.y;
  }
  return result;
}



auto findImage(lua_State*L)->int{
  checkUserData(L, 1);
  auto bitmap = lua::toObject<Bitmap>(L, 1);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int x1 = luaL_checkinteger(L, 4);
  int y1 = luaL_checkinteger(L, 5);
  checkCoordinates(bitmap, L, x, y, x1, y1);
  auto sim = ensureSimilarity(L, 7);
  int direction = ensureFindOrder(L, 8);
  if(lua_isstring(L, 6)){
    size_t size = 0;
    const char*imageNames = luaL_checklstring(L, 6, &size);
    std::vector<CommonBitmap> images;
    if(!loadImages(imageNames, size, images)){
      images.~vector();
      luaL_error(L, "Invalid image string");
    }
    auto onePointShiftSum = (1-sim)*MAX_COLOR_SHIFT;
    Point out(-1,-1);
    if(images.size() == 1){
      auto&image = images.at(0);
      if(findImage(bitmap, x, y, x1, y1, &image ,image.width_*image.height_*onePointShiftSum, direction, &out)){
        lua_pushinteger(L, out.x);
        lua_pushinteger(L, out.y);
        lua_pushinteger(L, 1);
        return 3;
      }
    }else{
      if(findImage(bitmap, x, y, x1, y1, &images ,onePointShiftSum, direction, &out)){
        lua_pushinteger(L, out.x);
        lua_pushinteger(L, out.y);
        lua_pushinteger(L, 1);
        return 3;
      }
    }
    lua_pushinteger(L, -1);
    lua_pushinteger(L, -1);
    lua_pushinteger(L, -1);
  }else{
    luaL_error(L, "Invalid image type");
  }
  return 3;
}


int loadImage(lua_State*L){
  size_t size = 0;
  const char* path = luaL_checklstring(L, 1, &size);
  std::string cPath  = std::string(path, size);
  std::string cache;
  auto image = luaL_pushNewObject(CommonBitmap,L);
  if(loadImage(cPath, cache, image)){
    return 1;
  }
  return 0;
}

int indexMethod(lua_State*L){
  lua_getmetatable(L, 1);
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);
  if(lua_isnil(L,-1)){
    lua_pop(L,1);
    lua_pushvalue(L,2);
    lua_gettable(L, -2);
    if(!lua_isnil(L, -1)){
      lua_pushvalue(L, 2);
      lua_pushvalue(L, -2);
      lua_rawset(L, 3);
    }
  }
  return 1;
}

int saveImageTo(lua_State*L){
  auto image = lua::toObject<Bitmap>(L, 1);
  size_t size = 0;
  const char* path = luaL_checklstring(L, 2, &size);
  std::string cPath  = std::string(path, size);
  auto error = lodepng::encode(cPath, image->origin_, image->width_, image->height_);
  if(error){
    lua_pushboolean(L, false);
    lua_pushstring(L, lodepng_error_text(error));
    return 2;
  }
  lua_pushboolean(L, true);
  return 1;
}

int cloneImage(lua_State*L){
  auto image = lua::toObject<Bitmap>(L, 1);
  auto x1 = luaL_optinteger(L, 2, 0);
  auto y1 = luaL_optinteger(L, 3, 0);
  auto x2 = luaL_optinteger(L, 4, image->width_-1);
  auto y2 = luaL_optinteger(L, 5, image->height_-1);
  checkCoordinates(image , L, x1,y1,x2,y2);
  auto newImage = luaL_pushNewObject(CommonBitmap, L);
  newImage->load(image, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  lua_pushboolean(L, true);
  return 1;
}


int getImageSize(lua_State*L){
  auto image = lua::toObject<Bitmap>(L, 1);
  lua_pushinteger(L, image->width_);
  lua_pushinteger(L, image->height_);
  return 2;
}