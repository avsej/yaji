desc 'Start an irb session and load the library.'
task :console do
  exec "irb -I lib -rruby-debug -ryaji"
end
