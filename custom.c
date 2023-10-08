#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum value_type {
    UINT16 = 1,
    UINT32,
    UINT64,
    FLOAT,
    BOOL,
    FLOAT_ARRAY,
    UINT32_ARRAY,
    PROTO_ARRAY,
} value_type_t;

typedef enum type_flags {
    ONEOF = 1 << 16,
} type_flags_t;

struct proto_ptr {
    struct value_pair *pb;
    size_t len;
};

struct value_pair;
struct oneof;

int no_flags(int type) {
    return type & 0xFF;
}

#define err(str) fprintf(stderr, str);
#define error(str, ...) fprintf(stderr, str, __VA_ARGS__);
#define UNKNOWN_TYPE_ERR "Unknown field type, all fields should be initialized before packing or unpacking\n"

static inline void *mpb_cmalloc(size_t size) {
    void *ptr = malloc(size);                                             
    if (!ptr) { 
        err("Out of memory. Malloc failed\n");
        exit(-1); 
    } 
    return ptr;
}

static inline void *mpb_ccalloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);                                             
    if (!ptr) { 
        err("Out of memory. Calloc failed\n");
      exit(-1); 
    } 
    return ptr;
}

typedef struct value {
    union {
        uint16_t uint16_val;
        uint32_t uint32_val;
        uint64_t uint64_val;
        float float_val;
        uint8_t bool_val;

        float *float_array_val;
        uint32_t *uint32_array_val;

        struct value_pair **pb_array_val;
    };

    union {
        size_t size;

        struct {
            size_t *size_ptr;
            size_t str_size;
            size_t nmem;
        } pb;
    };

} value_t;

struct value_pair {
    value_t val;
    enum value_type type;
};

struct oneof {
    uint32_t chosen;
    struct value_pair value;
};

#define CAT(a, b) a##b
#define CAT2(a, b) CAT(a, b)
#define CAT3(a, b, c) a##b##c
#define CAT4(a, b, c, d) a##b##c##d
#define CAT5(a, b, c, d, e) a##b##c##d##e

#define def_proto(name, size)         \
    struct CAT(proto_, name) {              \
        struct value_pair protobuf[size];       \
        size_t cnt;                             \
        int packed;                             \
    };

#define create_proto(name) \
    (struct CAT(proto_, name)) { .cnt = sizeof (((struct CAT(proto_, name)) {0}).protobuf) / sizeof (*((struct CAT(proto_, name)) {0}).protobuf) }

#define get_type(protoname, name, suffix, index)                              \
    value_type_t CAT5(protoname, _get_, name, _type, suffix) (struct CAT(proto_, protoname) *proto) { return proto->protobuf[index].type; }

#define get_size(protoname, name, suffix, index)                              \
    size_t CAT5(protoname, _get_, name, _size, suffix) (struct CAT(proto_, protoname) *proto) { return proto->protobuf[index].val.size; }

#define xadd_proto_pb_array(protoname, name, typ_proto, suffix, index)   \
    void CAT4(protoname, _set_, name, suffix) (struct CAT(proto_, protoname) *proto, struct CAT(proto_, typ_proto) **value, size_t *len) { \
        proto->protobuf[index] = (struct value_pair) \
            { .val = (value_t){ .pb_array_val = (struct value_pair **)value,                \
                  .pb = { .size_ptr = len, .str_size = sizeof(struct CAT(proto_, typ_proto)), .nmem = (*value[0]).cnt }}, .type = PROTO_ARRAY } ; \
    } \
    struct CAT(proto_, typ_proto) *CAT4(protoname, _get_, name, suffix) (struct CAT(proto_, protoname) *proto) { \
        return *(struct CAT(proto_, typ_proto) **)proto->protobuf[index].val.pb_array_val; \
    } \
    get_type(protoname, name, suffix, index)

#define add_proto_pb_array(protoname, name, typ_proto, index) xadd_proto_pb_array(protoname, name, typ_proto, , index)
#define add_proto_pb_array_oneof(protoname, name, typ_proto, index) xadd_proto_pb_array(protoname, name, typ_proto, , index)

#define xadd_array_proto_suf(protoname, name, pb_typ, field, typ, suf, index) \
    void CAT4(protoname, _set_, name, suf) (struct CAT(proto_, protoname) *proto, typ *value, size_t len) { \
        proto->protobuf[index] = (struct value_pair) { .val = (value_t){ .field = value, .size = len}, .type = pb_typ } ; \
    } \
    typ *CAT4(protoname, _get_, name, suf) (struct CAT(proto_, protoname) *proto) { \
        return proto->protobuf[index].val.field; \
    } \
    get_type(protoname, name, suf, index) \
    get_size(protoname, name, suf, index)

#define xadd_array_proto(protoname, name, pb_typ, field, typ, index) xadd_array_proto_suf(protoname, name, pb_typ, field, typ, , index)
#define xadd_array_proto_oneof(protoname, name, pb_typ, field, typ, index) xadd_array_proto_suf(protoname, name, pb_typ, field, typ, , index)

