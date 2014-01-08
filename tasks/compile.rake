gem 'rake-compiler', '>= 0.7.5'
require "rake/extensiontask"

def gemspec
  @clean_gemspec ||= eval(File.read(File.expand_path('../../yaji.gemspec', __FILE__)))
end

require 'rubygems/package_task'
Gem::PackageTask.new(gemspec) do |pkg|
  pkg.need_tar = true
end

version_router = lambda do |t|
  File.open(t.name, 'wb') do |f|
    f.write <<-RUBY
      require "yaji/\#{RUBY_VERSION.sub(/\\.\\d+$/, '')}/parser_ext"
    RUBY
  end
end

class Platform
  attr_reader :name, :host, :versions

  def initialize(params)
    @name = params[:name]
    @host = params[:host]
    @versions = params[:versions]
  end

  def each_version
    @versions.each do |v|
      yield(v, v[/\d\.\d\.\d/])
    end
  end

  def short_versions
    res = []
    each_version do |long, short|
      res << short
    end
    res
  end
end

recent = "2.0.0-p353"
CROSS_PLATFORMS = [
  Platform.new(:name => 'x64-mingw32', :host => 'x86_64-w64-mingw32', :versions => %w(1.9.3-p484 2.0.0-p353 2.1.0)),
  Platform.new(:name => 'x86-mingw32', :host => 'i686-w64-mingw32', :versions => %w(1.8.7-p374 1.9.3-p484 2.0.0-p353 2.1.0)),
]
Rake::ExtensionTask.new("parser_ext", gemspec) do |ext|
  ext.ext_dir = File.join('ext', 'yaji')

  ext.cross_compile = true
  ext.cross_platform = ENV['TARGET']
  if ENV['RUBY_CC_VERSION']
    ext.lib_dir = "lib/yaji"
  end
  ext.cross_compiling do |spec|
    spec.files.delete("lib/yaji/parser_ext.so")
    spec.files.push("lib/parser_ext.rb", Dir["lib/yaji/*/parser_ext.so"])
    file "#{ext.tmp_dir}/#{ext.cross_platform}/stage/lib/parser_ext.rb", &version_router
  end

  # clean compiled extension
  CLEAN.include "#{ext.lib_dir}/*.#{RbConfig::CONFIG['DLEXT']}"
end

file "lib/parser_ext.rb", &version_router
task :cross => "lib/parser_ext.rb"

desc "Package gem for windows"
task "package:windows" => :package do
  CROSS_PLATFORMS.each do |platform|
    ENV['TARGET'] = platform.name
    platform.each_version do |long, short|
      sh("env RUBY_CC_VERSION=#{short} RBENV_VERSION=#{long} rbenv exec rake cross compile")
    end
    sh("env RUBY_CC_VERSION=#{platform.short_versions.join(":")} RBENV_VERSION=#{recent} rbenv exec rake cross native gem")
  end
end
