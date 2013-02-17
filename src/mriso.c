#include <mruby.h>
#include <stdio.h>

static mrb_value
mrb_c_method(mrb_state *mrb, mrb_value self)
{
  puts("A C Extension");
  return self;
}

void
mrb_mriso_gem_init(mrb_state* mrb) {
  struct RClass * mriso_class = mrb_define_module(mrb, "Mriso");
  mrb_define_class_method(mrb, mriso_class, "c_method", mrb_c_method, ARGS_NONE());
}

void
mrb_mriso_gem_final(mrb_state* mrb) {
  // finalizer
}
