#include <string.h>
#include <stdlib.h>

#include <bl/base/utility.h>
#include <bl/base/time.h>

#include <ssc_lua/cmocka_pre.h>

#include <ssc/simulator/simulator.h>
#include <ssc_lua/ssc_lua.h>
#include <ssc_lua/test_params.h>

typedef struct context {
  ssc* sim;
}
context;
/*---------------------------------------------------------------------------*/
/* Basic test */
/*---------------------------------------------------------------------------*/
char* str_alloc_append_to_script_path (char const* name)
{
  bl_uword pathlen = strlen (script_path);
  bl_uword namelen = strlen (name);
  char* str     = (char*) malloc (pathlen + namelen + 1);
  assert_non_null (str);
  memcpy (str, script_path, pathlen);
  memcpy (str + pathlen, name, namelen);
  str[pathlen + namelen] = 0;
  return str;
}
/*---------------------------------------------------------------------------*/
static int test_setup (void **state, const char* script)
{
  static context      c;
  ssc_lua_passed_data pd;

  *state                = nullptr;
  pd.main_script_path   = str_alloc_append_to_script_path (script);
  pd.before_main_script = nullptr;
  pd.after_main_script  = nullptr;
  pd.on_init_context    = nullptr;
  pd.min_stack_bytes    = 512 * 1024 * 1024;
  bl_err err            = ssc_create (&c.sim, "", &pd);
  free ((char*) pd.main_script_path);
  assert_true (!err.bl);
  err = ssc_run_setup (c.sim);
  assert_true (!err.bl);
  *state = &c;
  return (int) err.bl;
}
/*---------------------------------------------------------------------------*/
static int queue_match_test_setup (void **state)
{
  return test_setup (state, "queue_match_test.lua");
}
/*---------------------------------------------------------------------------*/
static int test_teardown (void **state)
{
  context* ctx = (context*) *state;
  if (!ctx) {
    return 1;
  }
  assert_true (ssc_run_teardown (ctx->sim).bl == bl_ok);
  assert_true (ssc_destroy (ctx->sim).bl == bl_ok);
  return 0;
}
/*---------------------------------------------------------------------------*/
static void queue_match_test (void **state)
{
  /*the simulation code for this test is on "queue_match_test.lua"*/
  const bl_u8 expected_req[]   = {0x03, 0x01, 0x01, 0x00};
  const bl_u8 filter_out_req[] = {0x03, 0x00, 0x01, 0x00};
  const bl_u8 expected_resp[]  = {0x00, 0x01, 0x02, 0x03};
  context* ctx = (context*) *state;
  ssc_output_data read_data;
  bl_uword           count;

  /*send wrong message*/
  bl_u8* bs = ssc_alloc_write_bytestream (ctx->sim, sizeof filter_out_req);
  assert_non_null (bs);
  memcpy (bs, filter_out_req, sizeof filter_out_req);
  bl_err err = ssc_write (ctx->sim, 0, bs, sizeof filter_out_req);
  assert_true (!err.bl);

  /*run the simulator*/
  do {
    err = ssc_try_run_some (ctx->sim);
  }
  while (err.bl != bl_nothing_to_do);
  assert_true (err.bl == bl_nothing_to_do);

  /*check that the message was filtered out*/
  err = ssc_read (ctx->sim, &count, &read_data, 1, 10000);
  assert_true (err.bl == bl_timeout);

  /*send the message that will match*/
  bs = ssc_alloc_write_bytestream (ctx->sim, sizeof expected_req);
  assert_non_null (bs);
  memcpy (bs, expected_req, sizeof expected_req);
  err = ssc_write (ctx->sim, 0, bs, sizeof expected_req);
  assert_true (!err.bl);

  /*run the simulator*/
  do {
    err = ssc_try_run_some (ctx->sim);
  }
  while (err.bl != bl_nothing_to_do);
  assert_true (err.bl == bl_nothing_to_do);

  /*check that the right message triggered the sim semaphore signalling*/
  err = ssc_read (ctx->sim, &count, &read_data, 1, 10000);
  assert_true (!err.bl);
  assert_true (ssc_output_is_bytes (&read_data));
  assert_true (ssc_output_is_dynamic (&read_data));
  bl_memr16 rd = ssc_output_read_as_bytes (&read_data);
  assert_true (bl_memr16_size (rd) == sizeof expected_resp);
  assert_memory_equal (bl_memr16_beg (rd), expected_resp, sizeof expected_resp);
  err = ssc_dealloc_read_data (ctx->sim, &read_data);
  assert (!err.bl);
}
/*---------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown(
    queue_match_test, queue_match_test_setup, test_teardown
    ),
};
/*---------------------------------------------------------------------------*/
int basic_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*---------------------------------------------------------------------------*/

