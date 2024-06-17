#ifndef __LUA_VISION_H__
#define __LUA_VISION_H__


#include <functional>
#include <lua.hpp>
#include <string>

using ResourceProvider = std::function<bool (const std::string path, std::string&)>;
using CompareColorMethodReceiver = void (*)(const char *, lua_CFunction, void *);
auto setResourceProvider(ResourceProvider provider) -> void;
void eachCompareColorMethod(CompareColorMethodReceiver receiver, void*data);
#ifdef __cplusplus
extern "C"
{
#endif


auto luaopen_alv(struct lua_State*L) -> int;
void injectMethodTo(struct lua_State*L,int tableIndex);

#ifdef __cplusplus
}
#endif
#endif // __LUA_VISION_H__