# encoding: UTF-8
require 'mkmf'
require 'rbconfig'

def define(macro, value = nil)
  $defs.push("-D #{[macro.upcase, value].compact.join('=')}")
end

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

# have_library('yajl', 'yajl_parse', 'yajl/yajl_parse.h')
define("READ_BUFSIZE", "8192")

create_header("yaji_config.h")
create_makefile("parser_ext")
