/*
** mruby/io.h - mrb_io structure, functions and macros.
**
** See Copyright Notice in mruby.h
*/

#ifndef MRUBY_IO_H
#define MRUBY_IO_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mruby/string.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby.h"


/* IO struct. Use a FILE pointer since this implementation of IO/File
* is limited to pure ANSI C stdio capabilities.
*/
struct mriso_io {
  FILE              * stream;
  char              * path;
  char              * mode;
  int                 readable;
  int                 writeable;
  int                 openness;
  int                 buffering;
};


/* IO errors. */
#define E_IO_ERROR             (mrb_class_obj_get(mrb, "IOError"))

/* mruby datatype for io. */
extern struct mrb_data_type mriso_io_type;


/* Wraps an mrb_io into ab_value */
static mrb_value
mriso_io_wrap(mrb_state *mrb, struct RClass *ioc, struct mriso_io *io);

/* Initializes an mrb_io object. */
struct mriso_io*
mriso_io_init(mrb_state * mrb, struct mriso_io * io,
            FILE * stream, const char * path, const char * mode,
            int readable, int writeable, int openness, int buffering);

struct mriso_io*
mriso_io_unwrap(mrb_state * mrb, mrb_value self);






#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif  /* MRUBY_IREP_H */
