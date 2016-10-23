--------------------------------------------------------------------------------
ffi = require("ffi")
ffi.cdef[[
  typedef uint8_t   u8;
  typedef int8_t    i8;
  typedef uint16_t  u16;
  typedef int16_t   i16;
  typedef uint32_t  u32;
  typedef int32_t   i32;
  typedef uint64_t  u64;
  typedef int64_t   i64;
  typedef uintptr_t uword;
  typedef intptr_t  word;
  typedef void*     ssc_handle;
  typedef uword     bl_err;
  typedef u32       toffset;

  typedef struct sim_tstamp {
    u8 t[sizeof (u32)];
  }
  sim_tstamp;

  void sim_yield (void* h);
  void sim_delay (void* h, toffset us);

  sim_tstamp sim_timestamp_get (void* h);
  sim_tstamp sim_timestamp_add_usec (sim_tstamp t, toffset us);
  toffset    sim_timestamp_diff_usec (sim_tstamp future, sim_tstamp past);
  bool       sim_timestamp_is_expired (sim_tstamp cmp, sim_tstamp deadline);

  void sim_wake (void* h, u16 wait_id, u16 count);
  bool sim_wait (void* h, bool* timedout, u16 wait_id, toffset us);

  void sim_produce_bytes (void* h, u8 const* dat, u16 dat_size);
  void sim_produce_error (void* h, bl_err err);
  void sim_produce_string (void* h, char const* str, u16 str_size_no_null);

  void sim_consume (void* h, u8** dat, u16* dat_size);
  void sim_consume_match(
    void*     h,
    u8**      dat,
    u16*      dat_size,
    u8 const* match,
    u16       match_size
    );
  void sim_consume_match_mask(
    void*     h,
    u8**      dat,
    u16*      dat_size,
    u8 const* match,
    u16       match_size,
    u8 const* mask,
    u16       mask_size
    );

  void sim_timed_consume (void* h, u8** dat, u16* dat_size, toffset us);
  void sim_timed_consume_match(
    void*     h,
    u8**      dat,
    u16*      dat_size,
    u8 const* match,
    u16       match_size,
    toffset   us
    );
  void sim_timed_consume_match_mask(
    void*     h,
    u8**      dat,
    u16*      dat_size,
    u8 const* match,
    u16       match_size,
    u8 const* mask,
    u16       mask_size,
    toffset   us
    );
  void sim_consume_all (void* h);
  void sim_drop_input_head_private (void* h);
]]
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Don't be tempted to try to optimize these many coroutine.yield() calls
-- unfortunately LuaJIT can't yield within a ffi.C function call
-- http://www.freelists.org/post/luajit/Yielding-Within-FFI-call,3
function sim_yield()
  ffi.C.sim_yield (current_fiber_handle)
  coroutine.yield()
end
--------------------------------------------------------------------------------
function sim_delay (us)
  ffi.C.sim_delay (current_fiber_handle, us)
  coroutine.yield()
end
--------------------------------------------------------------------------------
function sim_timestamp_get()
  local res = ffi.new ("sim_tstamp[1]")
  res[0]    = ffi.C.sim_timestamp_get (current_fiber_handle)
  return res
end
--------------------------------------------------------------------------------
function sim_timestamp_add_usec (tstamp, us)
  local res = ffi.new ("sim_tstamp[1]")
  res[0]    = ffi.C.sim_timestamp_add_usec (tstamp[0], us)
  return res
end
--------------------------------------------------------------------------------
function sim_timestamp_diff_usec (future, past)
  return ffi.C.sim_timestamp_diff_usec (future[0], past[0])
end
--------------------------------------------------------------------------------
function sim_timestamp_is_expired (candidate, deadline)
  return ffi.C.sim_timestamp_is_expired (candidate[0], deadline[0])
end
--------------------------------------------------------------------------------
function sim_wake (wait_id, wake_count)
  ffi.C.sim_wake (current_fiber_handle, wait_id, wake_count)
  coroutine.yield()
end
--------------------------------------------------------------------------------
function sim_wait (wait_id, timeout_us)
  local tout = ffi.new ("bool[1]")
  ffi.C.sim_wait (current_fiber_handle, tout, wait_id, timeout_us)
  coroutine.yield()
  return tout[0]
end
--------------------------------------------------------------------------------
function sim_produce_bytes (dat)
  ffi.C.sim_produce_bytes (current_fiber_handle, dat, dat:len())
  coroutine.yield()
end
--------------------------------------------------------------------------------
function sim_produce_error (err)
  ffi.C.sim_produce_error (current_fiber_handle, err)
  coroutine.yield()
end
--------------------------------------------------------------------------------
function sim_produce_string (str)
  ffi.C.sim_produce_string (current_fiber_handle, str, str:len())
  coroutine.yield()
