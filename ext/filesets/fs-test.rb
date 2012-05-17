#!/usr/bin/env ruby

puts `pwd`

MAX_ID_VAL = 20
OUTFILE    = "/tmp/result.txt"

def run_one_test(line)
  result_file, *expression = line.strip.split(" ")
  expression = expression.join(" ")

  # puts "#{FileSet} -max #{MAX_ID_VAL} -o #{OUTFILE} #{expression}"

  # file-sets -max #{MAX_ID_VAL} -o result.txt expresion
  r = Kernel.system(FileSet, "-max", "#{MAX_ID_VAL}", "-o", OUTFILE, "#{expression}")

  # err exit(1) if non-zero exit val
  if r == false
    $stderr.puts "\nTEST FAILED: could not execute:\n"
    $stderr.puts "    #{result_file} != #{expression}\n\n"
    exit(1) 
  end

  # cmp result_file result.txt
  r = Kernel.system("cmp", "--quiet", result_file, "/tmp/result.txt")
  
  # err exit(1) if non-zero exit val
  if r == false
    $stderr.puts "\nTEST FAILED: did not match expected results:\n"
    $stderr.puts "    #{result_file} != #{expression}\n\n"
    exit(1)
  end

end


def process_test_file(file)
  flag = nil
  File.open(file) do |f|
    f.each do |line|
      next if line[0..0] == '#' || line.strip.size == 0
      run_one_test(line)
      flag = true
    end
  end
  if flag.nil?
    $stderr.puts "ERROR: No tests found in: #{file}"
    exit(1)  
  end
end

def find_and_process_test_files(path)
  flag = nil
  Dir.glob("*.t") do |file|
    process_test_file(file)
    flag = true
  end
  if flag.nil?
    $stderr.puts "ERROR: No test files found in: #{path}"
    exit(1)  
  end
end

if ARGV.size != 2
  $stderr.puts "ERROR: Test requires 2 args: 1) path to executable and 2) path to test dir."
  exit(1)
end

if ARGV[0][0..0] == "/"
  FileSet = ARGV[0]
else
  FileSet = Dir.pwd + "/" + ARGV[0]
end

TestDir = ARGV[1]

Dir.chdir(TestDir)

find_and_process_test_files(TestDir)

exit(0)
