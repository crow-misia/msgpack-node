#include <node_buffer.h>
#include <cmath>

#include "msgpack_node.h"

using namespace v8;
using namespace node;

static Persistent<FunctionTemplate> msgpack_unpack_template;


typedef struct {
  uint32_t index;
} unpack_user;

#define msgpack_unpack_struct(name) \
  struct template ## name

#define msgpack_unpack_func(ret, name) \
  ret template ## name

#define msgpack_unpack_callback(name) \
  template_callback ## name

#define msgpack_unpack_object Handle<Value>

#define msgpack_unpack_user unpack_user

#define msgpack_object Handle<Value>

#include <msgpack/unpack.h>
#include <msgpack/unpack_define.h>

struct template_context;
typedef struct template_context template_context;

static void template_init(template_context* ctx);

static Handle<Value> template_data(template_context* ctx);

static int template_execute(template_context* ctx,
    const char* data, size_t len, size_t* off);

static inline Handle<Value> template_callback_root(unpack_user* u)
{ return Undefined(); }

static inline int template_callback_uint8(unpack_user* u, uint8_t d, Handle<Value>* o)
{ *o = Integer::NewFromUnsigned(d); return 0; }

static inline int template_callback_uint16(unpack_user* u, uint16_t d, Handle<Value>* o)
{ *o = Integer::NewFromUnsigned(d); return 0; }

static inline int template_callback_uint32(unpack_user* u, uint32_t d, Handle<Value>* o)
{ *o = Integer::NewFromUnsigned(d); return 0; }

static inline int template_callback_uint64(unpack_user* u, uint64_t d, Handle<Value>* o)
{ *o = Number::New(static_cast<double>(d)); return 0; }

static inline int template_callback_int8(unpack_user* u, int8_t d, Handle<Value>* o)
{ *o = Integer::New(d); return 0; }

static inline int template_callback_int16(unpack_user* u, int16_t d, Handle<Value>* o)
{ *o = Integer::New(d); return 0; }

static inline int template_callback_int32(unpack_user* u, int32_t d, Handle<Value>* o)
{ *o = Integer::New(d); return 0; }

static inline int template_callback_int64(unpack_user* u, int64_t d, Handle<Value>* o)
{ *o = Number::New(static_cast<double>(d)); return 0; }

static inline int template_callback_float(unpack_user* u, float d, Handle<Value>* o)
{ *o = Number::New(static_cast<double>(d)); return 0; }

static inline int template_callback_double(unpack_user* u, double d, Handle<Value>* o)
{ *o = Number::New(d); return 0; }

static inline int template_callback_nil(unpack_user* u, Handle<Value>* o)
{ *o = Null(); return 0; }
 
static inline int template_callback_true(unpack_user* u, Handle<Value>* o)
{ *o = True(); return 0; }

static inline int template_callback_false(unpack_user* u, Handle<Value>* o)
{ *o = False(); return 0; }

static inline int template_callback_array(unpack_user* u, size_t n, Handle<Value>* o)
{ *o = Array::New(n); u->index = 0; return 0; }

static inline int template_callback_array_item(unpack_user* u, Handle<Value>* c, Handle<Value> o)
{
  Handle<Array> a = (*c).As<Array>();

  a->Set(u->index++, o);

  return 0;
}

static inline int template_callback_map(unpack_user* u, unsigned int n, Handle<Value>* o)
{ *o = Object::New(); return 0; }

static inline int template_callback_map_item(unpack_user* u, Handle<Value>* c, Handle<Value> k, Handle<Value> v)
{
  Handle<Object> o = (*c).As<Object>();

  o->Set(k, v);

  return 0;
}

static inline int template_callback_raw(unpack_user* u, const char* b, const char* p, unsigned int l, Handle<Value>* o)
{ *o = String::New(p, l); return 0; }

#include <msgpack/unpack_template.h>


msgpack_unpack_return
msgpack_unpack(const char* data, size_t len, size_t* off,
      msgpack_zone* result_zone, Handle<Value>* result)
{
    size_t noff = 0;
    if(off != NULL) { noff = *off; }

    if(len <= noff) {
      // FIXME
      return MSGPACK_UNPACK_CONTINUE;
    }

    template_context ctx;
    template_init(&ctx);

    int e = template_execute(&ctx, data, len, &noff);
    if(e < 0) {
      return MSGPACK_UNPACK_PARSE_ERROR;
    }

    if(off != NULL) { *off = noff; }

    if(e == 0) {
      return MSGPACK_UNPACK_CONTINUE;
    }

    *result = template_data(&ctx);

    if(noff < len) {
      return MSGPACK_UNPACK_EXTRA_BYTES;
    }

    return MSGPACK_UNPACK_SUCCESS;
}

// var o = msgpack.unpack(buf);
//
// Return the JavaScript object resulting from unpacking the contents of the
// specified buffer. If the buffer does not contain a complete object, the
// undefined value is returned.
static Handle<Value>
unpack(const Arguments &args) {
    static Persistent<String> msgpack_bytes_remaining_symbol =
        NODE_PSYMBOL("bytes_remaining");

    HandleScope scope;

    if (args.Length() < 0 || !Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a Buffer")));
    }

    const char *buf = Buffer::Data(args[0]);
    const size_t len = Buffer::Length(args[0]);

    Handle<Value> mo;
    size_t off = 0;
    switch (msgpack_unpack(buf, len, &off, NULL, &mo)) {
    case MSGPACK_UNPACK_EXTRA_BYTES:
    case MSGPACK_UNPACK_SUCCESS:
        try {
            msgpack_unpack_template->GetFunction()->Set(
                msgpack_bytes_remaining_symbol,
                Integer::NewFromUnsigned(len - off)
            );
            return scope.Close(mo);
        } catch (MsgpackException e) {
            return ThrowException(e.getThrownException());
        }

    case MSGPACK_UNPACK_CONTINUE:
        return scope.Close(Undefined());

    default:
        return ThrowException(Exception::Error(
            String::New("Error de-serializing object")));
    }
}

void unpack_initialize(const Handle<Object> target) {
    HandleScope scope;

    // Go through this mess rather than call NODE_SET_METHOD so that we can set
    // a field on the function for 'bytes_remaining'.
    msgpack_unpack_template = Persistent<FunctionTemplate>::New(
        FunctionTemplate::New(unpack)
    );
    target->Set(
        String::NewSymbol("unpack"),
        msgpack_unpack_template->GetFunction()
    );
}

// vim:ts=4 sw=4 et
