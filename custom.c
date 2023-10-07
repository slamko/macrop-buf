#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum value_type {
    UINT16 = 1,
    UINT32,
    UINT64,
    FLOAT,
    BOOL,
    FLOAT_ARRAY,
    UINT32_ARRAY,
    PROTO_ARRAY,
};

struct proto_ptr {
    struct value_pair *pb;
    size_t len;
};

struct value_pair;

typedef struct value {
    union {
        uint16_t uint16_val;
        uint32_t uint32_val;
        uint64_t uint64_val;
        float float_val;
        _Bool bool_val;

        float **float_array_val;
        uint32_t **uint32_array_val;
        struct value_pair **pb_array_val;
    };

    union {
        size_t size;
        size_t *size_ptr;

        struct {
            size_t *size_ptr;
            size_t str_size;
            size_t nmem;
        } pb;
    };

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

#define add_proto_pb_array(protoname, name, typ_proto, index) \
    void CAT3(protoname, _set_, name) (struct CAT(proto_, protoname) *proto, struct CAT(proto_, typ_proto) **value, size_t *len) { \
        proto->protobuf[index] = (struct value_pair) \
            { .val = (value_t){ .pb_array_val = (struct value_pair **)value,                \
                  .pb = { .size_ptr = len, .str_size = sizeof(struct CAT(proto_, typ_proto)), .nmem = (*value[0]).cnt }}, .type = PROTO_ARRAY } ; \
    } \
    struct CAT(proto_, typ_proto) *CAT3(protoname, _get_, name) (struct CAT(proto_, protoname) *proto) { \
        return *(struct CAT(proto_, typ_proto) **)proto->protobuf[index].val.pb_array_val; \
    } \

#define xadd_array_proto(protoname, name, pb_typ, field, typ, index) \
    void CAT3(protoname, _set_, name) (struct CAT(proto_, protoname) *proto, typ **value, size_t *len) {                   \
        proto->protobuf[index] = (struct value_pair) { .val = (value_t){ .field = value, .size_ptr = len}, .type = pb_typ } ; \
    } \
    typ *CAT3(protoname, _get_, name) (struct CAT(proto_, protoname) *proto) {             \
        return *proto->protobuf[index].val.field; \
    } \

#define add_proto_float_arr(protoname, name, index) \
    xadd_array_proto(protoname, name, FLOAT_ARRAY, float_array_val, float, index)

#define add_proto_uint32_arr(protoname, name, index) \
    xadd_array_proto(protoname, name, UINT32_ARRAY, uint32_array_val, uint32_t, index)

#define xadd_proto(protoname, name, pb_typ, typ, field, index)    \
    void CAT3(protoname, _set_, name) (struct CAT(proto_, protoname) *proto, typ value) { \
        proto->protobuf[index] = (struct value_pair) { .val = (value_t){ .field = value, .size = (sizeof(typ))}, .type = pb_typ } ; \
    } \
    typ CAT3(protoname, _get_, name) (struct CAT(proto_, protoname) *proto) {             \
        return proto->protobuf[index].val.field;  \
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
        case FLOAT_ARRAY:;
            size += *val->val.size_ptr * sizeof (float) + sizeof (size);
            break;

        case PROTO_ARRAY:;
            size += sizeof size;
            struct value_pair *pb_arr = *val->val.pb_array_val;
            struct proto_ptr pb = {0};
            pb.pb = pb_arr;
            pb.len = val->val.pb.nmem;
            
            size += get_proto_size(pb) * *val->val.size_ptr;
           
            break;

        default:
            size += val->val.size;
            break;
        }
    }

    return size;
}

