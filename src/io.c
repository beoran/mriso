/*
** io.c - IO class
**
** See Copyright Notice in mruby.h
*/
#include "mruby.h"
#ifdef ENABLE_FILE
#include <string.h>
#include <stdio.h>
#include "mruby/io.h"
#include "mruby/string.h"
#include "mruby/class.h"
#include "mruby/data.h"


#define E_IO_ERROR             (mrb_class_obj_get(mrb, "IOError"))

static void
mrb_io_free(mrb_state *mrb, void *ptr)
{
  mrb_free(mrb, ptr);
}


struct mrb_data_type mrb_io_type = { "IO", mrb_io_free };


/* Wraps an mrb_io into an mrb_value */
static mrb_value
mrb_io_wrap(mrb_state *mrb, struct RClass *ioc, struct mrb_io *io)
{
  return mrb_obj_value(Data_Wrap_Struct(mrb, ioc, &mrb_io_type, io));
}

/* Allocates a mrb_io object. Does initializes it to be empty. */
static struct mrb_io*
mrb_io_alloc(mrb_state *mrb) 
{
  struct mrb_io *io;
  io = (struct mrb_io *) mrb_malloc(mrb, sizeof(struct mrb_io));  
  io->stream    = NULL;
  io->path      = mrb_nil_value();
  io->mode      = mrb_nil_value();
  io->readable  = FALSE;
  io->writeable = FALSE;
  io->openness  = FALSE;
  io->buffering = FALSE;
  return io;
}


/* Initializes an mrb_io object. */
struct mrb_io*
mrb_io_init(mrb_state * mrb, struct mrb_io * io, 
            FILE * stream, mrb_value path, mrb_value mode,
            int readable, int writeable,
            int openness, int buffering) {
  if(!io) return NULL;
  io->stream    = stream;
  io->path      = path;
  io->mode      = mode;
  io->readable  = readable;
  io->writeable = writeable;
  io->openness  = openness;
  io->buffering = buffering;
  return io;
}

/* Unwraps an mrb_io from an mrb_value */
#ifndef mrb_io_unwrap
#define mrb_io_unwrap(MRB, SELF)                                        \
        (struct mrb_io *)mrb_get_datatype((MRB), (SELF), &mrb_io_type)
#endif

/* Returns true if the io is closed, false if not. */
static int mrb_io_isclosed(struct mrb_io * io) {
  return ((!io->stream));
}

/* Returns true if the io is writable, false if not. */
static int mrb_io_iswritable(struct mrb_io * io) {
  return ((io->stream) && (io->writeable));
}

/* Returns true if the io is readable, false if not. */
static int mrb_io_isreadable(struct mrb_io * io) {
  return ((io->stream) && (io->readable));
}


/* 15.2.20.1 
* Creates a new instance of IO for the use of 
*/
static mrb_value
mrb_io_new(mrb_state *mrb, mrb_value self) {
    int                argc;
    mrb_value          *argv;
    int                n;
    struct mrb_io     * io  = mrb_io_alloc(mrb);
    mrb_value           vio = mrb_io_wrap(mrb, mrb_class_ptr(self), io);
    
    if (!mrb_respond_to(mrb, vio, mrb_intern(mrb, "initialize"))) {
      fprintf(stderr, "Not invoking initialize: %d\n", n);
      return vio;
    }
    
    n = mrb_get_args(mrb, "*", &argv, &argc);
    return mrb_funcall_argv(mrb, vio, mrb_intern(mrb, "initialize"), argc, argv);
}


/* 15.2.20.5.1 
* Closes the underlying stream of this IO object. Raieses a IOException if 
* it was already closed.
*/
static mrb_value
mrb_io_close(mrb_state *mrb, mrb_value self) {
    struct mrb_io * io = mrb_io_unwrap(mrb, self);
    if(mrb_io_isclosed(io)) {
      mrb_raise(mrb, E_IO_ERROR, "closed stream");
    }
    fclose(io->stream);
    io->stream    = NULL;
    io->openness  = FALSE;
    io->readable  = FALSE;
    io->writeable = FALSE;
    return mrb_nil_value();
}


