
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



testGetColor()