end
--------------------------------------------------------------------------------
function sim_timed_consume (timeout_us)
  local dat      = ffi.new ("u8*[1]")
  local dat_size = ffi.new ("u16[1]")
  ffi.C.sim_timed_consume (current_fiber_handle, dat, dat_size, timeout_us)
  coroutine.yield()
  local ret
  if dat_size[0] ~= 0 then
    ret = ffi.string (dat[0], dat_size[0])
    ffi.C.sim_drop_input_head_private (current_fiber_handle)
    coroutine.yield()
  end
  return ret
end
--------------------------------------------------------------------------------
function sim_timed_consume_match (match, timeout_us)
  local dat      = ffi.new ("u8*[1]")
  local dat_size = ffi.new ("u16[1]")
  ffi.C.sim_timed_consume_match(
    current_fiber_handle, dat, dat_size, match, match:len(), timeout_us
    )
  coroutine.yield()
  local ret
  if dat_size[0] ~= 0 then
    ret = ffi.string (dat[0], dat_size[0])
    ffi.C.sim_drop_input_head_private (current_fiber_handle)
    coroutine.yield()
  end
  return ret
end
--------------------------------------------------------------------------------
function sim_timed_consume_match_mask (match, mask, timeout_us)
  local dat      = ffi.new ("u8*[1]")
  local dat_size = ffi.new ("u16[1]")
  ffi.C.sim_timed_consume_match_mask(
    current_fiber_handle,
    dat,
    dat_size,
    match,
    match:len(),
    mask,
    mask:len(),
    timeout_us
    )
  coroutine.yield()
  local ret
  if dat_size[0] ~= 0 then
    ret = ffi.string (dat[0], dat_size[0])
    ffi.C.sim_drop_input_head_private (current_fiber_handle)
    coroutine.yield()
  end
  return ret
end
--------------------------------------------------------------------------------
function sim_consume()
  local dat      = ffi.new ("u8*[1]")
  local dat_size = ffi.new ("u16[1]")
  ffi.C.sim_consume (current_fiber_handle, dat, dat_size)
  coroutine.yield()
  ret = ffi.string (dat[0], dat_size[0])
  ffi.C.sim_drop_input_head_private (current_fiber_handle)
  coroutine.yield()
  return ret
end
--------------------------------------------------------------------------------
function sim_consume_match (match)
  local dat      = ffi.new ("u8*[1]")
  local dat_size = ffi.new ("u16[1]")
  ffi.C.sim_consume_match(
    current_fiber_handle, dat, dat_size, match, match:len()
    )
  coroutine.yield()
  ret = ffi.string (dat[0], dat_size[0])
  ffi.C.sim_drop_input_head_private (current_fiber_handle)
  coroutine.yield()
  return ret
end
--------------------------------------------------------------------------------
function sim_consume_match_mask (match, mask)
  local dat      = ffi.new ("u8*[1]")
  local dat_size = ffi.new ("u16[1]")
  ffi.C.sim_consume_match_mask(
    current_fiber_handle,
    dat,
    dat_size,
    match,
    match:len(),
    mask,
    mask:len()
    )
  coroutine.yield()
  ret = ffi.string (dat[0], dat_size[0])
  ffi.C.sim_drop_input_head_private (current_fiber_handle)
  coroutine.yield()
  return ret
end
--------------------------------------------------------------------------------
function sim_consume_all()
  ffi.C.sim_consume_all (current_fiber_handle)
  coroutine.yield()
end
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
sim_sem         = {}
sim_sem.__index = sim_sem
--------------------------------------------------------------------------------
function sim_sem.create (id)
   local newsem = setmetatable ({}, sim_sem)
   newsem.id    = id;
   newsem.sem   = 0;
   return newsem
end
--------------------------------------------------------------------------------
function sim_sem:signal (count)
  assert (count >= 0, "invalid sim_sem signal count: " .. count)
  if self.sem < 0 then
    sim_wake (self.id, math.min (count, -self.sem))
  end
  self.sem = self.sem + count
  return self.sem
end
--------------------------------------------------------------------------------
function sim_sem:timed_wait (timeout_us)
  self.sem  = self.sem - 1
  local unexpired = true
  if self.sem < 0 then
    unexpired = sim_wait (self.id, timeout_us)
    if unexpired == false then
      self.sem = self.sem + 1
    end
  end
  return unexpired
end
--------------------------------------------------------------------------------
function sim_sem:reset()
  self.sem = 0
end
--------------------------------------------------------------------------------
ssc_fibers_private = {}
ssc_fibers_private_last_group = 1
--------------------------------------------------------------------------------
function sim_register_fiber(group, name_str, fiber)
  assert (type (group) == "number", "group must be a number")
  assert(
    ssc_fibers_private_last_group == group or
    ssc_fibers_private_last_group + 1 == group,
    "non consecutive group (groups start with 1)"
    )
  assert (type (name_str) == "string", "name should be a string")
  assert (name_str:len() ~= 0, "name can't be empty")
  assert (type (fiber) == "function", "fiber must be a function")
  if ssc_fibers_private[group] == nil then
    ssc_fibers_private[group] = {}
  end
  table.insert (ssc_fibers_private[group], {name_str, fiber})
  ssc_fibers_private_last_group = group
end
--------------------------------------------------------------------------------

