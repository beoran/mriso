
#include "mruby.h"
#ifdef ENABLE_FILE
#include <string.h>
#include <stdio.h>
#include "mruby/string.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/io.h"

/* Opens a file and stors the data into an io object. Retuns io if
the file was opened ok, or NULL on error. */
struct mrb_io *
mrb_file_open(mrb_state * mrb, struct mrb_io * io,
              mrb_value path, mrb_value mode)
{
  int readable       = FALSE;
  int writable       = FALSE;
  int openness       = FALSE; 
  int buffering      = TRUE; /* Stdio is buffered by default. */
  FILE * stream      = NULL;
  const char * cpath = RSTRING_PTR(path);
  const char * cmode = RSTRING_PTR(mode);
  if(!io)            { return NULL;     }
  stream             = fopen(cpath, cmode);
  if(stream) {
    if(RSTRING_LEN(path)>0) {
      if(cmode[0] == 'r') { readable = TRUE; } 
      if(cmode[0] == 'w') { writable = TRUE; } 
      /* non-standard extensions for file appening and +
      since they are supported by ANSI 89 C. Also allow rt+, wb+ , etc. */
      if(cmode[0] == 'a') { writable = TRUE; }       
    }
    if(RSTRING_LEN(path)>1) {
      if(cmode[1] == '+') { writable = TRUE; readable = TRUE; }
    }
    if(RSTRING_LEN(path)>2) {
        if(cmode[2] == '+') { writable = TRUE; readable = TRUE; }
    }
  }
  mrb_io_init(mrb, io, stream, path, mode,
              readable, writable, openness, buffering);
  if (stream) return io;
  return NULL;
}
  
/* 15.2.21.3.1 
* Returns true if a file exists and is readable by the current process.
* Otherwise returns false.
*/
static mrb_value
mrb_file_exist_p(mrb_state *mrb, mrb_value self) {
  FILE * stream;
  mrb_value s;
  mrb_get_args(mrb, "s", &s);
  /* It's not fool-proof, but it should give an indication. 
  That's all ANSI C's stdio can do.*/
  stream = fopen(RSTRING_PTR(s), "r");
  if(stream) {
    fclose(stream);
    return mrb_true_value();
  }
  return mrb_false_value();
}


/* 15.2.21.4.1 
* Initializes a file by opening it.
*/
static mrb_value
mrb_file_initialize(mrb_state *mrb, mrb_value self) 
{
  mrb_value     path  = mrb_str_new_cstr(mrb, "file");
  mrb_value     mode  = mrb_str_new_cstr(mrb, "r");
  struct mrb_io *io   = mrb_io_unwrap(mrb, self);
  if(!io) return self;
  if (io->stream) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "reinitializing File");
  }
  mrb_get_args(mrb, "S|S", &path, &mode);
  mrb_file_open(mrb, io, path, mode);
  return self;
}



/* 15.2.21.4.2 
* Returns the path of the File instance.
*/
static mrb_value
mrb_file_path(mrb_state *mrb, mrb_value self) 
{
  struct mrb_io * io   = mrb_io_unwrap(mrb, self);
  if(!io) return mrb_nil_value();
  return io->path;
}



void
mrb_init_file(mrb_state *mrb) 
{
  struct RClass * file;
  /* 15.2.21.1 */
  file = mrb_define_class(mrb, "File", mrb_class_get(mrb, "IO"));
  MRB_SET_INSTANCE_TT(file, MRB_TT_DATA);
  
  /* 15.2.21.3.1 */
  mrb_define_class_method(mrb, file, "exist?", mrb_file_exist_p, ARGS_REQ(1));
  
  /* 15.2.21.4.1 */
  mrb_define_method(mrb, file, "initialize", mrb_file_initialize, 
                    ARGS_REQ(1)|ARGS_OPT(1));
  /* 15.2.21.4.2 */
  mrb_define_method(mrb, file, "path", mrb_file_path, ARGS_NONE());
  
}

#else

void
mrb_init_file(mrb_state *mrb) {
}


#endif