/* 15.2.20.5.2 
* Returns true if the stream was closed, false if not.
*/
static mrb_value
mrb_io_close_p(mrb_state *mrb, mrb_value self) {
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(mrb_io_isclosed(io)) return mrb_true_value();
  return mrb_false_value();
}


/* 15.2.20.5.6 
* Returns true if the stream is at the end, false if not.
*/
static mrb_value
mrb_io_eof_p(mrb_state *mrb, mrb_value self) {
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(!mrb_io_isreadable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  if(feof(io->stream)) mrb_true_value();
  return mrb_false_value();
}


/* 15.2.20.5.7
* Flushes the buffered data to the stream.
*/
static mrb_value
mrb_io_flush(mrb_state *mrb, mrb_value self) {
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(!mrb_io_iswritable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  if(io->buffering) { 
    fflush(io->stream);
  }
  return self;
}


/* 15.2.20.5.8 
* Reads a single character from the stream. Return the character read 
* as a Fixnum, or nil if the stream is at the end.
*/
static mrb_value
mrb_io_getc(mrb_state *mrb, mrb_value self) {
  int ch;
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(!mrb_io_isreadable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  ch = fgetc(io->stream);
  if (ch == EOF) return mrb_nil_value();
  return mrb_fixnum_value(ch);
}

/* 15.2.20.5.8 
* Reads a line from the stream. Return the line read 
* as a String, or nil if the stream is at the end.
*/
static mrb_value
mrb_io_gets(mrb_state *mrb, mrb_value self) {
  mrb_value result;
  char * line;
  char buffer[1024];
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(!mrb_io_isreadable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  
  //XXX: won't work for a file withe extremely long lines...
  if(!fgets(buffer, 1024, io->stream)) { 
    mrb_nil_value();
  }
  return mrb_str_new_cstr(mrb, buffer);
}


/* 15.2.20.5.7
* Writes a single character to the stream.
*/
static mrb_value
mrb_io_putc(mrb_state *mrb, mrb_value self) {
  int ch;
  mrb_value val;
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(!mrb_io_iswritable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  // XXX/ does not invoke write, but fputc since it seems more logical
  // and will be more performant
  mrb_get_args(mrb, "o", &val);
  switch (mrb_type(val)) {
  case MRB_TT_FIXNUM:
    fputc(mrb_fixnum(val), io->stream);
    return val;
  case MRB_TT_STRING:
    if (RSTRING_LEN(val) > 0) {
      fputc(RSTRING_PTR(val)[0], io->stream);
    }
    return val;
    default:
    mrb_raise(mrb, E_TYPE_ERROR, "Fixnum or String");
  }
  return val;
}


/* 15.2.20.5.14 
* Reads a string with given length from the stream.
* If length is not given read the whole stream.
*/
static mrb_value
mrb_io_read(mrb_state *mrb, mrb_value self) {
  int read;
  mrb_value length = mrb_nil_value();
  mrb_value result = mrb_str_buf_new(mrb, 1024);
  char * dynbuf;
  char buffer[1024];
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  if(!mrb_io_isreadable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  mrb_get_args(mrb, "|i", &length);
  if(mrb_nil_p(length)) {
    do { 
      read = fread(buffer, 1024, 1, io->stream);
      if(read) mrb_str_cat(mrb, result, buffer, read);
    } while(read == 1024);
  } else {
    int toread = mrb_fixnum(length);
    if(toread<0) mrb_raise(mrb, E_ARGUMENT_ERROR, "positive integer expected");
    dynbuf = mrb_malloc(mrb, toread);
    read = fread(dynbuf, toread, 1, io->stream);
    if(read) mrb_str_cat(mrb, result, dynbuf, read);
    mrb_free(mrb, dynbuf);
  }
  return result;
}


/* 15.2.20.5.20 
* Writes a string to the stream.
*/
static mrb_value
mrb_io_write(mrb_state *mrb, mrb_value self) {
  size_t result;
  struct mrb_io * io = mrb_io_unwrap(mrb, self);
  mrb_value s;
  mrb_get_args(mrb, "o", &s);
  s = mrb_obj_as_string(mrb, s);
  if(RSTRING_LEN(s) < 1) {
    return mrb_fixnum_value(0);
  }
  if(!mrb_io_iswritable(io)) {
    mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  result = fwrite(RSTRING_PTR(s), RSTRING_LEN(s), 1, io->stream);
  return mrb_fixnum_value(result);
}




void
mrb_init_io(mrb_state *mrb)
{
  struct RClass *io;
  /* 15.2.20.1 */
  io = mrb_define_class(mrb, "IO", mrb->object_class);
  MRB_SET_INSTANCE_TT(io, MRB_TT_DATA);
  
  /* 15.2.20.1 */
  mrb_define_class_method(mrb, io, "new", mrb_io_new, ARGS_ANY());
  
  /* 15.2.20.1.3 */
  // mrb_include_module(mrb, io, mrb_class_get(mrb, "Enumerabe"));
  
  /* 15.2.20.4.1 open is in mrblib */
  // mrb_define_class_method(mrb, io, "open", mrb_io_open, ARGS_ANY());
  
  /* 15.2.20.5.1 */
  mrb_define_method(mrb, io, "close", mrb_io_close, ARGS_NONE());
  
  /* 15.2.20.5.2 */
  mrb_define_method(mrb, io, "close?", mrb_io_close_p, ARGS_NONE());
  
#ifdef THIS_IS_COMMENT___
  
  /* 15.2.20.5.3 */
  mrb_define_method(mrb, io, "each", mrb_io_each, ARGS_NONE());

  /* 15.2.20.5.4 */
  mrb_define_method(mrb, io, "each_byte", mrb_io_each_byte, ARGS_NONE());
  
  /* 15.2.20.5.5 */
  mrb_define_method(mrb, io, "each_line", mrb_io_each_line, ARGS_NONE());
  
  /* 15.2.20.5.6 */
  mrb_define_method(mrb, io, "eof?", mrb_io_eof_p, ARGS_NONE());
  
  /* 15.2.20.5.7 */
  mrb_define_method(mrb, io, "flush", mrb_io_flush, ARGS_NONE());

  /* 15.2.20.5.8 */
  mrb_define_method(mrb, io, "getc", mrb_io_getc, ARGS_NONE());
  
  /* 15.2.20.5.9 */
  mrb_define_method(mrb, io, "gets", mrb_io_gets, ARGS_NONE());
  
  /* 15.2.20.5.10 */
  mrb_define_method(mrb, io, "initialize_copy", mrb_io_initcopy, ARGS_REQ(1));
  
  /* 15.2.20.5.11 */
  mrb_define_method(mrb, io, "print"    , mrb_io_print, ARGS_ANY());
  /* 15.2.20.5.12 */
  mrb_define_method(mrb, io, "putc"     , mrb_io_putc, ARGS_REQ(1));
  /* 15.2.20.5.13 */
  mrb_define_method(mrb, io, "puts"     , mrb_io_puts, ARGS_ANY);
  /* 15.2.20.5.15 */
  mrb_define_method(mrb, io, "readchar" , mrb_io_readchar, ARGS_NONE());
  /* 15.2.20.5.16 */
  mrb_define_method(mrb, io, "readline" , mrb_io_readline, ARGS_NONE());
  /* 15.2.20.5.17 */
  mrb_define_method(mrb, io, "readlines", mrb_io_readlines, ARGS_NONE());
  /* 15.2.20.5.18 */
  mrb_define_method(mrb, io, "sync", mrb_io_sync, ARGS_NONE());
  /* 15.2.20.5.19 */
  mrb_define_method(mrb, io, "sync=", mrb_io_sync_, ARGS_REQ(1));

#endif

  /* 15.2.20.5.14 */
  mrb_define_method(mrb, io, "read"     , mrb_io_read, ARGS_OPT(1));

  /* 15.2.20.5.20 */
  mrb_define_method(mrb, io, "write", mrb_io_write, ARGS_REQ(1));

  /*
  mrb_define_method(mrb, io, "", mrb_io_, ARGS_());
  */

}


#else

void
mrb_init_io(mrb_state *mrb) {
}



#endif
