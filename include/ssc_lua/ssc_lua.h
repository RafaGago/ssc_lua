#ifndef __SSC_LUA_H__
#define __SSC_LUA_H__

#include <ssc/simulator/simulator.h>

struct lua_State;
/*----------------------------------------------------------------------------*/
typedef int (*ssc_lua_on_init_func) (struct lua_State* state, void* context);
/*----------------------------------------------------------------------------*/
/*
  ssc_lua_passed_data struct: data to pass on the ssc_create
  "simlib_passed_data" parameter to initialize "ssc_lua". It containts the next
  fields:

  "main_script_path":   Path to the simulation script.
  "before_main_script": This function will be called before running the main
                        script. As it passes the current "lua_State" it does
                        allow the user to add 3rd party libraries coded C to
                        the Lua enviroment.
  "after_main_script":  Same as the above function, but running after the
                        simulation script is loaded. It is intended to let the
                        user to retrieve data from the final environment. It
                        can be used to import Lua files, but this could be done
                        from the script too.
  "on_init_context":    void* to forward to the pair of functions above.
  "min_stack_bytes":    SSC stack size of each fiber. If weird segfaults happen
                        you might need to increment this. A good start on x64
                        for debug compiled executables is around 256KB.
*/
/*----------------------------------------------------------------------------*/
typedef struct ssc_lua_passed_data {
  char const*          main_script_path;
  ssc_lua_on_init_func before_main_script;
  ssc_lua_on_init_func after_main_script;
  void*                on_init_context;
  uword                min_stack_bytes;
}
ssc_lua_passed_data; /**/
/*----------------------------------------------------------------------------*/
#endif /* __SSC_LUA_H__ */
