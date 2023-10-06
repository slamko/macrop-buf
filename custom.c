#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PB_UINT16 1
#define PB_UINT32 2

enum value_type {
    UINT16 = 1,
    UINT32,
    UINT64,
    FLOAT,
    BOOL,
    ARRAY,
    PROTO_ARRAY,
};

struct proto_ptr {
    struct value_pair *pb;
    size_t len;
};

typedef struct value {
    union {
        uint16_t uint16_val;
        uint32_t uint32_val;
        uint64_t uint64_val;
        float float_val;
        _Bool bool_val;

        struct value *array_val;
        struct proto_ptr *pb_array_val;
    };

    size_t size;
} value_t;

struct value_pair {
    const char *name;
    value_t val;
    enum value_type type;
};

#define CAT(a, b) a##b
#define CAT3(a, b, c) a##b##c

#define def_proto(name, size)         \
    struct CAT(proto_, name) {              \
        struct value_pair protobuf[size];       \
        size_t cnt;                             \
    };

#define create_proto(name) \
    (struct CAT(proto_, name)) { .cnt = sizeof (((struct CAT(proto_, name)) {0}).protobuf) / sizeof (*((struct CAT(proto_, name)) {0}).protobuf) }

#define xadd_array_proto(protoname, name, pb_typ, typ, index) \
    void CAT3(protoname, _set_, name) (typ *value, size_t len) {                   \
        (CAT(proto_, protoname)).protobuf[index] = (struct value_pair) { .val = (value_t){ .array_val = (value_t *)value, .size = len}, .type = pb_typ } ; \
    } \
    typ *CAT3(protoname, _get_, name) (void) {             \
        return (typ *)(CAT(proto_, protoname)).protobuf[index].val.array_val; \
    } \

#define add_proto_uint32_arr(protoname, name, len, index) \
    xadd_array_proto(protoname, name, ARRAY, uint32_t, len, index)

#define xadd_proto(protoname, name, pb_typ, typ, field, index)    \
    void CAT3(protoname, _set_, name) (struct CAT(proto_, protoname) proto, typ value) { \
        proto.protobuf[index] = (struct value_pair) { .val = (value_t){ .field = value, .size = (sizeof(typ))}, .type = pb_typ } ; \
    } \
    typ CAT3(protoname, _get_, name) (struct CAT(proto_, protoname) proto) {             \
        return proto.protobuf[index].val.field;  \
    } \

#define add_proto_uint16(protoname, name, index)                         \
    xadd_proto(protoname, name, UINT16, uint16_t, uint16_val, index)

#define add_proto_uint32(protoname, name, index)                         \
    xadd_proto(protoname, name, UINT32, uint32_t, uint32_val, index)

#define add_proto_float(protoname, name, index)                         \
    xadd_proto(protoname, name, FLOAT, float, float_val, index)

size_t get_proto_size(struct proto_ptr pb) {
    size_t size = 0;
    
    for (size_t i = 0; i < pb.len; i++) {
         struct value_pair *val = &pb.pb[i];

        switch (val->type) {
        case PROTO_ARRAY:;
            for (size_t i = 0; i < val->val.size; i++) {
                size += get_proto_size(val->val.pb_array_val[i]);
            }

            break;
        default:
            size += val->val.size;
            break;
        }
    }

    return size;
}

void _proto_pack(struct proto_ptr pb, char *buf, size_t *pb_size) {
    size_t buf_pos = 0;
    
    for (size_t i = 0; i < pb.len; i++) {
        struct value_pair *val = &pb.pb[i];
        size_t size = val->val.size;

        if (!size) {
            printf("Error: zero size\n");
            return;
        }

        switch (val->type) {
        case PROTO_ARRAY:;
            for (size_t i = 0; i < val->val.size; i++) {
                _proto_pack(val->val.pb_array_val[i], buf, pb_size);
            }
            break;
        default:
            memcpy(buf + buf_pos, &val->val, size);
            break;
        }

        buf_pos += size;
    }

    *pb_size += buf_pos;
}

void _impl_proto_pack(struct proto_ptr pb, char **buf, size_t *size) {
    *size = get_proto_size(pb);
    *buf = calloc(*size, sizeof **buf);

    _proto_pack(pb, *buf, size);
}

#define proto_pack(str, buf, size) _impl_proto_pack(((struct proto_ptr) {.pb = str.protobuf, .len = str.cnt }), buf, size)

void proto_unpack(struct proto_ptr *pb, char *buf, size_t size) {
    size_t buf_pos = 0;
    
    for (size_t i = 0; i < pb->len && buf_pos <= size; i++) {
        struct value_pair *val = &pb->pb[i];
        size_t size = val->val.size;

        if (!size) {
            printf("unpack: Zero size\n");
        }

        switch (val->type) {
        case PROTO_ARRAY:;
            for (size_t i = 0; i < val->val.size; i++) {
                proto_unpack(&val->val.pb_array_val[i], buf, get_proto_size(val->val.pb_array_val[i]));
            }

            break;
        default:
            memcpy(&val->val.uint32_val, buf + buf_pos, size);
            break;
        }

        buf_pos += size;
    }
}



