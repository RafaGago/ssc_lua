# See https://github.com/anholt/libepoxy/blob/master/meson.build
# TODO: version, tests, valgrind, visibility, config generation

project(
    'ssc-lua',
    'c',
    version : '0.0.1',
    license : 'BSD-3',
    default_options : [
        'c_std=gnu99',
        'cpp_std=gnu++11', # Just to be able to compile base_library C++ tests
        'default_library=static'
        ]
    )

proj_dir = meson.current_source_dir()

ssc               = subproject ('ssc')
ssc_lib           = ssc.get_variable ('ssc_lib')
base_lib          = ssc.get_variable ('base_lib')
nonblock_lib      = ssc.get_variable ('nonblock_lib')
taskqueue_lib     = ssc.get_variable ('taskqueue_lib')
build_helpers_dir = ssc.get_variable ('build_helpers_dir')

luajit_lib = 'libluajit-5.1.a'
luajit_path = proj_dir + '/dep/install/lib/' + luajit_lib

host_system = host_machine.system()
if host_system != 'windows'
    message ('building LuaJIT')
    res = run_command(
        build_helpers_dir + '/run_in_tree_make_project_target.sh',
        proj_dir,
        'luajit',
        luajit_lib,
        'amalg',
        'PREFIX=/ DESTDIR=' + proj_dir + '/dep/install'
        )
    if res.returncode() != 0
      error (res.stderr().strip())
    endif
    message ('installing LuaJIT locally')
    res = run_command(
        build_helpers_dir + '/run_in_tree_make_project_target.sh',
        proj_dir,
        'luajit',
        luajit_lib,
        'install',
        'PREFIX=/ DESTDIR=' + proj_dir + '/dep/install'
        )
    if res.returncode() != 0
      error (res.stderr().strip())
    endif
endif

message ('generating lua_api.c')
res = run_command(
    proj_dir + '/dep/install/bin/luajit',
    proj_dir + '/script/lua/lua_to_c_string.lua',
    proj_dir + '/src/ssc_lua/lua_api.lua',
    proj_dir + '/src/ssc_lua/lua_api.c',
    'ssc_lua_api'
    )
if res.returncode() != 0
  error (res.stderr().strip())
endif

ssc_include_dirs = ssc.get_variable ('include_dirs')
ssc_test_include_dirs = ssc.get_variable ('test_include_dirs')

libcmocka_path = ssc.get_variable ('libcmocka_path')

include_dirs = [
    include_directories ('include'),
    include_directories ('src'),
    include_directories ('dep/install/include'),
    include_directories ('dep/install/include/luajit-2.0'),
]
include_dirs += ssc_include_dirs

test_include_dirs = include_dirs
test_include_dirs += include_directories ('test/src')
test_include_dirs += ssc_test_include_dirs

pkg_mod = import ('pkgconfig')
install_subdir ('include/ssc_lua', install_dir : 'include')

cflags = ssc.get_variable ('cflags')
lib_cflags  = []
#lib_cflags = ssc.get_variable ('lib_cflags')
test_cflags = []
exec_link_args = [ luajit_path ]
test_link_args = exec_link_args + [ libcmocka_path ]

if get_option ('sharedlib')
    libtype = 'shared_library'
#    lib_cflags += [ '-DSSC_SIM_SHAREDLIB', '-DSSC_SIM_SHAREDLIB_COMPILATION' ]
else
    libtype = 'static_library'
    lib_cflags += [ '-DSSC_PRIVATE_SYMS', '-DSSC_SIM_PRIVATE_SYMS' ]
endif

cc = meson.get_compiler ('c')
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
    if get_option ('pic_statlibs') and libtype == 'static_library'
        lib_cflags += ['-fPIC']
    endif
    # As per LuaJIT docs
    exec_link_args = ['-Wl,-E'] + exec_link_args
    test_link_args = ['-Wl,-E'] + test_link_args
endif

libmath = cc.find_library ('m', required : false)
libdl   = cc.find_library ('dl', required : false)

host_system = host_machine.system()
if host_system == 'windows'
    test_link_args += ['-lwinmm.lib'] # Untested
endif

threads = dependency ('threads')

#- ssc lua ---------------------------------------------------------------------

ssc_lua_srcs = [
    'src/ssc_lua/lua_api.c',
    'src/ssc_lua/lua_fibers.c',
    'src/ssc_lua/ssc_lua.c',
    'src/ssc_lua/lpack.c',
]
ssc_lua_test_srcs = [
    'test/src/ssc_lua/tests_main.c',
    'test/src/ssc_lua/test_params.c',
    'test/src/ssc_lua/basic_test.c',
]
ssc_lua_example_srcs = [
    'example/src/ssc_lua/ssc_lua_example.c'
]
ssc_lua_lib = build_target(
    'ssc-lua',
    ssc_lua_srcs,
    target_type         : libtype,
    include_directories : include_dirs,
    c_args              : cflags + lib_cflags,
    install             : true
    )
pkg_mod.generate(
    libraries   : ssc_lua_lib,
    version     : '0.0.1',
    name        : 'libssc-lua',
    filebase    : 'ssc-lua',
    description : 'simulator task scheduler. LUA binding.'
    )
test(
    'ssc-lua-test',
    executable(
        'ssc-lua-test',
        ssc_lua_test_srcs,
        include_directories : test_include_dirs,
        link_with           : [ ssc_lib, ssc_lua_lib ],
        link_args           : test_link_args,
        c_args              : cflags,
        dependencies        : [ threads, libmath, libdl ]
    ))
executable(
    'ssc-lua.example',
    ssc_lua_example_srcs,
    include_directories : include_dirs,
    link_with           : [ ssc_lib, ssc_lua_lib ],
    link_args           : exec_link_args,
    c_args              : cflags,
    dependencies        : [ threads, libmath, libdl ]
)
