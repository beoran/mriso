/*
** io.c - ISO Ruby IO class that uses ANSI C FILE* only.
**
** See Copyright Notice in mruby.h
*/
#include "mruby.h"
#include "io.h"

#define E_IO_ERROR             (mrb_class_obj_get(mrb, "IOError"))

static void
mriso_io_free(mrb_state *mrb, void *ptr)
{
  mrb_free(mrb, ptr);
}


struct mrb_data_type mriso_io_type = { "IO", mriso_io_free };

/** Unwraps an mriso_io into an mrb_value. */
struct mriso_io*
mriso_io_unwrap(mrb_state * mrb, mrb_value self) {
  return mrb_get_datatype(mrb, self, &mriso_io_type);
}


/* Wraps an mriso_io into an mrb_value */
static mrb_value
mriso_io_wrap(mrb_state *mrb, struct RClass *ioc, struct mriso_io *io)
{
  return mrb_obj_value(Data_Wrap_Struct(mrb, ioc, &mriso_io_type, io));
}

/* Allocates a mriso_io object. Does initializes it to be empty. */
static struct mriso_io*
mriso_io_alloc(mrb_state *mrb) 
{
  struct mriso_io *io;
  io = (struct mriso_io *) mrb_malloc(mrb, sizeof(struct mriso_io));  
  io->stream    = NULL;
  io->path      = NULL;
  io->mode      = NULL;
  io->readable  = FALSE;
  io->writeable = FALSE;
  io->openness  = FALSE;
  io->buffering = FALSE;
  return io;
}


/* Initializes an mriso_io object. */
struct mriso_io*
mriso_io_init(mrb_state * mrb, struct mriso_io * io, 
            FILE * stream, const char * path, const char * mode,
            int readable, int writeable,
            int openness, int buffering) {
  if(!io) return NULL;
  io->stream    = stream;
  io->path      = strdup(path);
  io->mode      = strdup(mode);
  io->readable  = readable;
  io->writeable = writeable;
  io->openness  = openness;
  io->buffering = buffering;
  return io;
}

/* Unwraps an mriso_io from an mrb_value. */
#ifndef mriso_io_unwrap
#define mriso_io_unwrap(MRB, SELF)                                        \
        (struct mriso_io *)mrb_get_datatype((MRB), (SELF), &mriso_io_type)
#endif

/* Returns true if the io is closed, false if not. */
static int mriso_io_isclosed(struct mriso_io * io) {
  return ((!io->stream));
}

/* Returns true if the io is writable, false if not. */
static int mriso_io_iswritable(struct mriso_io * io) {
  return ((io->stream) && (io->writeable));
}

/* Returns true if the io is readable, false if not. */
static int mriso_io_isreadable(struct mriso_io * io) {
  return ((io->stream) && (io->readable));
}

/* Returns an unwrapped IO, but only if it is writable. 
 Raises an exception otherwise. */
