-- Generate compile_commands.json for clangd every time the project is built
add_rules("plugin.compile_commands.autoupdate", {lsp = "clangd"})
-- Set c++ code standard: c++17
set_languages("c++17")
add_requires("sockpp")
add_requires("argparse")

target("simple-ftp-server")
  set_kind("binary")
  add_includedirs("include")
  add_files("lib/*.cc")
  add_files("src/server.cc")
  add_packages("sockpp")
  add_packages("argparse")
  

target("simple-ftp-client")
  set_kind("binary")
  add_includedirs("include")
  -- add_files("lib/*/*.cc")
  add_files("src/client.cc")
  add_packages("sockpp")
  add_packages("argparse")