require 'rake/testtask'
Rake::TestTask.new do |test|
  test.libs << "test" << "."
  test.ruby_opts << "-rruby-debug" if ENV['DEBUG']
  test.pattern = 'test/test_*.rb'
  test.verbose = true
end
