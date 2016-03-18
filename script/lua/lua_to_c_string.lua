function readfile(name)
  local f       = io.open (name, "rb")
  if f == nil then
    error ("error opening file: "..name)
  end
  local content = f:read("*all")
  f:close()
  return content
end

function writefile(name, str)
  local f = io.open (name, "wb")
  if f == nil then
    error ("error opening file to write: "..name)
  end
  f:write (str)
  f:close()
end

src_file   = arg[1]
dst_file   = arg[2]
c_str_name = arg[3]

if src_file == nil or dst_file == nil or c_str_name == nil then
  error ("usage: "..arg[0].." [source file] [dest file] [C string name]")
end

--Convert script to C string
str = readfile (src_file)
str = str:gsub ("\\\"", "\\\\\\\"")
str = str:gsub ("([^\\])\"", "%1\\\"")
str = str:gsub ("([^\n]*)\n", "\"%1\\n\"\n")
str = str:gsub (string.rep ("%-", 80), "")

newln = "\\n"
quote = "\""

str = str:gsub(
  newln..quote.."\n"..quote..newln..quote.."\n",
  newln..quote.."\n"                     .."\n"
  )

sep = "\n/*" .. string.rep ("%-", 76) .. "*/\n"

str = str:gsub ("\n\n", sep)
str = str:gsub ("\n\n", sep)
str = 
  "/*This file is auto-generated; don't edit*/\n\n"..
  "char const* "..c_str_name.." = \n" .. 
  str.. 
  ";\n"
writefile (dst_file, str)

