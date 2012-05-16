# Filesets

Perform set operations (union, intersection, difference, complement) on integers where each set is specified in a separate text file. See the *Usage* section below for examples.

## Note

This gem is not a Ruby gem in the traditional or expected sense. Instead it is a C program packaged as a Ruby gem in order to faciliate integration with the Change.org application and developer workflow.

## Building

Installing the gem will automatically compile and test the executable.

If you wish to build by hand, cd into the directory and type 'make'.

The program has been tested on both Linux and Mac and compiles cleanly (no warnings).


## Usage

    file_sets -max id [-h] [-v] [-s] [-o outfile] expression 
   
     -h                 help
     -v                 verbose
     -s                 shuffle (randomize) order of id's in output
     -o outfile         write output to outfile (otherwise stdout)

    expression ::= ( expression )
                | I expession 
                | expression binaryOp expession 
                | file 
    binaryOp   ::= U | X | D 
    file       ::= <path to file>
     
    Expression examples: 
     
      I f1
      f1 U f2
      f1 D f2
      f1 D ( f2 X f3 )
      I ( ( f1 X f2 X f3 ) U ( f4 x f5 ) 
      ( ( f1 X f2 X f3 ) U ( f4 x f5 ) D ( ( f6 x f7 ) U ( f8 X f9 ) )
    
    Notes:
    1) files must contain only integers separated by newlines
    2) all files, operators and parenthesss must be separate by whitespace
    3) operators must be upper case
    4) operator definition
      U = union
      X = intersection
      D = difference
      I = inversion/complement (highest precedence)


## Additional Notes

### Algorithmic Complexity

filesets is fast because it does not take an optimal Computer Science approach. It does this in three ways:

1. Knowing that the range of possible set values (IDs) are integers: 0 < n <= MAX ID
2. Representing a set with a vector (byte array) the size of MAX_ID. If ID is in the set then set[ID] = 1
3. Performing set operation using an O(n) algorithm instead of O(log n)

The algorithm for performing a union is: 

    for (i = 1; i <= MaxSetVal; i++)
      if (set2[i])
        set11[i] = set2[i];

(Difference, intersection, and complement/inversion are similar.)

The above approach is naive because MAX ID elements must be examined for every set operation. This can be wasteful if MAX ID is large and the number of IDs in the sets are few.

Implementing an algorithm that operates in O(log n) time requires using some sort of Tree data structure along the lines of a Hash Table. Ironically, in typical cases, doing so is both slower and uses more memory than the naive approach above. This has been verified using the GHashTable data structure from the gLib library as well as the SparseHash library from Google.

It turns out that dynamically allocating the nodes of a Tree and managing insertions and deletions requires more CPU cycles than the naive vector approach.

In addition, the memory utililzation is also higher. The naive approaches requires one byte per ID. A Tree data struct at least 12 bytes in a 32-bit OS and 20 bytes in a 64-bit OS. Presuming a MAX ID of 2**32 which can be represent a a unsigned 4 byte integer.

    32-bit OS: 12 bytes = 4 bytes for the ID + 2 * 4 bytes for pointers to the left & right nodes
    64-bit OS: 20 bytes = 4 bytes for the ID + 2 * 8 bytes for pointers to the left & right nodes


### Future Proofing

As of May 16 2012, the maximum uer ID in the Change.org database is a around 20M. filesets has been tested using a maximum user ID of 2B. If the maximum user ID every becomes great enough that allocating the memory becomes a problem, then the program can be modified to utilize one bit per ID instead of one byte. (This will likely cause loading a set from file to be a bit slower, but actually make the set operations faster.) See: http://en.wikipedia.org/wiki/Bit_array

