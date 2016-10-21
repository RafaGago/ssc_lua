local projname             = "ssc_lua"
local repo                 = path.getabsolute ("../..")

local ssc                = repo.."/dependencies/ssc"
local ssc_include        = ssc.."/include"
local ssc_src            = ssc.."/src"

local base_library         = ssc.."/dependencies/base_library"
local base_library_include = base_library.."/include"
local base_library_src     = base_library.."/src"

local cmocka               = base_library.."/dependencies/cmocka"
local cmocka_lib           = cmocka.."/lib"
local cmocka_include       = cmocka.."/include"

local repo_include         = repo.."/include"
local repo_src             = repo.."/src"
local repo_test_src        = repo.."/test/src"
local repo_example_src     = repo.."/example/src"
local repo_build           = repo.."/build"
local repo_script          = repo.."/script"
local repo_lua_script      = repo_script.."/lua"

local lua                  = repo.."/dependencies/code/LuaJIT-2.0.4/stage"
local lua_include          = lua.."/include/luajit-2.0"
local lua_lib              = lua.."/lib"
local lua_interpreter      = lua.."/bin/luajit"

local stage                = repo_build.."/stage/"..os.get()
local build                = repo_build.."/"..os.get()
local version              = ".0.0.0"

local function get_bl_stage(cfg)
  return base_library.."/build/stage/"..os.get().."/"..cfg
end

local function get_ssc_stage(cfg)
  return ssc.."/build/stage/"..os.get().."/"..cfg
end

local function get_repo_stage(cfg)
  return stage.."/"..cfg
end

local autogen_lua_api_cmd =
  lua_interpreter.." "..
    repo_lua_script.."/lua_to_c_string.lua "..
    repo_src.."/ssc_lua/lua_api.lua "..
    repo_src.."/ssc_lua/lua_api.c "..
    "ssc_lua_api"

if os.get() == "windows" then
  autogen_lua_api_cmd:gsub ("/", "\\")
end
os.execute (autogen_lua_api_cmd)

solution "build"
  configurations { "release", "debug" }
  location (build)

filter {"system:linux"}
  defines {"BL_USE_CLOCK_MONOTONIC_RAW"}

filter {"system:not windows"}
  postbuildcommands {
    "cd %{cfg.buildtarget.directory} && "..
    "ln -sf %{cfg.buildtarget.name} "..
    "%{cfg.buildtarget.name:gsub ('.%d+.%d+.%d+', '')}"
    }

filter {"configurations:*"}
  flags {"MultiProcessorCompile"}
  includedirs {
    repo_include,
    repo_src,
    base_library_include,
    ssc_include,
    lua_include,
    }

filter {"configurations:release"}
  defines {"NDEBUG"}
  optimize "On"

filter {"configurations:debug"}
  flags {"Symbols"}
  defines {"DEBUG"}
  optimize "Off"

--[[
filter {"configurations:debug", "action:not gmake"}
  links {
    get_bl_stage ("debug").."/lib/base_library_task_queue",
    get_bl_stage ("debug").."/lib/base_library_nonblock",
    get_bl_stage ("debug").."/lib/base_library",
    get_ssc_stage ("debug").."/lib/ssc",
    }
]]--

filter {"configurations:debug", "action:gmake", "kind:ConsoleApp"}
  linkoptions {
    "-Wl,-E",
    "-L"..get_bl_stage ("debug").."/lib",
    "-L"..get_ssc_stage ("debug").."/lib",
    "-L"..get_repo_stage ("debug").."/lib",
    "-L"..lua_lib,
    "-L"..cmocka_lib,
    "-l:libssc_static.a.d",
    "-l:lib"..projname.."_static.a.d",
    "-l:libluajit-5.1.a",
    "-l:libbase_library_task_queue.a.d",
    "-l:libbase_library_nonblock.a.d",
    "-l:libbase_library.a.d",
    }

filter {"configurations:release", "action:gmake", "kind:ConsoleApp"}
  linkoptions {
    "-Wl,-E",
    "-L"..get_bl_stage ("release").."/lib",
    "-L"..get_ssc_stage ("release").."/lib",
    "-L"..get_repo_stage ("release").."/lib",
    "-L"..lua_lib,
    "-L"..cmocka_lib,
    "-lssc_static",
    "-l"..projname.."_static",
    "-l:libluajit-5.1.a",
    "-lbase_library_task_queue",
    "-lbase_library_nonblock",
    "-lbase_library",
    }

filter {"action:gmake", "kind:ConsoleApp"}
  links {"pthread", "rt", "m", "dl"}

filter {"kind:SharedLib", "configurations:debug", "system:not windows"}
  targetextension (".so"..".d"..version)

filter {"kind:SharedLib", "configurations:release", "system:not windows"}
    targetextension (".so"..version)

filter {"kind:StaticLib", "configurations:debug", "system:not windows"}
  targetextension (".a"..".d"..version )

filter {"kind:StaticLib", "configurations:debug"}
  targetdir (stage.."/debug/lib")

filter {"kind:StaticLib", "configurations:release", "system:not windows"}
  targetextension (".a"..version )

filter {"kind:StaticLib", "configurations:release"}
  targetdir (stage.."/release/lib")

filter {"kind:ConsoleApp", "configurations:debug", "system:not windows"}
  targetextension ("_d"..version)

filter {"kind:ConsoleApp", "configurations:debug"}
  targetdir (stage.."/debug/bin")

filter {"kind:ConsoleApp", "configurations:release", "system:not windows"}
  targetextension (version )

filter {"kind:ConsoleApp", "configurations:release"}
  targetdir (stage.."/release/bin")

filter {"action:gmake"}
  buildoptions {"-std=gnu11"}
  buildoptions {"-Wfatal-errors"}

filter {"action:gmake", "kind:*Lib"}
  buildoptions {"-fvisibility=hidden"}

filter {"action:vs*"}
  buildoptions {"/TP"}

project (projname.."_static")
  kind "StaticLib"
  defines {"SSC_PRIVATE_SYMS", "SSC_SIM_PRIVATE_SYMS"}
  language "C"
  files {
    repo_src.."/**.c",
    }
  prebuildcommands { autogen_lua_api_cmd }
--[[
project (projname)
  kind "SharedLib"
  defines { "SSC_SHAREDLIB", "SSC_SHAREDLIB_COMPILATION"}
  language "C"
  files {
    repo_src.."/**.c",
    }
  prebuildcommands {
    "cd "..repo_src.."/ssc_lua && "..lua_interpreter..
    " generate_lua_api.lua"
    }
]]--
project (projname.."_test")
  kind "ConsoleApp"
  language "C"
  files {
    repo_test_src.."/**.c",
    }
  includedirs {
    repo_test_src,
    cmocka_include
    }
  links {
    projname.."_static",
    cmocka_lib.."/cmocka",
    }
  postbuildcommands {
    "{COPY} "..repo_test_src.."/"..projname.."/*.lua "..
      "%{cfg.buildtarget.directory}"
    }

project (projname.."_example")
  kind "ConsoleApp"
  language "C"
  files {
    repo_example_src.."/**.c",
    }
  links {
    projname.."_static",
    }
  postbuildcommands {
    "{COPY} "..repo_example_src.."/"..projname.."/*.lua "..
      "%{cfg.buildtarget.directory}"
    }
