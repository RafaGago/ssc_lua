--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Produce
--------------------------------------------------------------------------------
sim_register_fiber (1, "produce error", function()
    while true do
    sim_produce_error (0)
    sim_delay (10000000)
    -- fibers not reading the queue must periodically call "sim_consume_all" to
    -- decrease the input message refcount
    sim_consume_all()
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "produce bytes", function()
  local bytes = string.char (0x00, 0x01, 0x02, 0x03)
  while true do
    sim_produce_bytes (bytes)
    sim_delay (10000000)
    sim_consume_all()
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "produce string", function()
  while true do
    sim_produce_string ("hello world!")
    sim_delay (10000000)
    sim_consume_all()
  end
end)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Consume
--------------------------------------------------------------------------------
sim_register_fiber (1, "get everything", function()
  while true do
    local dat = sim_consume()
    sim_produce_string ("I answer to everything")
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "masked", function()
  local match = string.char (0x01, 0x01)
  while true do
    local dat = sim_consume_match (match)
    sim_produce_string ("I answer to 0101 prefix")
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "masked + matched", function()
  local match = string.char (0x01, 0x01)
  local mask = string.char (0xff, 0x01)
  while true do
    local dat = sim_consume_match_mask (match, mask)
    sim_produce_string ("I answer to 01 + odd number prefixes")
  end
end)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Timed Consume
--------------------------------------------------------------------------------
sim_register_fiber (1, "get everything timeout", function()
  while true do
    local dat = sim_timed_consume (10000000)
    if dat ~= nil then
      sim_produce_string ("I answer to everything or I time out")
    else
      sim_produce_string ("I answer to everything or I time out: timed out")
    end
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "masked timeout", function()
  local match = string.char (0x02, 0x02)
  while true do
    local dat = sim_timed_consume_match (match, 10000000)
    if dat ~= nil then
      sim_produce_string ("I answer to the 0202 prefix or I time out")
    else
      sim_produce_string ("I answer to the 0202 prefix or I time out: timed out")
    end
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "masked + matched timeout", function()
  local match = string.char (0x02, 0x00)
  local mask = string.char (0xff, 0x01)
  while true do
    local dat = sim_timed_consume_match_mask (match, mask, 10000000)
    if dat ~= nil then
      sim_produce_string ("I answer to 02 + even number prefixes or I time out")
    else
      sim_produce_string(
        "I answer 02 + even number prefixes or I time out: timed out"
        )
    end
  end
end)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Timestamps
--------------------------------------------------------------------------------
sim_register_fiber (1, "timestamps", function()
  local expired = sim_timestamp_get()
  while true do
    if sim_timestamp_is_expired (sim_timestamp_get(), expired) then
      sim_produce_string ("timestamp expired")
      expired = sim_timestamp_add_usec (sim_timestamp_get(), 10000000)
      sim_consume_all()
    end
    sim_delay (100000)
  end
end)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Semaphore
--------------------------------------------------------------------------------
sem = sim_sem.create (1)
--------------------------------------------------------------------------------
sim_register_fiber (1, "signal sem 1 on ffff match", function()
  local match = string.char (0xff, 0xff)
  local mask  = string.char (0xff, 0xff)
  while true do
    local dat = sim_consume_match_mask (match, mask)
    sim_produce_string ("ffff received: signaling semaphore")
    sem:signal (1)
  end
end)
--------------------------------------------------------------------------------
sim_register_fiber (1, "wait on sem 1", function()
  while true do
    local unexpired = sem:timed_wait (200000)
    if unexpired then
      sim_produce_string ("semaphore signal received")
    end
    -- fibers not reading the queue must periodically call "sim_consume_all" to
    -- decrease the input message refcount
    sim_consume_all()
  end
end)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
