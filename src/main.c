#include <mruby.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <libQRNG.h>

struct RClass *class_qrng;
struct RClass *class_qrngerror;
mrb_sym sym_connected;

#define QRNG_CALL(f) do { \
  int i; \
  i = f; \
  if (i != 0) { \
    mrb_raise(mrb, class_qrngerror, qrng_error_strings[i]); \
  } \
} while (0);

static mrb_value
mruby_qrng_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value username;
  mrb_value password;
  mrb_value block;

  mrb_get_args(mrb, "SS|&", &username, &password, &block);
  QRNG_CALL(qrng_connect(RSTRING_PTR(username), RSTRING_PTR(password)));
  mrb_iv_set(mrb, self, sym_connected, mrb_true_value());
  if (!mrb_nil_p(block)) {
    mrb_yield(mrb, block, self);  // does mruby have a rb_ensure counterpart?
    qrng_disconnect();
    mrb_iv_set(mrb, self, sym_connected, mrb_false_value());
  }

  return self;
}

static mrb_value
mruby_qrng_connect(mrb_state *mrb, mrb_value self)
{
  mrb_value username;
  mrb_value password;

  mrb_get_args(mrb, "SS", &username, &password);
  QRNG_CALL(qrng_connect(RSTRING_PTR(username), RSTRING_PTR(password)));
  mrb_iv_set(mrb, self, sym_connected, mrb_true_value());

  return mrb_true_value();
}

static mrb_value
mruby_qrng_disconnect(mrb_state *mrb, mrb_value self)
{
  if (mrb_test(mrb_iv_get(mrb, self, sym_connected))) {
    qrng_disconnect();
    mrb_iv_set(mrb, self, sym_connected, mrb_false_value());
    return mrb_true_value();
  }

  return mrb_false_value();
}

static mrb_value
mruby_qrng_connected(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, sym_connected);
}

static mrb_value
mruby_qrng_generate_password(mrb_state *mrb, mrb_value self)
{
  mrb_value chars;
  mrb_value length;
  mrb_value password;

  mrb_get_args(mrb, "Si", &chars, &length);
  if (length.value.i < 0 || length.value.i >= INT_MAX) {
    mrb_raise(mrb, class_qrngerror, "invalid length");
  }
  password = mrb_str_new(mrb, NULL, length.value.i);
  QRNG_CALL(qrng_generate_password(RSTRING_PTR(chars), RSTRING_PTR(password), length.value.i + 1));

  return password;
}

void
mrb_mruby_qrng_gem_init(mrb_state* mrb)
{
  class_qrng = mrb_define_class(mrb, "QRNG", mrb->object_class);
  class_qrngerror = mrb_define_class_under(mrb, class_qrng, "QRNGError", mrb->eStandardError_class);
  mrb_define_const(mrb, class_qrng, "VERSION", mrb_str_new_cstr(mrb, qrng_libQRNG_version));
  mrb_define_method(mrb, class_qrng, "initialize", mruby_qrng_initialize, ARGS_OPT(1));
  mrb_define_method(mrb, class_qrng, "connect", mruby_qrng_connect, ARGS_REQ(2));
  mrb_define_method(mrb, class_qrng, "disconnect", mruby_qrng_disconnect, ARGS_NONE());
  mrb_define_method(mrb, class_qrng, "connected?", mruby_qrng_connected, ARGS_NONE());
  mrb_define_method(mrb, class_qrng, "generate_password", mruby_qrng_generate_password, ARGS_REQ(2));
  sym_connected = mrb_intern(mrb, "connected");  // intentionally unprefixed
}
