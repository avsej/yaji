# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)
require "yaji/version"

Gem::Specification.new do |s|
  s.name        = "yaji"
  s.version     = YAJI::VERSION
  s.author      = 'Couchbase'
  s.email       = 'info@couchbase.com'
  s.license     = 'ASL-2'
  s.homepage    = "https://github.com/avsej/yaji"
  s.summary     = %q{Yet another JSON iterator}
  s.description = %q{YAJI is a ruby wrapper to YAJL providing iterator interface to streaming JSON parser}

  s.rubyforge_project = "yaji"

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.executables   = `git ls-files -- bin/*`.split("\n").map{ |f| File.basename(f) }
  s.extensions    = `git ls-files -- ext/**/extconf.rb`.split("\n")
  s.require_paths = ["lib"]

  s.add_development_dependency 'rake', '~> 0.8.7'
  s.add_development_dependency 'rake-compiler'
  s.add_development_dependency 'minitest'
  s.add_development_dependency 'curb'
  s.add_development_dependency RUBY_VERSION =~ /^1\.9/ ? 'ruby-debug19' : 'ruby-debug'
end
