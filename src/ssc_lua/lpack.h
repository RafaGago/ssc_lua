#include "lua.h"

extern int luaopen_pack(lua_State *L);

/*
local test = string.pack ("QQQs", 1, 2333, 3333, "pedro")
idx, num1, num2, num3, pedro = test:unpack ("QQQs")
print (num1)
print (num2)
print (num3)
print (pedro)
 */
