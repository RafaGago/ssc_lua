#ifndef __SSC_LUA_FIBER_DATA_H__
#define __SSC_LUA_FIBER_DATA_H__

#include <string.h>
#include <bl/base/preprocessor_basic.h>
#include <ssc_lua/lua_fibers.h>
/*----------------------------------------------------------------------------*/
bl_err lua_fibers_init(
  lua_fibers* f, alloc_tbl const* alloc
  )
{
  return lua_fiber_list_init (&f->list, 0, alloc);
}
/*----------------------------------------------------------------------------*/
void lua_fibers_destroy(
  lua_fibers* f, alloc_tbl const* alloc
  )
{
  dynarray_foreach (lua_fiber_list, lua_fiber_data, &f->list, it) {
    bl_dealloc (alloc, it->name);
  }
  lua_fiber_list_destroy (&f->list, alloc);
}
/*----------------------------------------------------------------------------*/
lua_fiber_data* lua_fibers_add(
  lua_fibers*      f,
  lua_State*       thread,
  int              thread_ref,
  ssc_group_id     gid,
  char const*      name,
  alloc_tbl const* alloc
  )
{
  uword len = strlen (name) + 1;
  char* name_copy = (char*) bl_alloc (alloc, len);
  if (!name_copy) {
    return nullptr;
  }
  if (lua_fiber_list_grow (&f->list, 1, alloc).bl != bl_ok) {
    bl_dealloc (alloc, name_copy);
    return nullptr;
  }
  memcpy (name_copy, name, len);
  lua_fiber_data* last = lua_fiber_list_last (&f->list);
  last->name           = name_copy;
  last->thread         = thread;
  last->thread_ref     = thread_ref;
  last->gid            = gid;
  return last;
}
/*----------------------------------------------------------------------------*/
#endif /* __SSC_LUA_FIBER_DATA_H__ */

