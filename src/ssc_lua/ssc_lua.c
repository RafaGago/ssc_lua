#include <stdlib.h>
#include <string.h>

#include <ssc_lua/ssc_lua.h>
#include <ssc_lua/lua_api.h>
#include <ssc_lua/lua_fibers.h>
#include <ssc_lua/log.h>
#include <ssc_lua/lpack.h>

#include <ssc/simulation/libexport.h>
#include <ssc/simulation/simulation.h>
#include <ssc/simulation/simulation_src.h>

#include <bl/base/time.h>
#include <bl/base/dynarray.h>
#include <bl/base/preprocessor_basic.h>
#include <bl/base/default_allocator.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifndef SSC_LUA_FIBER_STACK_BYTES
  #define SSC_LUA_FIBER_STACK_BYTES (64 * 1024)
#endif
/*----------------------------------------------------------------------------*/
/* Types */
/*----------------------------------------------------------------------------*/
typedef struct ssc_lua_global {
  lua_State* lua;
  lua_fibers fibers;
  alloc_tbl  alloc;
}
ssc_lua_global;
/*----------------------------------------------------------------------------*/
typedef enum fiber_ops_e {
  fop_yield,
  fop_wake,
  fop_wait,
  fop_delay,
  fop_produce_bytes,
  fop_produce_error,
  fop_produce_string,
  fop_consume,
  fop_consume_match,
  fop_consume_match_mask,
  fop_timed_consume,
  fop_timed_consume_match,
  fop_timed_consume_match_mask,
  fop_sim_consume_all,
  fop_set_fiber_as_produce_only,
  fop_drop_input_head_private,

  fop_out_of_scope,
}
fiber_ops_e;
typedef uword fiber_ops;
/*----------------------------------------------------------------------------*/
typedef struct op_wake {
  uword_d2 wait_id;
  uword_d2 count;
}
op_wake;
/*----------------------------------------------------------------------------*/
typedef struct op_wait {
  bool*    timedout;
  uword_d2 wait_id;
  toffset  us;
}
op_wait;
/*----------------------------------------------------------------------------*/
typedef struct op_delay {
  toffset us;
}
op_delay;
/*----------------------------------------------------------------------------*/
typedef struct op_produce_bytes {
  u8 const* dat;
  u16       dat_size;
}
op_produce_bytes;
/*----------------------------------------------------------------------------*/
typedef struct op_produce_error {
  bl_err err;
}
op_produce_error;
/*----------------------------------------------------------------------------*/
typedef struct op_produce_string {
  char const* str;
  u16         str_size_no_null;
}
op_produce_string;
/*----------------------------------------------------------------------------*/
typedef struct op_consume {
  u8**    dat;
  u16*    dat_size;
}
op_consume;
/*----------------------------------------------------------------------------*/
typedef struct op_consume_match {
  u8**      dat;
  u16*      dat_size;
  u8 const* match;
  u16       match_size;
}
op_consume_match;
/*----------------------------------------------------------------------------*/
typedef struct op_consume_match_mask {
  u8**      dat;
  u16*      dat_size;
  u8 const* match;
  u16       match_size;
  u8 const* mask;
  u16       mask_size;
}
op_consume_match_mask;
/*----------------------------------------------------------------------------*/
typedef struct op_timed_consume {
  u8**    dat;
  u16*    dat_size;
  toffset us;
}
op_timed_consume;
/*----------------------------------------------------------------------------*/
typedef struct op_timed_consume_match {
  u8**      dat;
  u16*      dat_size;
  u8 const* match;
  u16       match_size;
  toffset   us;
}
op_timed_consume_match;
/*----------------------------------------------------------------------------*/
typedef struct op_timed_consume_match_mask {
  u8**      dat;
  u16*      dat_size;
  u8 const* match;
  u16       match_size;
  u8 const* mask;
  u16       mask_size;
  toffset   us;
}
op_timed_consume_match_mask;
/*----------------------------------------------------------------------------*/
typedef union op_data {
  op_wake                     wake;
  op_wait                     wait;
  op_delay                    delay;
  op_produce_bytes            produce_bytes;
  op_produce_error            produce_error;
  op_produce_string           produce_string;
  op_consume                  consume;
  op_consume_match            consume_m;
  op_consume_match_mask       consume_mm;
  op_timed_consume            consume_t;
  op_timed_consume_match      consume_tm;
  op_timed_consume_match_mask consume_tmm;
}
op_data;
/*----------------------------------------------------------------------------*/
typedef struct ssc_lua_handle {
  ssc_handle h;
  fiber_ops  op;
  op_data    d;
}
ssc_lua_handle;
/*----------------------------------------------------------------------------*/
typedef struct sim_tstamp {
  u8 t[sizeof (tstamp)];
}
sim_tstamp;
/*----------------------------------------------------------------------------*/
/* C Api impl */
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_yield (void* h)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  lh->op = fop_yield;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_wake (void* h, uword_d2 wait_id, uword_d2 count)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  lh->op             = fop_wake;
  lh->d.wake.wait_id = wait_id;
  lh->d.wake.count   = count;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_wait(
  void* h, bool* timedout, uword_d2 wait_id, toffset us
  )
{
  ssc_lua_handle* lh  = (ssc_lua_handle*) h;
  lh->op              = fop_wait;
  lh->d.wait.wait_id  = wait_id;
  lh->d.wait.us       = us;
  lh->d.wait.timedout = timedout;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_delay (void* h, toffset us)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  lh->op             = fop_delay;
  lh->d.delay.us     = us;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT sim_tstamp sim_timestamp_get (void* h)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  tstamp r = ssc_get_timestamp (lh->h);
  return *((sim_tstamp*) &r);
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT sim_tstamp sim_timestamp_add_usec(
  sim_tstamp t, toffset us
  )
{
  tstamp r = *((tstamp*) &t);
  r       += bl_usec_to_tstamp (us);
  return *((sim_tstamp*) &r);
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT toffset sim_timestamp_diff_usec(
  sim_tstamp future, sim_tstamp past
  )
{
  return bl_tstamp_to_usec (tstamp_get_diff(
    *((tstamp*) &future), *((tstamp*) &past)
    ));
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT bool sim_timestamp_is_expired(
  sim_tstamp candidate, sim_tstamp deadline
  )
{
  return tstamp_get_diff(
    *((tstamp*) &candidate), *((tstamp*) &deadline)
    ) >= 0;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_produce_bytes(
  void* h, u8 const* dat, u16 dat_size
  )
{
  ssc_lua_handle* lh           = (ssc_lua_handle*) h;
  lh->op                       = fop_produce_bytes;
  lh->d.produce_bytes.dat      = dat;
  lh->d.produce_bytes.dat_size = dat_size;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_produce_error (void* h, bl_err err)
{
  ssc_lua_handle* lh      = (ssc_lua_handle*) h;
  lh->op                  = fop_produce_error;
  lh->d.produce_error.err = err;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_produce_string(
  void* h, char const* str, u16 str_size_no_null
  )
{
  ssc_lua_handle* lh                    = (ssc_lua_handle*) h;
  lh->op                                = fop_produce_string;
  lh->d.produce_string.str              = str;
  lh->d.produce_string.str_size_no_null = str_size_no_null;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_consume(
  void* h, u8** dat, u16* dat_size
  )
{
  ssc_lua_handle* lh     = (ssc_lua_handle*) h;
  lh->op                 = fop_consume;
  lh->d.consume.dat      = dat;
  lh->d.consume.dat_size = dat_size;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_consume_match(
  void* h, u8** dat, u16* dat_size, u8 const* match, u16 match_size
  )
{
  ssc_lua_handle* lh         = (ssc_lua_handle*) h;
  lh->op                     = fop_consume_match;
  lh->d.consume_m.dat        = dat;
  lh->d.consume_m.dat_size   = dat_size;
  lh->d.consume_m.match      = match;
  lh->d.consume_m.match_size = match_size;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_consume_match_mask(
  void*     h,
  u8**      dat,
  u16*      dat_size,
  u8 const* match,
  u16       match_size,
  u8 const* mask,
  u16       mask_size
  )
{
  ssc_lua_handle* lh        = (ssc_lua_handle*) h;
  lh->op                      = fop_consume_match_mask;
  lh->d.consume_mm.dat        = dat;
  lh->d.consume_mm.dat_size   = dat_size;
  lh->d.consume_mm.match      = match;
  lh->d.consume_mm.match_size = match_size;
  lh->d.consume_mm.mask       = mask;
  lh->d.consume_mm.mask_size  = mask_size;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_timed_consume(
  void* h, u8** dat, u16* dat_size, toffset us
  )
{
  ssc_lua_handle* lh       = (ssc_lua_handle*) h;
  lh->op                   = fop_timed_consume;
  lh->d.consume_t.dat      = dat;
  lh->d.consume_t.dat_size = dat_size;
  lh->d.consume_t.us       = us;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_timed_consume_match(
  void* h, u8** dat, u16* dat_size, u8 const* match, u16 match_size, toffset us
  )
{
  ssc_lua_handle* lh          = (ssc_lua_handle*) h;
  lh->op                      = fop_timed_consume_match;
  lh->d.consume_tm.dat        = dat;
  lh->d.consume_tm.dat_size   = dat_size;
  lh->d.consume_tm.match      = match;
  lh->d.consume_tm.match_size = match_size;
  lh->d.consume_tm.us         = us;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_timed_consume_match_mask(
  void*     h,
  u8**      dat,
  u16*      dat_size,
  u8 const* match,
  u16       match_size,
  u8 const* mask,
  u16       mask_size,
  toffset   us
  )
{
  ssc_lua_handle* lh           = (ssc_lua_handle*) h;
  lh->op                       = fop_timed_consume_match_mask;
  lh->d.consume_tmm.dat        = dat;
  lh->d.consume_tmm.dat_size   = dat_size;
  lh->d.consume_tmm.match      = match;
  lh->d.consume_tmm.match_size = match_size;
  lh->d.consume_tmm.mask       = mask;
  lh->d.consume_tmm.mask_size  = mask_size;
  lh->d.consume_tmm.us         = us;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_consume_all (void* h)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  lh->op = fop_sim_consume_all;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_set_fiber_as_produce_only (void* h)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  lh->op = fop_set_fiber_as_produce_only;
}
/*----------------------------------------------------------------------------*/
BL_VISIBILITY_DEFAULT void sim_drop_input_head_private (void* h)
{
  ssc_lua_handle* lh = (ssc_lua_handle*) h;
  lh->op = fop_drop_input_head_private;
}
/*----------------------------------------------------------------------------*/
/* Fiber */
/*----------------------------------------------------------------------------*/
static void fiber_function(
  ssc_handle h, void* fiber_context, void* sim_context
  )
{
  ssc_lua_global* gc       = (ssc_lua_global*) sim_context;
  lua_fiber_data const* fd = (lua_fiber_data const*) fiber_context;

  ssc_lua_handle lh;
  lh.h = h;

  while (1) {
    lua_pushlightuserdata (fd->thread, &lh);
  	lua_setglobal (fd->thread, "current_fiber_handle"); /*setfenv?*/

    lh.op = fop_out_of_scope;

    int resume_status = lua_resume (fd->thread, 0);
    if (resume_status != 0 && resume_status != LUA_YIELD) {
      fprintf(
        stderr,
        "terminating fiber \"%s\" because of Lua runtime error:\n%s\n",
        fd->name,
        lua_tostring (fd->thread, -1)
        );
      log_error ("error on fiber: %s", fd->name);
      goto fiber_end;
    }
    switch (lh.op) {
    case fop_yield: {
      ssc_yield (h);
      break;
    }
    case fop_wake: {
      ssc_wake (h, lh.d.wake.wait_id, lh.d.wake.count);
      break;
    }
    case fop_wait: {
      *lh.d.wait.timedout = ssc_wait(
        h, lh.d.wait.wait_id, lh.d.wait.us
        );
      break;
    }
    case fop_delay: {
      ssc_delay (h, lh.d.delay.us);
      break;
    }
    case fop_produce_bytes: {
      memr16 o = memr16_rv(
        bl_alloc (&gc->alloc, lh.d.produce_bytes.dat_size),
        lh.d.produce_bytes.dat_size
        );
      if (!memr16_beg (o)) {
        log_error ("error allocating output, terminating fiber %s", name);
        goto fiber_end;
      }
      memcpy (memr16_beg (o), lh.d.produce_bytes.dat, memr16_size (o));
      ssc_produce_dynamic_output (h, o);
      break;
    }
    case fop_produce_error: {
      ssc_produce_error (h, lh.d.produce_error.err, nullptr);
      break;
    }
    case fop_produce_string: {
      u16   size   = lh.d.produce_string.str_size_no_null;
      char* str_cp = (char*) bl_alloc (&gc->alloc, size + 1);
      if (!str_cp) {
        log_error ("error allocating string, terminating fiber %s", name);
        goto fiber_end;
      }
      memcpy (str_cp, lh.d.produce_string.str, size);
      str_cp[size] = 0;
      ssc_produce_dynamic_string (h, str_cp, size + 1);
      break;
    }
    case fop_consume: {
      memr16 r = ssc_peek_input_head (h);
      *lh.d.consume.dat      = (u8*) memr16_beg (r);
      *lh.d.consume.dat_size = memr16_size (r);
      break;
    }
    case fop_consume_match: {
      memr16 r = ssc_peek_input_head_match(
        h, memr16_rv ((u8*) lh.d.consume_m.match, lh.d.consume_m.match_size)
        );
      *lh.d.consume_m.dat      = (u8*) memr16_beg (r);
      *lh.d.consume_m.dat_size = memr16_size (r);
      break;
    }
    case fop_consume_match_mask: {
      memr16 r = ssc_peek_input_head_match_mask(
        h,
        memr16_rv ((u8*) lh.d.consume_mm.match, lh.d.consume_mm.match_size),
        memr16_rv ((u8*) lh.d.consume_mm.mask, lh.d.consume_mm.mask_size)
        );
      *lh.d.consume_mm.dat      = (u8*) memr16_beg (r);
      *lh.d.consume_mm.dat_size = memr16_size (r);
      break;
    }
    case fop_timed_consume: {
      memr16 r = ssc_timed_peek_input_head (h, lh.d.consume_t.us);
      *lh.d.consume_t.dat      = (u8*) memr16_beg (r);
      *lh.d.consume_t.dat_size = memr16_size (r);
      break;
    }
    case fop_timed_consume_match: {
      memr16 r = ssc_timed_peek_input_head_match(
        h,
        memr16_rv ((u8*) lh.d.consume_tm.match, lh.d.consume_tm.match_size),
        lh.d.consume_tm.us
        );
      *lh.d.consume_tm.dat      = (u8*) memr16_beg (r);
      *lh.d.consume_tm.dat_size = memr16_size (r);
      break;
    }
    case fop_timed_consume_match_mask: {
      memr16 r = ssc_timed_peek_input_head_match_mask(
        h,
        memr16_rv ((u8*) lh.d.consume_tmm.match, lh.d.consume_tmm.match_size),
        memr16_rv ((u8*) lh.d.consume_tmm.mask, lh.d.consume_tmm.mask_size),
        lh.d.consume_tmm.us
        );
      *lh.d.consume_tmm.dat      = (u8*) memr16_beg (r);
      *lh.d.consume_tmm.dat_size = memr16_size (r);
      break;
    }
    case fop_sim_consume_all: {
      ssc_drop_all_input (h);
      break;
    }
    case fop_set_fiber_as_produce_only: {
      ssc_set_fiber_as_produce_only (h);
      break;
    }
    case fop_drop_input_head_private: {
      ssc_drop_input_head (h);
      break;
    }
    default: goto fiber_end;
    } /*switch*/
  }
fiber_end:
  luaL_unref (gc->lua, LUA_REGISTRYINDEX, fd->thread_ref);
  return;
}
/*----------------------------------------------------------------------------*/
/* ssc functions */
/*----------------------------------------------------------------------------*/
SSC_EXPORT
  void ssc_sim_dealloc(
    void const* mem, uword size, ssc_group_id id, void* sim_context
    )
{
  ssc_lua_global* gc = (ssc_lua_global*) sim_context;
  bl_dealloc (&gc->alloc, mem);
}
/*----------------------------------------------------------------------------*/
SSC_EXPORT
  bl_err ssc_sim_on_setup(
    ssc_handle h, void* passed_data, void** sim_context
    )
{
  bl_assert (passed_data && sim_context);
  ssc_lua_global* gc      = (ssc_lua_global*) malloc (sizeof *gc);
  ssc_lua_passed_data* pd = (ssc_lua_passed_data*) passed_data;
  *sim_context            = nullptr;

  if (!gc) {
    return bl_alloc;
  }
  gc->alloc  = get_default_alloc();
  bl_err err = lua_fibers_init (&gc->fibers, &gc->alloc);
  if (err) {
    goto global_free;
  }
  err     = bl_error;
  gc->lua = luaL_newstate();
  luaL_openlibs (gc->lua);
  luaopen_pack (gc->lua);

#ifndef DSRST_LUA_NO_API_FUNCTIONS
  /*open and execute api*/
  int status = luaL_loadstring (gc->lua, ssc_lua_api);
  assert (status == 0);
  status     = lua_pcall (gc->lua, 0, 0, 0);
  assert (status == 0);
#else
  int status;
#endif
  /*letting the user to modify the lua state before running the main script*/
  if (pd->before_main_script) {
    status = pd->before_main_script (gc->lua, pd->on_init_context);
    if (status) {
      log_error ("error running \"before_main_script\" function");
      goto close;
    }
  }
  /*open and execute main simulation script*/
  status = luaL_loadfile (gc->lua, pd->main_script_path);
  if (status) {
    fprintf (stderr, "%s\n", lua_tostring (gc->lua, -1));
    log_error ("error on user script file");
    goto close;
  }
  if (lua_pcall (gc->lua, 0, 0, 0) != 0) {
    fprintf(
      stderr, "error parsing user script: %s\n", lua_tostring (gc->lua, -1)
      );
    log_error ("error parsing user script");
    goto close;
  }
  /*retrieve groups with its fibers*/
  lua_getglobal (gc->lua, "ssc_fibers_private");
  if (!lua_istable (gc->lua, -1)) {
    fprintf (stderr, "can't find ssc_fibers (not a table?)\n");
    log_error ("ssc_fibers error");
    goto close;
  }
  uword group = 0;

  lua_pushnil (gc->lua);
  while (lua_next (gc->lua, -2) != 0) {
    bl_assert (lua_istable (gc->lua, -1));

    lua_pushnil (gc->lua);
    while (lua_next (gc->lua, -2) != 0) {
      bl_assert (lua_istable (gc->lua, -1));

      /*retrieving fiber string tag*/
      lua_pushnil (gc->lua);
      bl_assert_side_effect (lua_next (gc->lua, -2) != 0);
      bl_assert (lua_isstring (gc->lua, -1));
      char const* name = lua_tostring (gc->lua, -1);
      lua_pop (gc->lua, 1);

      /*retrieving fiber function and associating it to a thread*/
      bl_assert_side_effect (lua_next (gc->lua, -2) != 0);
      lua_State* thr = lua_newthread (gc->lua);
      if (!thr) {
        log_error ("allocation error on lua_newthread");
        goto destroy_lua_fibers;
      }
      bl_assert (lua_isthread (gc->lua, -1));
      int thr_ref = luaL_ref (gc->lua, LUA_REGISTRYINDEX);
      bl_assert (lua_isfunction (gc->lua, -1));
      lua_xmove (gc->lua, thr, 1); /*ready to next resume invocation*/

      bl_assert_side_effect (lua_next (gc->lua, -2) == 0);/*end of table*/

      /*saving fiber data*/
      lua_fiber_data* fiber_data = lua_fibers_add(
        &gc->fibers, thr, thr_ref, group, name, &gc->alloc
        );
      if (!fiber_data) {
        log_error ("allocation error on lua_fibers_add");
        goto destroy_lua_fibers;
      }
      lua_pop (gc->lua, 1);
    }
    lua_pop (gc->lua, 1);
    ++group;
  }
  lua_pop (gc->lua, 1);

  /*configure all ssc_fibers*/
  lua_fiber_list const* df = lua_fibers_get_list (&gc->fibers);
  dynarray_foreach_const (lua_fiber_list, lua_fiber_data, df, it)
  {
    ssc_fiber_cfg cfg = ssc_fiber_cfg_rv(
      it->gid, fiber_function, nullptr, nullptr, (void*) it
      );
    cfg.min_stack_size = SSC_LUA_FIBER_STACK_BYTES;
    if (ssc_add_fiber (h, &cfg) != bl_ok) {
      log_error ("error adding fiber");
      goto destroy_lua_fibers;
    }
  }
  /*letting the user to read the lua state (intended to be able to retrieve
    data) after running the main script*/
  if (pd->after_main_script) {
    status = pd->after_main_script (gc->lua, pd->on_init_context);
    if (status) {
      log_error ("error running \"after_main_script\" function");
      goto close;
    }
  }
  /*run ssc_setup if exists*/
  lua_getglobal (gc->lua, "ssc_setup");
  if (lua_isfunction (gc->lua, -1)) {
    if (lua_pcall (gc->lua, 0, 0, 0) != 0) {
      fprintf(
        stderr, "error running ssc_setup: %s\n", lua_tostring (gc->lua, -1)
        );
      log_error ("error running ssc_setup");
      goto destroy_lua_fibers;
    }
  }
  else {
    log_notice ("script without ssc_setup");
    lua_pop (gc->lua, 1);
  }
  err          = bl_ok;
  *sim_context = (void*) gc;
  return err;

destroy_lua_fibers:
  lua_fibers_destroy (&gc->fibers, &gc->alloc);
close:
  lua_close (gc->lua);
  gc->lua = nullptr;
global_free:
  free (gc);
  return err;
}
/*----------------------------------------------------------------------------*/
SSC_EXPORT
  void ssc_sim_on_teardown (void* sim_context)
{
  ssc_lua_global* gc = (ssc_lua_global*) sim_context;
  /*TODO: async log flush if needed */
  /*run ssc_teardown if exists*/
  lua_getglobal (gc->lua, "ssc_teardown");
  if (lua_isfunction (gc->lua, -1)) {
    if (lua_pcall (gc->lua, 0, 0, 0) != 0) {
      fprintf(
        stderr, "error running ssc_teardown: %s\n", lua_tostring (gc->lua, -1)
        );
      log_error ("error running ssc_teardown");
    }
  }
  else {
    log_notice ("script without ssc_teardown");
    lua_pop (gc->lua, 1);
  }
  /*destruction*/
  lua_fibers_destroy (&gc->fibers, &gc->alloc);
  lua_close (gc->lua);
  free (gc);
}
/*----------------------------------------------------------------------------*/
#ifdef SSC_BEFORE_FIBER_CONTEXT_SWITCH_EVT
SSC_EXPORT
  void ssc_sim_before_fiber_context_switch (void* sim_context)
{
  /*ssc_lua_global* gc = (ssc_lua_global*) sim_context;*/
}
#endif
/*----------------------------------------------------------------------------*/