void _proto_pack(struct proto_ptr pb, char *buf, size_t *sizep) {
    size_t buf_pos = 0;
    char *cur_buf = buf;
    
    for (size_t i = 0; i < pb.len; i++) {
        struct value_pair *val = &pb.pb[i];
        size_t size = 0;

        switch (val->type) {
        case FLOAT_ARRAY:;
            size = *val->val.size_ptr;
            memcpy(cur_buf, &size, sizeof size);
            size *= sizeof (float);
            memcpy(cur_buf + sizeof size, *val->val.float_array_val, size);
            size += sizeof size;

            break;

        case PROTO_ARRAY:;
            memcpy(cur_buf, val->val.pb.size_ptr, sizeof size);
            size = sizeof size;

            for (size_t i = 0; i < *val->val.size_ptr; i++) {
                struct proto_ptr pb = {0};
                char *pb_array = (char *)*val->val.pb_array_val;
                struct value_pair *pb_arr = (struct value_pair *)&pb_array[i * val->val.pb.str_size];

                pb.pb = pb_arr;
                pb.len = val->val.pb.nmem;

                _proto_pack(pb, cur_buf + size, &size);
            }
            break;

        default:;
            size = val->val.size;
            if (!size) {
                printf("pack: zero size\n");
                break;
            }

            memcpy(cur_buf, &val->val, size);
            break;
        }

        buf_pos += size;
        cur_buf = buf + buf_pos;
    }

    if (sizep) {
        *sizep += buf_pos;
    }
}

void _impl_proto_pack(struct proto_ptr pb, char **buf, size_t *size) {
    *size = get_proto_size(pb);
    *buf = calloc(*size, sizeof **buf);

    _proto_pack(pb, *buf, NULL);
}

#define proto_pack(str, buf, size) _impl_proto_pack(((struct proto_ptr) {.pb = str.protobuf, .len = str.cnt }), buf, size)

void _impl_proto_unpack(struct proto_ptr pb, char *buf, size_t *parsed_size, size_t buf_size) {
    size_t buf_pos = 0;
    char *cur_buf = buf;
    
    for (size_t i = 0; i < pb.len && buf_pos <= buf_size; i++) {
        struct value_pair *val = &pb.pb[i];
        size_t size = 0;

        switch (val->type) {
        case FLOAT_ARRAY:;
            size_t old_size = *val->val.size_ptr;
            memcpy(val->val.size_ptr, buf + buf_pos, sizeof(*val->val.size_ptr));

            size = *val->val.size_ptr * sizeof (float);
            if (*val->val.size_ptr > old_size) {
                *val->val.float_array_val = malloc(size);
            }

            memcpy(*val->val.float_array_val, buf + buf_pos + sizeof(size), size);
            size += sizeof (size);

            break;

        case PROTO_ARRAY:;
            size_t nmem = *val->val.pb.size_ptr;
            memcpy(val->val.pb.size_ptr, cur_buf, sizeof size);
            size = sizeof size;

            struct value_pair *init_str = *val->val.pb_array_val;
            if (*val->val.pb.size_ptr > nmem) {
                *val->val.pb_array_val = calloc(*val->val.pb.size_ptr, val->val.pb.str_size);
            }

            for (size_t i = 0; i < *val->val.size_ptr; i++) {
                struct proto_ptr pb = {0};
                char *pb_array = (char *)*val->val.pb_array_val;
                struct value_pair *pb_arr = (struct value_pair *)&pb_array[i * val->val.pb.str_size];

                for (size_t n = 0; n < val->val.pb.nmem; n++) {
                    pb_arr[n] = init_str[n]; 
                }

                pb.pb = pb_arr;
                pb.len = val->val.pb.nmem;

                _impl_proto_unpack(pb, cur_buf + size, &size, buf_size - buf_pos);
            }

            break;
        default:
            size = val->val.size;
            if (!size) {
                printf("unpack: Zero size\n");
                break;
            }

            memcpy(&val->val.uint32_val, buf + buf_pos, size);
            break;
        }

        buf_pos += size;
        cur_buf = buf + buf_pos;
    }

    if (parsed_size) {
        *parsed_size += buf_pos;
    }
}

#define proto_unpack(str, buf, size) _impl_proto_unpack(((struct proto_ptr) {.pb = str.protobuf, .len = str.cnt }), buf, NULL, size)

