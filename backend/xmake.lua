-- Generate compile_commands.json for clangd every time the project is built
add_rules("plugin.compile_commands.autoupdate", {lsp = "clangd"})
-- Set c++ code standard: c++17
set_languages("c++17")
-- add_requires("toml++")
-- add_requires("argparse")

target("simple-ftp-server")
  set_kind("binary")
  add_includedirs("server/include")
  -- add_files("server/src/lib/*/*.cc")
  add_files("server/src/main.cc")
  -- add_packages("toml++")
  -- add_packages("argparse")

target("simple-ftp-client")
  set_kind("binary")
  add_includedirs("client/include")
  -- add_files("client/src/lib/*/*.cc")
  add_files("client/src/main.cc")