static struct mriso_io * 
mriso_io_unwrap_writable(mrb_state * mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(!mriso_io_iswritable(io)) { 
     mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  return io;
}  

/* Returns an unwrapped IO, but only if it is readable. 
 Raises an exception otherwise. */
static struct mriso_io * 
mriso_io_unwrap_readable(mrb_state * mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(!mriso_io_iswritable(io)) { 
     mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  return io;
}  



/* 15.2.20.1 
* Creates a new instance of IO for the use of File, etc. 
*/
static mrb_value
mriso_io_new(mrb_state *mrb, mrb_value self) {
    int                argc;
    mrb_value          *argv;
    int                n;
    struct mriso_io  * io  = mriso_io_alloc(mrb);
    mrb_value          vio = mriso_io_wrap(mrb, mrb_class_ptr(self), io);
    
    if (!mrb_respond_to(mrb, vio, mrb_intern(mrb, "initialize"))) {
      fprintf(stderr, "Not invoking initialize: %d\n", n);
      return vio;
    }
    
    n = mrb_get_args(mrb, "*", &argv, &argc);
    return mrb_funcall_argv(mrb, vio, mrb_intern(mrb, "initialize"), argc, argv);
}


/* 15.2.20.5.1 
* Closes the underlying stream of this IO object. Raises a IOException if 
* it was already closed.
*/
static mrb_value
mriso_io_close(mrb_state *mrb, mrb_value self) {
    struct mriso_io * io = mriso_io_unwrap(mrb, self);
    if(mriso_io_isclosed(io)) {
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
mriso_io_close_p(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(mriso_io_isclosed(io)) return mrb_true_value();
  return mrb_false_value();
}


/* 15.2.20.5.6 
* Returns true if the stream is at the end, false if not.
*/
static mrb_value
mriso_io_eof_p(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(!mriso_io_isreadable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  if(feof(io->stream)) mrb_true_value();
  return mrb_false_value();
}


/* 15.2.20.5.7
* Flushes the buffered data to the stream.
*/
static mrb_value
mriso_io_flush(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(!mriso_io_iswritable(io)) { 
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
mriso_io_getc(mrb_state *mrb, mrb_value self) {
  int ch;
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(!mriso_io_isreadable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for reading");
  }
  ch = fgetc(io->stream);
  if (ch == EOF) return mrb_nil_value();
  return mrb_fixnum_value(ch);
}


/* Returns a malloced buffer that contains one line read from the file. 
 * The buffer will expand automatically but may be arbitrary large. 
 * The newline followed by a \0 are stored in the buffer. 
 * If memory runs out, during reading, 
 * returns a truncated line, without the \n, but with the \0 appended.
 */
char * mriso_gets(FILE * stream) {
  int    ch;
  char * result;
  char * aid;
  return NULL;
}

/* 15.2.20.5.8 
* Reads a line from the stream. Return the line read 
* as a String, or nil if the stream is at the end.
*/
static mrb_value
mriso_io_gets(mrb_state *mrb, mrb_value self) {
  mrb_value result;
  char * line;
  char buffer[1024];
  struct mriso_io * io = mriso_io_unwrap_readable(mrb, self);
  
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
mriso_io_putc(mrb_state *mrb, mrb_value self) {
  int ch;
  mrb_value val;
  struct mriso_io * io = mriso_io_unwrap_writable(mrb, self);
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

/* 15.2.20.5.xxx
* Prinnts a string to the stream.
*/
static mrb_value
mriso_io_print(mrb_state *mrb, mrb_value self) {
  int ch;
  mrb_value val;
  struct mriso_io * io = mriso_io_unwrap_writable(mrb, self);
  if(!mriso_io_iswritable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  // XXX/ does not invoke write, but fprintf, etc since it seems more logical
  // and will be more performant
  mrb_get_args(mrb, "o", &val);
  switch (mrb_type(val)) {
  case MRB_TT_FIXNUM:
    fputc(mrb_fixnum(val), io->stream);
    return val;
  case MRB_TT_STRING:
    if (RSTRING_LEN(val) > 0) {
      fprintf(io->stream, "%s", RSTRING_PTR(val));
    }
    return val;
    default:
    mrb_raise(mrb, E_TYPE_ERROR, "Fixnum or String");
  }
  return val;
}


/* 15.2.20.5.xxx
* Writes a string to the stream.
*/
static mrb_value
mriso_io_puts(mrb_state *mrb, mrb_value self) {
  int ch;
  mrb_value val;
  struct mriso_io * io = mriso_io_unwrap_writable(mrb, self);
  if(!mriso_io_iswritable(io)) { 
    mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  // XXX/ does not invoke write, but fputs since it seems more logical
  // and will be more performant
  mrb_get_args(mrb, "o", &val);
  switch (mrb_type(val)) {
  case MRB_TT_FIXNUM:
    fputc(mrb_fixnum(val), io->stream);
    return val;
  case MRB_TT_STRING:
    if (RSTRING_LEN(val) > 0) {
      fputs(RSTRING_PTR(val), io->stream);
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
mriso_io_read(mrb_state *mrb, mrb_value self) {
  int read;
  mrb_value length = mrb_nil_value();
  mrb_value result = mrb_str_buf_new(mrb, 1024);
  char * dynbuf;
  char buffer[1024];
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if(!mriso_io_isreadable(io)) { 
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
mriso_io_write(mrb_state *mrb, mrb_value self) {
  size_t result;
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  mrb_value s;
  mrb_get_args(mrb, "o", &s);
  s = mrb_obj_as_string(mrb, s);
  if(RSTRING_LEN(s) < 1) {
    return mrb_fixnum_value(0);
  }
  if(!mriso_io_iswritable(io)) {
    mrb_raise(mrb, E_IO_ERROR, "not opened for writing");
  }
  result = fwrite(RSTRING_PTR(s), RSTRING_LEN(s), 1, io->stream);
  return mrb_fixnum_value(result);
}


/*
 *
 */
static mrb_value
mriso_io_initcopy(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  return mrb_nil_value();
}


/*
 *
 */
static mrb_value
mriso_io_readchar(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  return mrb_nil_value();
}

/*
 *
 */
static mrb_value
mriso_io_readline(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  return mrb_nil_value();
}

/*
 *
 */
static mrb_value 
mriso_io_readlines(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  return mrb_nil_value();
}



/*
 *
 */
static mrb_value
mriso_io_sync(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  if ((io->buffering) > 0) { 
    return mrb_false_value();
  } 
  return mrb_true_value();
}

/*
 *
 */
static mrb_value
mriso_io_sync_(mrb_state *mrb, mrb_value self) {
  struct mriso_io * io = mriso_io_unwrap(mrb, self);
  int i;
  mrb_get_args(mrb, "i", &i);
  io->buffering = i; 
  return mrb_fixnum_value(0);
}




void
mrb_init_io(mrb_state *mrb)
{
  struct RClass *io;
  /* 15.2.20.1 */
  io = mrb_define_class(mrb, "IO", mrb->object_class);
  MRB_SET_INSTANCE_TT(io, MRB_TT_DATA);
  
  /* 15.2.20.1 */
  mrb_define_class_method(mrb, io, "new", mriso_io_new, ARGS_ANY());
  
  /* 15.2.20.1.3 */
  // mrb_include_module(mrb, io, mrb_class_get(mrb, "Enumerabe"));
  
  /* 15.2.20.4.1 open is in mrblib */
  // mrb_define_class_method(mrb, io, "open", mriso_io_open, ARGS_ANY());
  
  /* 15.2.20.5.1 */
  mrb_define_method(mrb, io, "close", mriso_io_close, ARGS_NONE());
  
  /* 15.2.20.5.2 */
  mrb_define_method(mrb, io, "close?", mriso_io_close_p, ARGS_NONE());
  
#ifdef THIS_IS_COMMENT___
  
  /* 15.2.20.5.3 */
  mrb_define_method(mrb, io, "each", mriso_io_each, ARGS_NONE());

  /* 15.2.20.5.4 */
  mrb_define_method(mrb, io, "each_byte", mriso_io_each_byte, ARGS_NONE());
  
  /* 15.2.20.5.5 */
  mrb_define_method(mrb, io, "each_line", mriso_io_each_line, ARGS_NONE());

  
#endif
  /* 15.2.20.5.6 */
  mrb_define_method(mrb, io, "eof?", mriso_io_eof_p, ARGS_NONE());
  
  /* 15.2.20.5.7 */
  mrb_define_method(mrb, io, "flush", mriso_io_flush, ARGS_NONE());
  /* 15.2.20.5.8 */
  mrb_define_method(mrb, io, "getc", mriso_io_getc, ARGS_NONE());
  
  /* 15.2.20.5.9 */
  mrb_define_method(mrb, io, "gets", mriso_io_gets, ARGS_NONE());
  
  /* 15.2.20.5.10 */
  mrb_define_method(mrb, io, "initialize_copy", mriso_io_initcopy, ARGS_REQ(1));
  
  /* 15.2.20.5.11 */
  mrb_define_method(mrb, io, "print"    , mriso_io_print, ARGS_ANY());
  /* 15.2.20.5.12 */
  mrb_define_method(mrb, io, "putc"     , mriso_io_putc, ARGS_REQ(1));
  /* 15.2.20.5.13 */
  mrb_define_method(mrb, io, "puts"     , mriso_io_puts, ARGS_ANY());
  /* 15.2.20.5.15 */
  mrb_define_method(mrb, io, "readchar" , mriso_io_readchar, ARGS_NONE());
  /* 15.2.20.5.16 */
  mrb_define_method(mrb, io, "readline" , mriso_io_readline, ARGS_NONE());
  /* 15.2.20.5.17 */
  mrb_define_method(mrb, io, "readlines", mriso_io_readlines, ARGS_NONE());
  /* 15.2.20.5.18 */
  mrb_define_method(mrb, io, "sync", mriso_io_sync, ARGS_NONE());
  /* 15.2.20.5.19 */
  mrb_define_method(mrb, io, "sync=", mriso_io_sync_, ARGS_REQ(1));

  /* 15.2.20.5.14 */
  mrb_define_method(mrb, io, "read"     , mriso_io_read, ARGS_OPT(1));

  /* 15.2.20.5.20 */
  mrb_define_method(mrb, io, "write", mriso_io_write, ARGS_REQ(1));

  /*
  mrb_define_method(mrb, io, "", mriso_io_, ARGS_());
  */

}

