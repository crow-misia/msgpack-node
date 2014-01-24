#include "msgpack_node.h"

using namespace v8;
using namespace node;

extern "C" {
    static void initialize(Handle<Object> target) {
        pack_initialize(target);
        unpack_initialize(target);
    }

    NODE_MODULE(msgpackBinding, initialize)
}

// vim:ts=4 sw=4 et
