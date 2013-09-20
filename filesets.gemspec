Gem::Specification.new do |s|
  s.name        = 'filesets'
  s.version     = '0.0.4'
  s.date        = '2013-09-20'
  s.summary     = "filesets: performs set operations on integers. Each file contains on set."
  s.description = "Blah..."
  s.homepage    = "http://change.org"
  s.authors     = ["Mark Steckel", "Vijay Ramesh"]
  s.email       = ['mjs@change.org', 'vijay@change.org']
  s.files       = ["ext/filesets/filesets.c", "ext/filesets/fs-test.rb", "ext/filesets/Makefile"]
  s.extensions  = ['ext/filesets/extconf.rb']
  s.executables << 'filesets'
end
