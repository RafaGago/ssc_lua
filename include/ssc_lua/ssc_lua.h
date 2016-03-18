#ifndef __SSC_LUA_H__
#define __SSC_LUA_H__

/*this is the data to pass to the similator on "ssc_create" as the
 "simlib_passed_data" parameter */

#include <ssc/simulator/simulator.h>

struct lua_State;
/*----------------------------------------------------------------------------*/
typedef int (*ssc_lua_on_init_func) (struct lua_State* state, void* context);
/*----------------------------------------------------------------------------*/
typedef struct ssc_lua_passed_data {
  char const*          main_script_path; /*path to the simulation script*/
  ssc_lua_on_init_func before_main_script;
  ssc_lua_on_init_func after_main_script;
  void*                on_init_context;
}
ssc_lua_passed_data; /*data to pass on the ssc_create "simlib_passed_data"
                         parameter*/
/*----------------------------------------------------------------------------*/
#endif /* __SSC_LUA_H__ */
