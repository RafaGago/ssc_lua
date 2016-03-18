--------------------------------------------------------------------------------
sem = sim_sem.create (1)
--------------------------------------------------------------------------------
-- if the queue content matches it does signal "sem"
--------------------------------------------------------------------------------
sim_register_fiber (1, "signal_sem", function()
  local match = string.char (0x03, 0x01, 0x01, 0x00)
  local mask  = string.char (0xff, 0x01)
  while true do
    local dat = sim_consume_match_mask (match, mask)
    sem:signal (1)
  end
end)
--------------------------------------------------------------------------------
-- waits on id 1 and sends response 0x00, 0x01, 0x02, 0x03
--------------------------------------------------------------------------------
sim_register_fiber (1, "produce_out", function()
  local resp = string.char (0x00, 0x01, 0x02, 0x03)
  while true do
    sem:timed_wait (0)
    sim_produce_bytes (resp)
  end
end)
--------------------------------------------------------------------------------

