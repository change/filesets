all: filesets test

filesets: filesets.c
	$(CC) -Wall -O3 -o filesets filesets.c

test:
	ruby fs-test.rb filesets t
	rm -f /tmp/result.txt

clean:
	rm -f filesets
	rm -f t/result.txt
	rm -f *~ t/*~
