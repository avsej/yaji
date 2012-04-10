# encoding: UTF-8
ENV['RC_ARCHS'] = '' if RUBY_PLATFORM =~ /darwin/

require 'mkmf'
require 'rbconfig'

def define(macro, value = nil)
  $defs.push("-D #{[macro.upcase, value].compact.join('=')}")
end

$CFLAGS  << " #{ENV["CFLAGS"]}"
$LDFLAGS << " #{ENV["LDFLAGS"]}"
$LIBS    << " #{ENV["LIBS"]}"

$CFLAGS << ' -std=c99 -Wall -funroll-loops -Wextra '
$CFLAGS << ' -O0 -ggdb3 -pedantic ' if ENV['DEBUG']

# have_library('yajl', 'yajl_parse', 'yajl/yajl_parse.h')

define("READ_BUFSIZE", "8192")

create_header("yaji_config.h")
create_makefile("parser_ext")
