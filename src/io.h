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


/* IO struct. Use a FILE pointer since this implementation of IO/File
* is limited to pure ANSI C stdio capabilities.
*/
struct mrb_io {
  FILE              * stream;
  mrb_value           path;
  mrb_value           mode;
  int                 readable;
  int                 writeable;
  int                 openness;
  int                 buffering;
};


/** IO errors. */
#define E_IO_ERROR             (mrb_class_obj_get(mrb, "IOError"))

/** IO mrubt datatype. */
extern struct mrb_data_type mrb_io_type;


/** Wraps an mrb_io into an mrb_value */
static mrb_value
mrb_io_wrap(mrb_state *mrb, struct RClass *ioc, struct mrb_io *io);

/** Initializes an mrb_io object. */
struct mrb_io*
mrb_io_init(mrb_state * mrb, struct mrb_io * io,
            FILE * stream, mrb_value path, mrb_value mode,
            int readable, int writeable, int openness, int buffering);

/* Unwraps an mrb_io from an mrb_value */
#ifndef mrb_io_unwrap
#define mrb_io_unwrap(MRB, SELF)                                        \
        (struct mrb_io *)mrb_get_datatype((MRB), (SELF), &mrb_io_type)
#endif




#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif  /* MRUBY_IREP_H */