#define add_proto_float_arr(protoname, name, index) \
    xadd_array_proto(protoname, name, FLOAT_ARRAY, float_array_val, float, index)

#define add_proto_uint32_arr(protoname, name, index) \
    xadd_array_proto(protoname, name, UINT32_ARRAY, uint32_array_val, uint32_t, index)

#define add_proto_float_arr_oneof(protoname, name, index) \
    xadd_array_proto_oneof(protoname, name, FLOAT_ARRAY | ONEOF, float_array_val, float, index)

#define add_proto_uint32_arr_oneof(protoname, name, index) \
    xadd_array_proto_oneof(protoname, name, UINT32_ARRAY | ONEOF, uint32_array_val, uint32_t, index)

#define xadd_proto_suf(protoname, name, pb_typ, typ, field, suffix, index)   \
    void CAT4(protoname, _set_, name, suffix) (struct CAT(proto_, protoname) *proto, typ value) { \
        proto->protobuf[index] = (struct value_pair) { .val = (value_t){ .field = value, .size = (sizeof(typ))}, .type = pb_typ } ; \
    } \
    typ CAT4(protoname, _get_, name, suffix) (struct CAT(proto_, protoname) *proto) { \
        return proto->protobuf[index].val.field;  \
    } \
    get_type(protoname, name, suffix, index)

#define xadd_proto(protoname, name, pb_typ, typ, field, index) xadd_proto_suf(protoname, name, pb_typ, typ, field, , index)

#define add_proto_bool(protoname, name, index)                         \
    xadd_proto(protoname, name, BOOL, uint8_t, bool_val, index)

#define add_proto_uint16(protoname, name, index)                         \
    xadd_proto(protoname, name, UINT16, uint16_t, uint16_val, index)

#define add_proto_uint32(protoname, name, index)                         \
    xadd_proto(protoname, name, UINT32, uint32_t, uint32_val, index)

#define add_proto_float(protoname, name, index)                         \
    xadd_proto(protoname, name, FLOAT, float, float_val, index)

#define add_proto_float_oneof(protoname, name, index) xadd_proto_suf(protoname, name, FLOAT | ONEOF, float, float_val, , index)

#define add_proto_uint32_oneof(protoname, name, index) xadd_proto_suf(protoname, name, UINT32 | ONEOF, uint32_t, uint32_val, , index)

int get_proto_size(struct proto_ptr pb, size_t *sizep) {
    size_t size = 0;
    
    for (size_t i = 0; i < pb.len; i++) {
        struct value_pair *val = &pb.pb[i];

        if (val->type & ONEOF) {
            size += sizeof (val->type);
        }

        switch (no_flags(val->type)) {
        case FLOAT_ARRAY:;
            size += val->val.size * sizeof (float) + sizeof (size);
            break;
        case UINT32_ARRAY:;
            size += val->val.size * sizeof (uint32_t) + sizeof (size);
            break;

        case PROTO_ARRAY:;
            size += sizeof size;
            struct value_pair *pb_arr = *val->val.pb_array_val;
            struct proto_ptr pb = {0};
            pb.pb = pb_arr;
            pb.len = val->val.pb.nmem;
            
            size_t sub_size = 0;
            int ret = get_proto_size(pb, &sub_size);
            if (ret) {
                return ret;
            }
            
            size += sub_size * *val->val.pb.size_ptr;
           
            break;

        default:
            if (val->type > 0 && !(val->type >> 16)) {
                size += val->val.size;
            } else {
                err(UNKNOWN_TYPE_ERR);
                return 1;
            }

            break;
        }
    }

    *sizep += size;
    return 0;
}

int _proto_pack(struct proto_ptr pb, char *buf, size_t *sizep) {
    size_t buf_pos = 0;
    char *cur_buf = buf;
    
    for (size_t i = 0; i < pb.len; i++) {
        struct value_pair *val = &pb.pb[i];
        size_t size = 0;

        if (val->type & ONEOF) {
            memcpy(cur_buf, &val->type, sizeof val->type);
            buf_pos += sizeof val->type;
            cur_buf += sizeof val->type;
        }

        switch (no_flags(val->type)) {
            /*
        case UINT32:;
            uint32_t v = val->val.uint32_val;
            if (v < (1 << 7)) {
                size = 1;
                cur_buf[0] = v & 0xFF;
            } else if (v < (1 << 14)) {
                size = 2;
                cur_buf[0] = v & 0x7F | 0x80;
                cur_buf[1] = (v << 1) & (0xFF << 8);
            } else if (v < (1 << 21)) {
                size = 3;
                cur_buf[0] = v & 0x7F | 0x80;
                cur_buf[1] = v & 0x7F | 0x80;
                cur_buf[2] = (v << 1) & (0xFF << 8);
            }

            for (size_t j = 5; j >= 0; j--) {
                cur_buf[j] = (v >> (7 * j)); 
            }

            break;
            */

        case UINT32_ARRAY:;
            size = val->val.size;
            memcpy(cur_buf, &size, sizeof size);
            size *= sizeof (uint32_t);
            memcpy(cur_buf + sizeof size, val->val.uint32_array_val, size);
            size += sizeof size;

            break;

        case FLOAT_ARRAY:;
            size = val->val.size;
            memcpy(cur_buf, &size, sizeof size);
            size *= sizeof (float);
            memcpy(cur_buf + sizeof size, val->val.float_array_val, size);
            size += sizeof size;

            break;

        case PROTO_ARRAY:;
            memcpy(cur_buf, val->val.pb.size_ptr, sizeof size);
            size = sizeof size;

            for (size_t i = 0; i < *val->val.pb.size_ptr; i++) {
                struct proto_ptr pb = {0};
                char *pb_array = (char *)*val->val.pb_array_val;
                struct value_pair *pb_arr = (struct value_pair *)&pb_array[i * val->val.pb.str_size];

                pb.pb = pb_arr;
                pb.len = val->val.pb.nmem;

                _proto_pack(pb, cur_buf + size, &size);
            }
            break;

        default:;
            if (val->type < 0 || val->type >> 16) {
                err(UNKNOWN_TYPE_ERR);
                return 1;
            }
            
            size = val->val.size;
            if (!size) {
                printf("pack: zero size\n");
                break;
            }

            memcpy(cur_buf, &val->val, size);
            break;
        }

        buf_pos += size;
        cur_buf += size;
    }

    if (sizep) {
        *sizep += buf_pos;
    }

    return 0;
}

