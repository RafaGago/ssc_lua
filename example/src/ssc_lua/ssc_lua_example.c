#include <stdio.h>
#include <string.h>
#include <bl/base/thread.h>
#include <bl/base/time.h>
#include <bl/base/hex_string.h>
#include <ssc/simulator/simulator.h>
#include <ssc_lua/ssc_lua.h>

/*---------------------------------------------------------------------------*/
typedef struct program {
  ssc*        sim;
  int         running;
  bl_timept32 startup;
  char        rcv[512];
  char        send[512];
}
program;
/*---------------------------------------------------------------------------*/
void print_usage()
{
  printf ("usage: ssc_lua_example [simulation script path]\n");
}
/*---------------------------------------------------------------------------*/
void print_time (program* p, bl_timept32 t)
{
  bl_timeoft32 toff = bl_timept32_to_usec (t - p->startup);
  bl_timeoft32 sec  = toff / bl_usec_in_sec;
  bl_timeoft32 usec = toff % bl_usec_in_sec;
  printf ("[%05u.%06u]", sec, usec);
}
/*---------------------------------------------------------------------------*/
void process_read_message (program* p, ssc_output_data* od)
{
  print_time (p, od->time);
  switch ((od->type & ~ssc_type_is_dynamic_mask)) {
  case ssc_type_bytes: {
    bl_memr16 data = ssc_output_read_as_bytes (od);
    int ret = bl_bytes_to_hex_string(
      p->rcv, bl_arr_elems (p->rcv), bl_memr16_beg (data), bl_memr16_size (data)
      );
    if (-1 == ret) {
      fprintf (stderr, "<- message too big\n");
    }
    printf ("<- %s\n", p->rcv);
    break;
  }
  case ssc_type_string: {
    bl_u16 strlength;
    char const* str = ssc_output_read_as_string (od, &strlength);
    printf ("<- %s\n", str);
    break;
  }
  case ssc_type_error: {
    bl_err      err;
    char const* errstr;
    ssc_output_read_as_error (od, &err, &errstr);
    printf ("<- err: %s, errstr: %s\n", bl_strerror (err), errstr);
    break;
  }
  default: break;
  }
}
/*---------------------------------------------------------------------------*/
int write_console (void* context)
{
  program* p = (program*) context;
  while (p->running) {
    char* line = fgets (p->send, bl_arr_elems (p->send), stdin);
    if (!line) {
      fprintf (stderr, "fgets failed\n");
      continue;
    }
    char *lf_ptr = strchr (line, '\n');
    if (lf_ptr) {
      *lf_ptr = 0;
    }
    bl_uword size = strlen (line);
    if (size == 0) {
      continue;
    }
    if (strcmp (line, "exit") == 0) {
      p->running = 0;
      return 0;
    }
    bl_uword bytes = (size / 2) + (size & 1);
    bl_u8* mem = ssc_alloc_write_bytestream (p->sim, bytes);
    if (!mem) {
      fprintf (stderr, "ssc_alloc_write_bytestream failed!\n");
      continue;
    }
    /*printf ("strlen: %u, bytes: %u, line: %s\n", size, bytes, line);*/
    bl_word ret = bl_hex_string_to_bytes (mem, bytes, line);
    if (ret < 0) {
      fprintf (stderr, "invalid hex string\n");
      continue;
    }
    print_time (p, bl_timept32_get());
    bl_err err = ssc_write (p->sim, 0, mem, (bl_u16) ret);
    if (!err.own) {
      printf ("-> %s\n", line);
    }
    else {
      fprintf (stderr, "-> ssc_write failed: %s\n", bl_strerror (err));
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  struct program p;
  p.running = 1;
  if (argc != 2) {
    print_usage();
    return -1;
  }
  if (strcmp (argv[1], "help") == 0 || strcmp (argv[1], "--help") == 0) {
    print_usage();
    return 0;
  }
  ssc_lua_passed_data pd;
  pd.main_script_path   = argv[1];
  pd.before_main_script = nullptr;
  pd.after_main_script  = nullptr;
  pd.on_init_context    = nullptr;
  pd.min_stack_bytes    = 512 * 1024 * 1024;
  bl_err err            = ssc_create (&p.sim, "", &pd);
  if (err.own) {
    fprintf (stderr, "unable to create ssc_lua: %s\n", bl_strerror (err));
    return (int) err.own;
  }
  p.startup = bl_timept32_get();
  err       = ssc_run_setup (p.sim);
  if (err.own) {
    fprintf (stderr, "unable to run ssc_lua setup: %s\n", bl_strerror (err));
    goto destroy;
  }
  printf(
    "-------------------------------------------------------------\n"
    " Type hex strings followed by \"Enter\" to send them to the\n"
    " simulator. Type \"exit\" to quit the program\n"
    "-------------------------------------------------------------\n"
    );
  bl_thread thr;
  int thr_err = bl_thread_init (&thr, write_console, &p).own;
  if (thr_err != 0) {
    fprintf (stderr, "unable to start console thread\n");
    goto teardown;
  }
  ssc_output_data od[16];
  while (p.running) {
    err = ssc_run_some (p.sim, 100);
    if (err.own && err.own != bl_timeout) {
      /*TODO*/
    }
    bl_uword read_msgs;
    (void) ssc_read (p.sim, &read_msgs, od, bl_arr_elems (od), 0);
    for (bl_uword i = 0; i < read_msgs; ++i) {
      process_read_message (&p, od + i);
      ssc_dealloc_read_data (p.sim, od + i);
    }
  }
teardown:
  ssc_run_teardown (p.sim);
destroy:
  ssc_destroy (p.sim);
  return (int) err.own;
}
/*---------------------------------------------------------------------------*/
