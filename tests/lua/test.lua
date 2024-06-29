
local suffix = ""
if package.cpath:find(".so",1,true) then
  suffix = ".so"
else
  suffix = ".dll"
end

package.cpath = package.cpath .. ";./build/?" .. suffix


local alv = require("alv")

local image = alv.loadImage("./tests/lua/test.png")

local function testGetColor()
  local color = image:getColor(202,174)
  assert(color ==  0xefb020)
end

local function testIsFeature()
  print("start test isFeature")
  local feature = "141|68|7C9E27,138|81|567927,156|96|7C9E27,167|87|587B27,164|75|5A7D27,146|62|799B27,146|56|94BA36,156|63|557827,166|65|94BA36,181|89|94BA36,189|75|B2D043,182|64|94BA36"
  assert(image:isFeature(feature,1))
end

local function testFindFeature()
  print("start test findFeature")
  local feature = "0|0|09093E,-7|6|5F499B,-26|15|52318F,-6|21|52318F,4|8|0C0C3F,7|20|52318F,34|2|B4348D,48|-5|AD338E,52|-10|B3338D,53|-7|5E499B,58|5|51308F,60|2|B3338D,64|-1|5E499B"
  local x,y = image:findFeature(49,27,454,340,feature,1,1)
  assert(x == 262 and y == 85)
end


testGetColor()
testIsFeature()
testFindFeature()





