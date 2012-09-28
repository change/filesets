/*
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#ifdef __linux
#include <linux/limits.h>
#endif
 
#define FALSE 0
#define TRUE  (!FALSE)

#define MAX_OP_STACK  1024
#define MAX_EXP_LEN  10240
 
/* 
 * U = Union
 * X = Intersection
 * D = Difference
 * I = Inverse/Complement
 */
#define is_operator(c)  (c == 'U' || c == 'X' || c == 'D' || c == 'I')


typedef           int boolean;
typedef           int int32;
typedef unsigned  int uint32;
typedef          long int64;
typedef unsigned long uint64;


typedef enum _TokenType
{ 
  OPERATOR = 1,
  SFILE    = 2,
  SET      = 3
} TokenType;

typedef struct _Token {
  TokenType type;
  union {
    uint64 operator;
    char   file[PATH_MAX];
    char   history[MAX_EXP_LEN];
  } x;
  char * vector;
} Token;

typedef Token Set;

typedef struct _Stack {
  int32  depth;
  void * data[MAX_OP_STACK];
} Stack;


boolean Verbose   =  0;
uint32  MaxSetVal = -1;

/* -------------------------------------------------------------------- */

void
usage (void)
{
  fprintf(stderr, "\nUsage: file_sets -max id [-h] [-v] [-s] [-o outfile] expression \n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -h                 help\n");
  fprintf(stderr, "  -v                 verbose\n");
  fprintf(stderr, "  -s                 shuffle (randomize) order of id's in output\n");
  fprintf(stderr, "  -o outfile         write output to outfile (otherwise stdout)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "expression ::= ( expression )\n");
  fprintf(stderr, "             | I expession \n");
  fprintf(stderr, "             | expression binaryOp expession \n");
  fprintf(stderr, "             | file \n");
  fprintf(stderr, "binaryOp   ::= U | X | D \n");
  fprintf(stderr, "file       ::= <path to file>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Expression examples: \n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  I f1\n");
  fprintf(stderr, "  f1 U f2\n");
  fprintf(stderr, "  f1 D f2\n");
  fprintf(stderr, "  f1 D ( f2 X f3 )\n");
  fprintf(stderr, "  I ( ( f1 X f2 X f3 ) U ( f4 x f5 ) \n");
  fprintf(stderr, "  ( ( f1 X f2 X f3 ) U ( f4 x f5 ) D ( ( f6 x f7 ) U ( f8 X f9 ) )\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Notes:\n");
  fprintf(stderr, "1) files must contain only positive integers separated by newlines\n");
  fprintf(stderr, "2) all files, operators and parentheses must be separate by whitespace\n");
  fprintf(stderr, "3) operators must be upper case\n");
  fprintf(stderr, "4) operator definition\n");
  fprintf(stderr, "  U = union\n");
  fprintf(stderr, "  X = intersection\n");
  fprintf(stderr, "  D = difference\n");
  fprintf(stderr, "  I = inversion/complement (highest precedence)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\n");

  exit(-1);
}

void
seedRandom (void)
{
  FILE * fp;
  uint32 sdata[1024];
  int    i, n;

  fp = fopen ("/dev/urandom", "r");
  if ( ! fp) 
    return;

  n = fread (sdata, sizeof(uint32), 1024, fp);
  for (i = 0; i < n; i++) 
    srandom (sdata[i]);
}

Token *
tokenNew (void)
{
  Token * t;
  t = malloc (sizeof(Token));
  if (t == NULL)
  {
    fprintf (stderr, "tokenNew(): can't malloc() %ld bytes\n", sizeof(Token));
    exit(-1);
  }
  memset(t, 0, sizeof(Token));
  return (t);
}

void
tokenFree (Token * t)
{
  free(t);
}

Set *
setNew()
{
  Set * s;
  s = (Set *) tokenNew();
  s->vector = malloc (MaxSetVal + 1);
  if (s->vector == NULL)
  {
    fprintf (stderr, "setNew(): can't malloc() %u bytes\n", MaxSetVal);
    exit(-1);
  }
  memset(s->vector, 0, MaxSetVal + 1);
  s->type = SET;
  return (s);
}

void
setFree (Set * s)
{
  free(s->vector);
  tokenFree((Token *) s);
}

boolean
setRead(Set * s)
{
  int         fd;
  struct stat statBuf;
  char      * srcBase, * srcCurr, * srcEnd, * dstPtr;
  char        line[1024];
  unsigned long  id;

  /*  if (Verbose) fprintf (stderr, "     loading: %s\n", s->x.file); */

  if (s->vector == NULL)
  {
    s->vector = malloc (MaxSetVal + 1);
    if (s->vector == NULL)
    {
      fprintf (stderr, "setNew(): can't malloc() %u bytes\n", MaxSetVal);
      exit(-1);
    }
    memset(s->vector, 0, MaxSetVal + 1);
  }

  /* open the input file */
  if ((fd = open (s->x.file, O_RDONLY)) < 0)
  {
    fprintf (stderr, "\nfilesets: ERROR: can't open %s for reading \n\n", s->x.file);
    exit(-1);
  }

  /* find size of input file */
  if (fstat (fd, &statBuf) < 0)
  {
    fprintf (stderr, "fstat error: %s \n", s->x.file);
    exit(-1);
  }

  /* mmap() fails if the file is empty (zero bytes),
   * so check if the file is empty before mmap()'ing it.
   * If the file is empty, then a empty set is returned.
   */
  if (statBuf.st_size > 0) 
  {
    /* mmap the input file */
    srcBase = mmap (0, statBuf.st_size, PROT_READ,  MAP_SHARED, fd, 0);
    if (srcBase == (char *) -1)
      fprintf (stderr, "filesets: ERROR: mmap error for input file: %s \n", s->x.file);

    srcCurr = srcBase;
    srcEnd  = srcBase + statBuf.st_size;

    dstPtr = line;

    /* 
     * The following block of commented code is equivalent to the
     * uncommented code just after. The difference is that the 
     * code above mmap()'ed the file and it can now be treated
     * as one big memory block. The mmap()'ed code is faster
     * because the file data is minimally copied and no
     * buffering is performed, which would be wasted. However, 
     * the faster code is doing a bunch of pointer manipulation 
     * which may appear to be rather confusing.
     * 
     * while (fgets (line, 1024, fp)) {
     *  id = strtol (line, NULL, 10);
     *  table[id] = 1;
     * }
     *
     */
    while (srcCurr < srcEnd)
    {
      if (*srcCurr == '\n')
      {
        *dstPtr = '\0';
        id = strtol(line, NULL, 10);

        if (id > MaxSetVal)
        {
          fprintf (stderr, "\nfile-sets: ERROR: input data contains value greater than specified max ID\n\n");
          exit(-1);
        }

        if (id != LONG_MIN && id != LONG_MAX && id != 0)
          s->vector[id] = 1;
        dstPtr = line;
        srcCurr++;
      }
      *dstPtr++ = *srcCurr++;
    }

    /* Just in case the last line did not contain a newline */
    if (dstPtr != line)
    {
      *dstPtr = '\0';
      id = strtol(line, NULL, 10);

      if (id > MaxSetVal)
      {
        fprintf (stderr, "\nfile-sets: ERROR: input data contains value greater than specified max ID\n\n");
        exit(-1);
      }

      if (id != LONG_MIN && id != LONG_MAX && id != 0)
        s->vector[id] = 1;
    }

    munmap(srcBase, statBuf.st_size);
  }
  close(fd);

  s->type = SET;

  return TRUE;
}

void
setWrite(Set * s, FILE * fp)
{
  uint32 i;

  if (Verbose) printf ("Output:\n");

  if (fp == NULL) fp = stdout;

  for (i = 1; i <= MaxSetVal; i++)
    if (s->vector[i])
      fprintf (fp, "%u\n", i);

  fclose(fp);
}


Set * 
setUnion (Set * s1, Set * s2)
{
  uint32 i;
  char   buf[MAX_EXP_LEN];

  if (s1->type == SFILE)
    setRead(s1);
  if (s2->type == SFILE)
    setRead(s2);

  assert(s1->type == SET && s2->type == SET);

  for (i = 1; i <= MaxSetVal; i++)
    if (s2->vector[i])
      s1->vector[i] = s2->vector[i];

  sprintf (buf, "( %s U %s )", s1->x.history, s2->x.history);
  strcpy (s1->x.history, buf);
  setFree (s2);

  if (Verbose)
    fprintf (stderr, "%s\n", s1->x.history);

  return (s1);
}


Set * 
setDiff (Set * s1, Set * s2)
{
  uint32 i;
  char   buf[MAX_EXP_LEN];

  if (s1->type == SFILE)
    setRead(s1);
  if (s2->type == SFILE)
    setRead(s2);

  assert(s1->type == SET && s2->type == SET);

  for (i = 1; i <= MaxSetVal; i++)
    if (s2->vector[i])
      s1->vector[i] = 0;

  sprintf (buf, "( %s D %s )", s1->x.history, s2->x.history);
  strcpy (s1->x.history, buf);
  setFree(s2);

  if (Verbose)
    fprintf (stderr, "%s\n", s1->x.history);

  return (s1);
}

Set * 
setIntersect (Set * s1, Set * s2)
{
  uint32 i;
  char   buf[MAX_EXP_LEN];

  if (s1->type == SFILE)
    setRead(s1);
  if (s2->type == SFILE)
    setRead(s2);

  assert(s1->type == SET && s2->type == SET);

  for (i = 1; i <= MaxSetVal; i++)
    if (s1->vector[i] && s2->vector[i])
      s1->vector[i] = 1;
    else
      s1->vector[i] = 0;

  sprintf (buf, "( %s X %s )", s1->x.history, s2->x.history);
  strcpy (s1->x.history, buf);

  setFree (s2);

  if (Verbose)
    fprintf (stderr, "%s\n", s1->x.history);

  return (s1);
}

Set * 
setInvert (Set * s)
{
  uint32 i;
  char   buf[MAX_EXP_LEN];

  if (s->type == SFILE)
    setRead(s);

  assert(s->type == SET);

  for (i = 1; i <= MaxSetVal; i++)
    if (s->vector[i])
      s->vector[i] = 0;
    else
      s->vector[i] = 1;

  sprintf (buf, "( I %s )", s->x.history);
  strcpy (s->x.history, buf);

  if (Verbose) fprintf (stderr, "%s\n", s->x.history);

  return (s);
}

void
setShuffleAndWrite (Set * s, FILE * fp)
{
  uint32   i, j, idCnt, tmp;
  char     buf[MAX_EXP_LEN];
  uint32 * array;

  if (s->type == SFILE)
    setRead(s);

  assert(s->type == SET);

  /* Count the number of ID in the set */
  for (i = 1, idCnt = 0; i <= MaxSetVal; i++)
    if (s->vector[i])
      idCnt++;

  if (idCnt == 0)
    return;

  array = malloc (sizeof(uint32) * idCnt);
  if (array == NULL)
  {
    fprintf (stderr, "setShuffleAbdWrite(): can't malloc() %lu bytes\n", sizeof(uint32) * idCnt);
    exit (-1);
  }

  /* Map the set vector to an array */
  for (i = 1, j = 0; i <= MaxSetVal; i++)
    if (s->vector[i])
      array[j++] = i;

  seedRandom();

  /*
   * http://en.wikipedia.org/wiki/Fisher–Yates_shuffle
   *
   * To shuffle an array a of n elements (indices 0..n-1):
   * for i from n − 1 downto 1 do
   *    j ← random integer with 0 ≤ j ≤ i
   *    exchange a[j] and a[i]
   */
  for (i = (idCnt - 1); i > 0; i--)
  {
    j = (uint32) (( ((double) random()) / ((double) RAND_MAX)) * (double) i);

    /* exchange values */
    tmp = array[j];
    array[j] = array[i];
    array[i] = tmp;
  }

  sprintf (buf, "( R %s )", s->x.history);
  strcpy (s->x.history, buf);

  if (Verbose) fprintf (stderr, "%s\n", s->x.history);

  for (i = 0; i < idCnt; i++)
    fprintf (fp, "%u\n", array[i]);
}

/* -------------------------------------------------------------------- */

Stack *
stackNew (void)
{
  Stack * s;

  s = malloc (sizeof(Stack));
  if (s == NULL)
  {
    fprintf (stderr, "filesets: ERROR: stackNew() can't malloc() %lu bytes\n", sizeof(Stack));
    exit(-1);
  }
  memset(s, 0, sizeof(Stack));
  s->depth = -1;
  
  return (s);
}

boolean
stackEmpty (Stack * s)
{
  return (s->depth == -1);
}

Stack *
stackPush(Stack * s, void * data)
{
  if (s->depth == MAX_OP_STACK)
    return (FALSE);

  s->depth++;
  s->data[s->depth] = data;
  return s;
}

Stack *
stackPushOp (Stack * s, uint64 c)
{
  Token * t;
  
  t = tokenNew();
  t->type = OPERATOR;
  t->x.operator = c;

  return stackPush(s, t);
}

Stack * 
stackPushFile (Stack * s, char * filePath)
{
  Token * t;
  
  t = tokenNew();
  t->type = SFILE;
  strcpy(t->x.file, filePath);

  return stackPush(s, t);
}


void *
stackPop (Stack * s)
{
  void * data;

  if (stackEmpty(s))
    return (0);

  data = s->data[s->depth];
  s->depth--;

  return (data);
}

void *
stackPeek (Stack * s)
{
  void * data;

  if (stackEmpty(s))
    return (0);

  data = s->data[s->depth];

  return (data);
}

int32
stackDepth (Stack * s)
{
  return (s->depth + 1);
}

/*
 * pop an item from the beginning of the stack
 * bug: if depth == MAX_OP_STACK then memory is copied from past the end.
 */
void *
stackShift (Stack * s)
{
  void * data;

  if (stackEmpty(s))
    return (0);

  data = s->data[0];
  memmove (&(s->data[0]), &(s->data[1]), sizeof(void*) * s->depth + 1);
  s->depth--;

  return (data);
}

void
stackDump (Stack * s)
{
  int i;

  fprintf (stderr, "stackDump(): depth= %d\n", s->depth);
  for (i = s->depth; i >= 0; i--)
    fprintf (stderr, "\t %d:  %c  %p\n", i, (unsigned char) (uint64) s->data[i],  s->data[i]);
}


/* 
 * operators (higher = greater)
 * precedence   operators       associativity
 * 2            I               left to right
 * 1            U X D           left to right
 */
int 
op_preced (const char c)
{
  switch (c)    
  {
    case 'I':
      return 2;
    case 'U':  case 'X': case 'D':
      return 1;
    default:
      fprintf (stderr, "op_preced(): bad value: %c\n", c);
      exit (-1);
  }
  return 0;
}
 
boolean 
op_left_assoc (const char c)
{
  switch(c)
  {
    /* left to right */
    case 'U': case 'X': case 'D': case 'I':
      return TRUE;
/*
    case 'I':                                  // right to left
      return FALSE;
 */
    default:
      fprintf (stderr, "op_left_assoc(): bad value: %c\n", c);
      exit (-1);
  }
  return FALSE;
}
 
unsigned int 
op_arg_count (const char c)
{
  switch(c)
  {
    case 'U': case 'X': case 'D': 
      return 2;
    case 'I':
      return 1;
    default:
      fprintf (stderr, "op_arg_count(): bad value: %c\n", c);
      exit (-1);
  }
  return 0;
}
 

boolean 
convertToPostfix (const char *input, Stack * outputStack)
{
  boolean pe = FALSE;
  Stack * opStack;
  uint64  c;
  uint64  sc;
  char  * tok;
  char  * buffer;

  /* Since strtok() mangles the input, make a copy before calling. */
  buffer = malloc (strlen (input) + 1);
  if (buffer == NULL)
  {
    fprintf (stderr, "convertToPostfix(): Can't malloc() for buffer\n");
    exit(-1);
  }
  strcpy(buffer, input);

  opStack = stackNew();

  tok = strtok(buffer, " "); /* Pull the first token */
  while (tok != NULL)
  {
    /* printf ("tok= %s \n", tok); */
    /* stackDump (opStack); */

    c = tok[0];

    /* If the token is an operator (U, X, D, I), then: */
    if (strlen(tok) == 1 && is_operator(c))
    {
      while ( ! stackEmpty (opStack))    
      {
        sc = (unsigned long int) stackPeek(opStack);

        /* While there is an operator token, o2, at the top of the opStack
         * op1 is left-associative and its precedence is less than or equal to that of op2,
         * or op1 is right-associative and its precedence is less than that of op2,
         */
        if (is_operator(sc) &&
            ((op_left_assoc(c) && (op_preced(c) <= op_preced(sc))) ||
             (!op_left_assoc(c) && (op_preced(c) < op_preced(sc)))))   
        {
          /* Pop o2 off the opStack, onto the outStack queue; */
          stackPushOp (outputStack, (uint64) stackPop(opStack));
        }
        else
          break;
      }

      /*  push op1 onto the opStack. */
      stackPush (opStack, (void *) c);
    }
    /* If the token is a left paren, then push it onto the opStack. */
    else if (c == '(')
    {
      stackPush (opStack, (void *) c);
      pe = TRUE;
    }
    /* If the token is a right paren */
    else if (c == ')')
    {
      pe = FALSE;

      /* Until the token at the top of the opStack is a left paren,
       * pop operators off the opStack and onto the output queue
       */
      while ( ! stackEmpty (opStack))
      {
        sc = (uint64) stackPeek (opStack);
        if (sc == '(')
        {
          pe = TRUE;
          break;
        }
        else
          stackPushOp(outputStack, (uint64) stackPop (opStack));
      }

      /* mismatched paren's if the opStack empties without a left paren */
      if (pe == FALSE) {
        fprintf (stderr, "1: Error: parentheses mismatched\n");
        stackDump (opStack);
        return FALSE;
      }

      stackPop(opStack);  /* Pop the left paren from the opStack, but not onto the output queue. */
   }
    
   /* 
    * Since the token is not an operator or paren, treat
    * it as a file by adding it to the output queue.
    */
   else 
     stackPushFile (outputStack, tok);


    tok = strtok(NULL, " ");
  }

  /* When there are no more tokens to read and
   * while there are still operator tokens in the opStack:
   */
  while ( ! stackEmpty (opStack))
  {
    sc = (uint64) stackPop (opStack);
    if (sc == '(' || sc == ')')   
    {
      fprintf (stderr, "2: Error: parentheses mismatched\n");
      return FALSE;
    }
    stackPushOp(outputStack, sc);
  }
  return TRUE;
}
 
Set * 
execute (Stack * input) 
{
  Token * tok;
  Set   * s1, * s2;
  Stack * opStack;
  uint32  opCnt    = 0;

  if (Verbose) printf ("order:\n");

  opStack = stackNew();

  tok = stackShift (input);
  while (tok != NULL)  
  {
    if (tok->type == OPERATOR)
    {
      /* printf ("tok->operator= %c\n", tok->x.operator); */
      if (Verbose) fprintf (stderr, "%02d = ", opCnt);

      opCnt++;

      /* 
       * Each operator takes a defined number of arguments. Err
       * if there are fewer than the expected num on the stack.
       */
      if (stackDepth(opStack) < op_arg_count(tok->x.operator)) 
      {
        fprintf (stderr, 
                 "execution_order(): insufficient values for the current operater (%c)\n", 
                 (char) tok->x.operator);
        return FALSE;
      }

      /* Else, Pop the top n values from the stack. */
      if (tok->x.operator == 'I')
        s1 = setInvert (stackPop (opStack)); 
      else /* U, X, D */
      {
        s2 = stackPop(opStack);
        s1 = stackPop(opStack);

        switch (tok->x.operator) 
        {
          case 'U':
            s1 = setUnion(s1, s2);
            break;
          case 'D':
            s1 = setDiff(s1, s2);
            break;
          case 'X':
            s1 = setIntersect(s1, s2);
            break;
          default:
            assert(0);
        }
      }

      /* Push the returned results */
      stackPush(opStack, s1);
    }
    else  /* If the token is a value */
      stackPush(opStack, tok);

    tok = stackShift (input);
  }

  /* If there is only one value in the stack,
   * that value is the result of the calculation.
   */
  if (stackDepth (opStack) == 1) 
  {
    s1 = stackPop (opStack);

    if (s1->type == SFILE)
      setRead(s1);

    if (s1->type != SET)
    {
      fprintf (stderr, "execute(): result != a Set\n");
      exit (-1);
    }

    return ((Set * ) s1);
  }

  /* Anything but one value on the output stack is an error. */
  return NULL;
}

char *
cmdLine (int argc, char *argv[])
{
  int i;
  char * buf;

  buf = malloc(MAX_EXP_LEN);
  memset(buf, 0, MAX_EXP_LEN);

  for (i = 0; i < argc; i++) {
    strcat (buf, argv[i]);
    strcat (buf, " ");
  }

  return (buf);
}

int 
main (int argc, char *argv[]) 
{
  char    input[MAX_EXP_LEN];
  FILE  * outFile;
  Stack * outputStack;
  char  * currPos;
  int     i;
  size_t  len;
  boolean shuffle = FALSE;
  Set   * resultSet;

  if (argc == 1)
    usage();

  outFile = stdout;
  outputStack = stackNew();

  memset(input, 0, MAX_EXP_LEN);
  currPos = input;

  for (i = 1; i < argc; i++) 
  {
    if (strcmp(argv[i], "-h") == 0)
      usage();
 
     if (strcmp(argv[i], "-v") == 0)
    {
      Verbose = TRUE;
      continue;
    }

    if (strcmp(argv[i], "-s") == 0)
    {
      shuffle = TRUE;
      continue;
    }

   if (strcmp(argv[i], "-o") == 0)
    {
      i++;
      if ((outFile = fopen(argv[i], "w")) == NULL)
      {
        fprintf (stderr, "\nfilesets: ERROR: Can't open output file: %s\n\n", argv[i]);
        usage();
      }
      continue;
    }

    if (strcmp(argv[i], "-max") == 0)
    {
      i++;
      MaxSetVal = strtol(argv[i], NULL, 10);
      if (MaxSetVal == 0 || MaxSetVal == UINT_MAX)
      {
        fprintf (stderr, "\nfilesets: ERROR: Max Id must be an integer greater than zero.\n");
        usage();
      }
      continue;
    }
    
    len = strlen(argv[i]);
    memcpy (currPos, argv[i], len);
    currPos += len;
    memcpy(currPos, " ", 1);
    currPos++;
  }

  if (Verbose) fprintf (stderr, "input: %s\n", input);

  if (MaxSetVal == -1)
  {
    fprintf (stderr, "\nfilesets: ERROR: The max ID (-max) is a required option and must be positive integer.\n");
    usage();
  }

  if (convertToPostfix (input, outputStack))
  {
    /*
    for (i = 0; Verbose && i < outputStack.depth; i++)
    {
      Token * t;
      t = (Token *) outputStack.data[i];
      if (t->type == OPERATOR)
        fprintf (stderr, "%d \t %c \n", i, t->x.operator);
      else
        fprintf (stderr, "%d \t %s \n", i, t->x.file);
    }
    */

    resultSet = execute (outputStack);
    if (resultSet) 
    {
      if (shuffle == TRUE)
        setShuffleAndWrite (resultSet, outFile);
      else
        setWrite (resultSet, outFile);
    }
    else
    {
      fprintf (stderr, "\nfile-sets: ERROR: Invalid input\n\t%s\n", cmdLine(argc, argv));
      exit(-1);
    }
    
  }
  return 0;
}
