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
    } CAT(proto_, name) = { .cnt = size };        \
    struct proto_ptr CAT(pb_, name) = { .pb = CAT(proto_, name).protobuf, .len = size };

#define xadd_proto(protoname, name, pb_typ, typ, field, index)    \
    void CAT3(protoname, _set_, name) (typ value) {                    \
        (CAT(proto_, protoname)).protobuf[index] = (struct value_pair) { .val = (value_t){ .field = value, .size = (sizeof(typ))}, .type = pb_typ } ; \
    } \
    typ CAT3(protoname, _get_, name) (void) {             \
        return (CAT(proto_, protoname)).protobuf[index].val.field;  \
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

void proto_pack(struct proto_ptr pb, char **buf, size_t *size) {
    *size = get_proto_size(pb);
    *buf = calloc(*size, sizeof **buf);

    _proto_pack(pb, *buf, size);
}

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

#define val_uint16(num) ((value_t){.size = 2, .uint16_val = num })
#define val_uint32(num) ((value_t){.size = 4, .uint32_val = num })

value_t zero_uint16(void) {
    return (value_t) { .size = 2, .uint16_val = 0 };
}

value_t zero_uint32(void) {
    return (value_t) { .size = 4, .uint32_val = 0 };
}

