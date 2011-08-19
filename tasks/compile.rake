gem 'rake-compiler', '>= 0.7.5'
require "rake/extensiontask"

def gemspec
  @clean_gemspec ||= eval(File.read(File.expand_path('../../yaji.gemspec', __FILE__)))
end

Rake::ExtensionTask.new("parser_ext", gemspec) do |ext|
  ext.ext_dir = File.join('ext', 'yaji')

  # clean compiled extension
  CLEAN.include "#{ext.lib_dir}/*.#{RbConfig::CONFIG['DLEXT']}"
end
