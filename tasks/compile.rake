gem 'rake-compiler', '>= 0.7.5'
require 'rake/extensiontask'

def gemspec
  @clean_gemspec ||= eval(File.read(File.expand_path('../../yaji.gemspec', __FILE__)))
end

require 'rubygems/package_task'
Gem::PackageTask.new(gemspec) { |_pkg| }

version_router = proc do |t|
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
    each_version do |_long, short|
      res << short
    end
    res
  end
end

CROSS_PLATFORMS = [
  Platform.new(name: 'x64-mingw32', host: 'x86_64-w64-mingw32', versions: %w(2.0.0-p645 2.1.6 2.2.2 2.3.0 2.4.0)),
  Platform.new(name: 'x86-mingw32', host: 'i686-w64-mingw32', versions: %w(2.0.0-p645 2.1.6 2.2.2 2.3.0 2.4.0))
].freeze

Rake::ExtensionTask.new('parser_ext', gemspec) do |ext|
  ext.ext_dir = File.join('ext', 'yaji')

  ext.cross_compile = true
  ext.cross_platform = ENV['TARGET']
  ext.lib_dir = 'lib/yaji' if ENV['RUBY_CC_VERSION']
  ext.cross_compiling do |spec|
    spec.files.delete('lib/yaji/parser_ext.so')
    spec.files.push('lib/parser_ext.rb', Dir['lib/yaji/*/parser_ext.so'])
    file "#{ext.tmp_dir}/#{ext.cross_platform}/stage/lib/parser_ext.rb", &version_router
  end

  # clean compiled extension
  CLEAN.include "#{ext.lib_dir}/*.#{RbConfig::CONFIG['DLEXT']}"
end
CLEAN.include Dir['lib/yaji/*/parser_ext.so']

file 'lib/parser_ext.rb', &version_router
task cross: 'lib/parser_ext.rb'

desc 'Package gem for windows'
task 'package:windows' => :package do
  CROSS_PLATFORMS.each do |platform|
    platform.each_version do |_long, short|
      sh("TARGET=#{platform.name} RUBY_CC_VERSION=#{short} rake cross compile")
    end
    sh("TARGET=#{platform.name} RUBY_CC_VERSION=#{platform.short_versions.join(':')} rake cross native gem")
    sh('rake clean')
  end
end
