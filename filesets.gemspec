Gem::Specification.new do |s|
  s.name        = 'filesets'
  s.version     = '0.0.2'
  s.date        = '2012-05-17'
  s.summary     = "filesets: performs set operations on integers. Each file contains on set."
  s.description = "Blah..."
  s.homepage    = "http://change.org"
  s.authors     = ["Mark Steckel"]
  s.email       = 'mjs@change.org'
  s.files       = ["ext/filesets/filesets.c", "ext/filesets/fs-test.rb", "ext/filesets/Makefile"]
  s.extensions  = ['ext/filesets/extconf.rb']
  s.executables << 'filesets'
end
