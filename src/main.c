#include <limits.h>
#include <stdlib.h>
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <libQRNG.h>

static struct RClass *class_qrngerror;

static mrb_sym sym_connected;
static mrb_sym sym_ssl;
static mrb_sym sym_byte;
static mrb_sym sym_int;
static mrb_sym sym_double;
//static mrb_sym sym_bytes;

static mrb_value connect();
static mrb_value disconnect();

#define ARY_MAX_SIZE (INT_MAX / sizeof(mrb_value))  // mruby/src/array.c

#define QRNG_CALL(f) do { \
  int i; \
  i = /*qrng_*/f; \
  if (i != 0) { \
    mrb_raise(mrb, class_qrngerror, qrng_error_strings[i]); \
  } \
} while (0)

#define QRNG_CALL2(f, p, n) do { \
  void *pp; \
  int to_be_read; \
  int received; \
  int i; \
  pp = p; \
  to_be_read = n; \
  received = 0; \
  do { \
    i = /*qrng_*/f(pp, to_be_read, &received); \
    if (i != 0) { \
      free(p); \
      mrb_raise(mrb, class_qrngerror, qrng_error_strings[i]); \
    } \
    pp += received; \
    to_be_read -= received; \
  } while (to_be_read > 0); \
} while (0)


static inline void *
alloc(mrb_state *mrb, size_t size)
{
  void *buf;

  buf = malloc(size);
  if (!buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "failed to allocate memory");
  }

  return buf;
}

static inline void
check_length(mrb_state *mrb, mrb_int length, size_t max)
{
  if (length < 0 || length > max) {
    mrb_raise(mrb, class_qrngerror, "invalid length");
  }
}

static mrb_value
initialize(mrb_state *mrb, mrb_value self)
{
  return connect(mrb, self);
}

static mrb_value
connect(mrb_state *mrb, mrb_value self)
{
  mrb_value username;
  mrb_value password;
  int ssl;
  mrb_value block;

  if (mrb_get_args(mrb, "SS|b&", &username, &password, &ssl, &block) > 2 && ssl) {
    QRNG_CALL(qrng_connect_SSL(RSTRING_PTR(username), RSTRING_PTR(password)));
    mrb_iv_set(mrb, self, sym_ssl, mrb_true_value());
  } else {
    QRNG_CALL(qrng_connect(RSTRING_PTR(username), RSTRING_PTR(password)));
    mrb_iv_set(mrb, self, sym_ssl, mrb_false_value());
  }
  mrb_iv_set(mrb, self, sym_connected, mrb_true_value());
  if (!mrb_nil_p(block)) {
    mrb_yield(mrb, block, self);
    disconnect(mrb, self);
  }

  return self;
}

static mrb_value
disconnect(mrb_state *mrb, mrb_value self)
{
  if (mrb_test(mrb_iv_get(mrb, self, sym_connected))) {
    qrng_disconnect();
    mrb_iv_set(mrb, self, sym_connected, mrb_false_value());
    mrb_iv_set(mrb, self, sym_ssl, mrb_false_value());
    return mrb_true_value();
  }

  return mrb_false_value();
}

static mrb_value
connected(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, sym_connected);
}

static mrb_value
ssl(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, sym_ssl);
}

static mrb_value
data(mrb_state *mrb, mrb_value self)
{
  void *buf;
  mrb_sym type;
  mrb_int size;
  mrb_value data;
  int in;  // counter and used when one int is requested
  double dn;
  struct RArray *ary;

  buf = NULL;
  if (mrb_get_args(mrb, "n|i", &type, &size) == 1) {
    size = 1;
  }
  if (type == sym_byte) {
    if (size == 1) {
      QRNG_CALL(qrng_get_int(&in));
      return mrb_str_new(mrb, (char*)&in, 1);
    }
    check_length(mrb, size, INT_MAX - 1);
    buf = malloc(size);
    QRNG_CALL2(qrng_get_byte_array, buf, size);
    data = mrb_str_new(mrb, buf, size);
    //data = mrb_funcall_argv(mrb, data, sym_bytes, 0, NULL);
  } else if (type == sym_int) {
    if (size == 1) {
      QRNG_CALL(qrng_get_int(&in));
      return mrb_fixnum_value(in);
    }
    check_length(mrb, size, ARY_MAX_SIZE);
    buf = alloc(mrb, size * sizeof(int));
    QRNG_CALL2(qrng_get_int_array, buf, size);
    data = mrb_ary_new_capa(mrb, size);
    ary = mrb_ary_ptr(data);
    for (in = 0; in < size; ++in) {
      ary->ptr[in] = mrb_fixnum_value(((int *)buf)[in]);
    }
    ary->len = size;
  } else if (type == sym_double) {
    if (size == 1) {
      QRNG_CALL(qrng_get_double(&dn));
      return mrb_float_value(dn);
    }
    check_length(mrb, size, ARY_MAX_SIZE);
    buf = alloc(mrb, size * sizeof(double));
    QRNG_CALL2(qrng_get_double_array, buf, size);
    data = mrb_ary_new_capa(mrb, size);
    ary = mrb_ary_ptr(data);
    for (in = 0; in < size; ++in) {
      ary->ptr[in] = mrb_float_value(((double *)buf)[in]);
    }
    ary->len = size;
  } else {
    mrb_raise(mrb, E_TYPE_ERROR, "invalid data type");
  }
  free(buf);

  return data;
}

static mrb_value
password(mrb_state *mrb, mrb_value self)
{
  mrb_int length;
  mrb_value chars;
  mrb_value password;

  if (mrb_get_args(mrb, "i|S", &length, &chars) == 1) {
    // cache it? where? instance variable maybe?
    chars = mrb_str_new(mrb, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 62);
  }
  check_length(mrb, length, INT_MAX - 1);
  password = mrb_str_new(mrb, NULL, length);
  QRNG_CALL(qrng_generate_password(RSTRING_PTR(chars), RSTRING_PTR(password), length + 1));

  return password;
}

void
mrb_mruby_libqrng_gem_init(mrb_state* mrb)
{
  struct RClass *class_qrng;

  class_qrng = mrb_define_class(mrb, "QRNG", mrb->object_class);
  class_qrngerror = mrb_define_class_under(mrb, class_qrng, "QRNGError", mrb->eStandardError_class);
  //mrb_define_const(mrb, class_qrng, "VERSION", mrb_str_new_cstr(mrb, qrng_libQRNG_version));
  mrb_define_method(mrb, class_qrng, "initialize", initialize, ARGS_ANY());
  mrb_define_method(mrb, class_qrng, "connect", connect, ARGS_REQ(2) | ARGS_OPT(2));
  mrb_define_method(mrb, class_qrng, "disconnect", disconnect, ARGS_NONE());
  mrb_define_method(mrb, class_qrng, "connected?", connected, ARGS_NONE());
  mrb_define_method(mrb, class_qrng, "ssl?", ssl, ARGS_NONE());
  mrb_define_method(mrb, class_qrng, "data", data, ARGS_REQ(1) | ARGS_OPT(1));
  mrb_define_method(mrb, class_qrng, "[]", data, ARGS_REQ(1) | ARGS_OPT(1));
  mrb_define_method(mrb, class_qrng, "password", password, ARGS_REQ(1) | ARGS_OPT(1));
  sym_connected = mrb_intern(mrb, "connected");
  sym_ssl = mrb_intern(mrb, "ssl");
  sym_byte = mrb_intern(mrb, "byte");
  sym_int = mrb_intern(mrb, "int");
  sym_double = mrb_intern(mrb, "double");
  //sym_bytes = mrb_intern(mrb, "bytes");
}
