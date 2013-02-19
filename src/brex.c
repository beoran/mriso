/* BREX: Beoran's regular expressions, 2013. MIT license. */

typedef struct BrexState_ BrexState;

/* Regexps are compiled to a nondeterministic state machine. 
 */
struct BrexState_ {
  int          match; /* Character that this state matches */
  BrexState ** out;
  int          max_out;
};




