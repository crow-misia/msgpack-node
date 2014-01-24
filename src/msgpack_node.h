#ifndef MSGPACK_NODE_H_
#define MSGPACK_NODE_H_

#include <v8.h>
#include <node.h>
#include <msgpack.h>
#include <cmath>

using namespace v8;
using namespace node;

// MSC does not support C99 trunc function.
#ifdef _MSC_BUILD
inline double trunc(double d){ return (d>0) ? floor(d) : ceil(d) ; }
#endif

#define DBG_PRINT_BUF(buf, name) \
    do { \
        char *data = Buffer::Data(buf); \
        const size_t len = Buffer::Length(buf); \
        const char *end = data + len; \
        fprintf(stderr, "Buffer %s has %lu bytes:\n", \
            (name), len \
        ); \
        while (data < end) { \
            fprintf(stderr, "  "); \
            for (int ii = 0; ii < 16 && data < end; data++) { \
                fprintf(stderr, "%s%2.2hhx", \
                    (ii > 0 && (ii % 2 == 0)) ? " " : "", data \
                ); \
            } \
            fprintf(stderr, "\n"); \
        } \
    } while (0)


// An exception class that wraps a textual message
class MsgpackException {
    public:
        MsgpackException(const char *str) :
            msg(String::New(str)) {
        }

        Handle<Value> getThrownException() {
            return Exception::TypeError(msg);
        }

    private:
        const Handle<String> msg;
};

// A holder for a msgpack_zone object; ensures destruction on scope exit
class MsgpackZone {
    public:
        msgpack_zone _mz;

        MsgpackZone(size_t sz = 1024) {
            msgpack_zone_init(&this->_mz, sz);
        }

        ~MsgpackZone() {
            msgpack_zone_destroy(&this->_mz);
        }
};

extern "C" {
    void pack_initialize(const Handle<Object> target);
    void unpack_initialize(const Handle<Object> target);
}

#endif
// vim:ts=4 sw=4 et