int _impl_proto_pack(struct proto_ptr pb, char **buf, size_t *size) {
    *size = 0;
    int ret = get_proto_size(pb, size);

    if (ret) {
        return ret;
    }
    
    *buf = mpb_ccalloc(*size, sizeof **buf);

    return _proto_pack(pb, *buf, NULL);
}

#define proto_pack(str, buf, size) _impl_proto_pack(((struct proto_ptr) {.pb = str.protobuf, .len = str.cnt }), buf, size)

void _impl_proto_unpack(struct proto_ptr pb, char *buf, size_t *parsed_size, size_t buf_size) {
    size_t buf_pos = 0;
    char *cur_buf = buf;
    
    for (size_t i = 0; i < pb.len && buf_pos <= buf_size; i++) {
        struct value_pair *val = &pb.pb[i];
        size_t size = 0;
        size_t old_size = 0;
        value_type_t type = val->type;

        if (val->type & ONEOF) {
            memcpy(&type, cur_buf, sizeof type);
            cur_buf += sizeof type;
            buf_pos += sizeof type;
            val->type = type;
        }

        switch (no_flags(type)) {
        case UINT32_ARRAY:;
            old_size = val->val.size;
            memcpy(&val->val.size, buf + buf_pos, sizeof(val->val.size));

            size = val->val.size * sizeof (uint32_t);
            val->val.uint32_array_val = mpb_cmalloc(size);

            memcpy(val->val.uint32_array_val, buf + buf_pos + sizeof(size), size);
            size += sizeof (size);

            break;

        case FLOAT_ARRAY:;
            old_size = val->val.size;
            memcpy(&val->val.size, buf + buf_pos, sizeof(val->val.size));

            size = val->val.size * sizeof (float);
            val->val.float_array_val = mpb_cmalloc(size);

            memcpy(val->val.float_array_val, buf + buf_pos + sizeof(size), size);
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

            for (size_t i = 0; i < *val->val.pb.size_ptr; i++) {
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

        case BOOL:;
            size = sizeof (uint8_t);
            memcpy(&val->val.bool_val, buf + buf_pos, size);
            break;
        case FLOAT:;
            size = sizeof (float);
            memcpy(&val->val.float_val, buf + buf_pos, size);
            break;
        case UINT16:;
            size = sizeof (uint16_t);
            memcpy(&val->val.uint16_val, buf + buf_pos, size);
            break;
        case UINT32:;
            size = sizeof (uint32_t);
            memcpy(&val->val.uint32_val, buf + buf_pos, size);
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
        cur_buf += size;
    }

    if (parsed_size) {
        *parsed_size += buf_pos;
    }
}

#define proto_unpack(str, buf, size) \
    { _impl_proto_unpack(((struct proto_ptr) {.pb = str.protobuf, .len = str.cnt }), buf, NULL, size); \
        str.packed = 1; }

void _proto_free(struct proto_ptr pb) {
    for (size_t i = 0; i < pb.len; i++) {
        struct value_pair *val = &pb.pb[i];
        size_t size = 0;
        size_t old_size = 0;
        value_type_t type = val->type;
        
        switch (no_flags(type)) {
        case UINT32_ARRAY:;
            free(val->val.uint32_array_val);
            val->val.uint32_array_val = NULL;
            break;

        case FLOAT_ARRAY:;
            free(val->val.float_array_val);
            val->val.float_array_val = NULL;
            break;

        case PROTO_ARRAY:;
            free(val->val.pb_array_val);
            val->val.pb_array_val = NULL;
            break;
        }
    }
}

#define proto_free(str) \
    { if (str.packed) { _proto_free(((struct proto_ptr) {.pb = str.protobuf, .len = str.cnt })); str.packed = 0; } }

