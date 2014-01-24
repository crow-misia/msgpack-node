#include <node_buffer.h>
#include <cmath>

#include "msgpack_node.h"

using namespace v8;
using namespace node;

static Persistent<FunctionTemplate> msgpack_unpack_template;

// Convert a MessagePack object to a V8 object.
//
// This method is recursive. It will probably blow out the stack on objects
// with extremely deep nesting.
static Handle<Value>
msgpack_to_v8(msgpack_object *mo) {
    switch (mo->type) {
    case MSGPACK_OBJECT_NIL:
        return Null();

    case MSGPACK_OBJECT_BOOLEAN:
        return (mo->via.boolean) ?
            True() :
            False();

    case MSGPACK_OBJECT_POSITIVE_INTEGER:
        // As per Issue #42, we need to use the base Number
        // class as opposed to the subclass Integer, since
        // only the former takes 64-bit inputs. Using the
        // Integer subclass will truncate 64-bit values.
        return Number::New(static_cast<double>(mo->via.u64));

    case MSGPACK_OBJECT_NEGATIVE_INTEGER:
        // See comment for MSGPACK_OBJECT_POSITIVE_INTEGER
        return Number::New(static_cast<double>(mo->via.i64));

    case MSGPACK_OBJECT_DOUBLE:
        return Number::New(mo->via.dec);

    case MSGPACK_OBJECT_ARRAY: {
        Local<Array> a = Array::New(mo->via.array.size);

        for (uint32_t i = 0; i < mo->via.array.size; i++) {
            a->Set(i, msgpack_to_v8(&mo->via.array.ptr[i]));
        }

        return a;
    }

    case MSGPACK_OBJECT_RAW:
        return String::New(mo->via.raw.ptr, mo->via.raw.size);

    case MSGPACK_OBJECT_MAP: {
        Local<Object> o = Object::New();

        for (uint32_t i = 0; i < mo->via.map.size; i++) {
            o->Set(
                msgpack_to_v8(&mo->via.map.ptr[i].key),
                msgpack_to_v8(&mo->via.map.ptr[i].val)
            );
        }

        return o;
    }

    default:
        throw MsgpackException("Encountered unknown MesssagePack object type");
    }
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

    MsgpackZone mz;
    msgpack_object mo;
    size_t off = 0;

    switch (msgpack_unpack(buf, len, &off, &mz._mz, &mo)) {
    case MSGPACK_UNPACK_EXTRA_BYTES:
    case MSGPACK_UNPACK_SUCCESS:
        try {
            msgpack_unpack_template->GetFunction()->Set(
                msgpack_bytes_remaining_symbol,
                Integer::New(static_cast<int32_t>(len - off))
            );
            return scope.Close(msgpack_to_v8(&mo));
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

void unpack_initialize(Handle<Object> target) {
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
