require 'rake/testtask'
require 'rake/clean'

NAME = 'filesets'

# rule to build the extension: this says
# that the extension should be rebuilt
# after any change to the files in ext
file "ext/filesets/#{NAME}" =>
  Dir.glob("ext/#{NAME}/*{.c,*.rb}") do
    Dir.chdir("ext/#{NAME}") do
    # this does essentially the same thing
    # as what RubyGems does
    sh "make"
  end
end

# make the :test task depend on the shared
# object, so it will be built automatically
# before running the tests
task :filesets => "ext/#{NAME}/#{NAME}"

# use 'rake clean' and 'rake clobber' to
# easily delete generated files
CLEAN.include('ext/**/*{.o,.log,.so}')
CLOBBER.include('ext/**/filesets')
CLOBBER.include('bin/filesets')

# the same as before
# Rake::TestTask.new do |t|
#   t.libs << 'test'
# end
# 

desc "Run tests"
task :default => :filesets
