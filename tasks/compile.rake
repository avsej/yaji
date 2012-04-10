gem 'rake-compiler', '>= 0.7.5'
require "rake/extensiontask"

def gemspec
  @clean_gemspec ||= eval(File.read(File.expand_path('../../yaji.gemspec', __FILE__)))
end

Rake::ExtensionTask.new("parser_ext", gemspec) do |ext|
  ext.ext_dir = File.join('ext', 'yaji')

  ext.cross_compile = true
  ext.cross_platform = [ENV['HOST'] || "i386-mingw32"]
  if ENV['RUBY_CC_VERSION']
    ext.lib_dir = "lib/couchbase"
  end
  ext.cross_compiling do |spec|
    spec.files.delete("lib/yaji/parser_ext.so")
    spec.files.push("lib/parser_ext.rb", Dir["lib/yaji/1.{8,9}/parser_ext.so"])
  end

  # clean compiled extension
  CLEAN.include "#{ext.lib_dir}/*.#{RbConfig::CONFIG['DLEXT']}"
end

require 'rubygems/package_task'
Gem::PackageTask.new(gemspec) do |pkg|
  pkg.need_tar = true
end

file "lib/parser_ext.rb" do
  File.open("lib/parser_ext.rb", 'wb') do |f|
    f.write <<-RUBY
      require "yaji/\#{RUBY_VERSION.sub(/\\.\\d+$/, '')}/parser_ext"
    RUBY
  end
end

task :cross => "lib/parser_ext.rb"

desc "Package gem for windows"
task "package:windows" => :package do
  sh("env RUBY_CC_VERSION=1.8.7 rvm 1.8.7 do bundle exec rake cross compile")
  sh("env RUBY_CC_VERSION=1.9.2 rvm 1.9.2 do bundle exec rake cross compile")
  sh("env RUBY_CC_VERSION=1.8.7:1.9.2 rvm 1.9.2 do bundle exec rake cross native gem")
end
