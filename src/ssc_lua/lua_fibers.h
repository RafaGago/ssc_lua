#ifndef __SSC_LUA_FIBERS_H__
#define __SSC_LUA_FIBERS_H__

#include <lua.h>
#include <bl/base/integer.h>
#include <bl/base/error.h>
#include <bl/base/dynarray.h>
#include <bl/base/allocator.h>
#include <ssc/types.h>

/*----------------------------------------------------------------------------*/
typedef struct lua_fiber_data {
  lua_State*   thread;
  int          thread_ref;
  char const*  name;
  ssc_group_id gid;
}
lua_fiber_data;
/*----------------------------------------------------------------------------*/
bl_define_dynarray_types (lua_fiber_list, lua_fiber_data)
/*----------------------------------------------------------------------------*/
bl_declare_dynarray_funcs (lua_fiber_list, lua_fiber_data)
/*----------------------------------------------------------------------------*/
typedef struct lua_fibers {
 lua_fiber_list list;
}
lua_fibers;
/*----------------------------------------------------------------------------*/
extern bl_err lua_fibers_init (lua_fibers* f, bl_alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void lua_fibers_destroy (lua_fibers* f, bl_alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern lua_fiber_data* lua_fibers_add(
  lua_fibers*         f,
  lua_State*          thread,
  int                 thread_ref,
  ssc_group_id        gid,
  char const*         name,
  bl_alloc_tbl const* alloc
  );
/*----------------------------------------------------------------------------*/
static inline lua_fiber_list const* lua_fibers_get_list (lua_fibers* f)
{
  return &f->list;
}
/*----------------------------------------------------------------------------*/

#endif /* __SSC_LUA_FIBER_DATA_H__ */

