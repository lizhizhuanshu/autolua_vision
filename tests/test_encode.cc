

#include <gtest/gtest.h>

#include "vision_color.h"
#include "vision_feature.h"

auto decodeColor(const std::string& str)  {
  return autolua::decodeColor(str.c_str(), str.size());
}

TEST(Encode, decodeColor) {
  auto color = decodeColor("000000");
  ASSERT_EQ(color->color.type, autolua::TColorType::ALONE);
  ASSERT_EQ(color->color.data[0], 0);
  ASSERT_EQ(color->color.data[1], 0);
  ASSERT_EQ(color->color.data[2], 0);
  autolua::freeColorComposition(color);

  color = decodeColor("FF00FF");
  ASSERT_EQ(color->color.type, autolua::TColorType::ALONE);
  ASSERT_EQ(color->color.data[0], 255);
  ASSERT_EQ(color->color.data[1], 0);
  ASSERT_EQ(color->color.data[2], 255);
  autolua::freeColorComposition(color);

  color = decodeColor("FF00FF-FF00FF");
  ASSERT_EQ(color->color.type, autolua::TColorType::COLOR_GAMUT);
  auto gamut = (autolua::ColorGamut*)color->color.data;
  ASSERT_EQ(gamut->shift, 0xFF00FF);
  ASSERT_EQ(gamut->color, 0xFF00FF);
  autolua::freeColorComposition(color);

  color = decodeColor("FF00FF-FF00FF|000000");
  ASSERT_EQ(color->color.type, autolua::TColorType::COLOR_GAMUT);
  gamut = (autolua::ColorGamut*)color->color.data;
  ASSERT_EQ(gamut->shift, 0xFF00FF);
  ASSERT_EQ(gamut->color, 0xFF00FF);
  auto two = color->next;
  ASSERT_EQ(two->color.type, autolua::TColorType::ALONE);
  ASSERT_EQ(two->color.data[0], 0);
  ASSERT_EQ(two->color.data[1], 0);
  ASSERT_EQ(two->color.data[2], 0);
  autolua::freeColorComposition(color);

}

TEST(Encode,decodeFeature){
  std::string str = "0|0|000000,1|3|FF00FF-FF00FF|100208";
  autolua::FeatureCompositionRoot root;
  ASSERT_TRUE(autolua::decodeFeature(str.c_str(), str.size(), &root));
  ASSERT_EQ(root.count, 2);
  ASSERT_EQ(root.data->x, 0);
  ASSERT_EQ(root.data->y, 0);
  ASSERT_EQ(root.data->color->color.type, autolua::TColorType::ALONE);
  auto next = root.data->next;
  ASSERT_EQ(next->x, 1);
  ASSERT_EQ(next->y, 3);
  ASSERT_EQ(next->color->color.type, autolua::TColorType::COLOR_GAMUT);
  auto gamut = (autolua::ColorGamut*)next->color->color.data;
  ASSERT_EQ(gamut->shift, 0xFF00FF);
  ASSERT_EQ(gamut->color, 0xFF00FF);
  std::transform(str.begin(), str.end(), str.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
  ASSERT_EQ(autolua::encodeFeature(&root), str);
  autolua::freeFeatureComposition(&root);
}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
