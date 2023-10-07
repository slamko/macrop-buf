#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include "custom.c"

def_proto(arg, 2)
add_proto_uint16(arg, first, 0);
add_proto_uint32(arg, second, 1);

def_proto(new, 4);
add_proto_float(new, third, 0);
add_proto_float_arr(new, arr, 1);
add_proto_pb_array(new, pb_arr, arg, 2);

def_proto(parsed, 4);
add_proto_float(parsed, third, 0)
add_proto_float_arr(parsed, arr, 1);
add_proto_pb_array(parsed, pb_arr, arg, 2);

add_proto_float_arr_oneof(parsed, only_arr, 3);
add_proto_float_oneof(parsed, only_float, 3);
add_proto_uint32_oneof(parsed, only_int, 3);

add_proto_float_arr_oneof(new, only_arr, 3);
add_proto_float_oneof(new, only_float, 3);
add_proto_uint32_oneof(new, only_int, 3);
/*
*/

int main() {
    struct proto_new new = create_proto(new);
    float *arr = malloc(sizeof *arr * 3);
    arr[0] = 87.0;
    arr[1] = 67.0;
    arr[2] = 57.0;

    size_t arr_len = 3;
    
    new_set_third(&new, 0.67);
    new_set_arr(&new, &arr, &arr_len);
    new_set_only_float_float(&new, 0.587);
    
    size_t arg_arr_len = 3;
    struct proto_arg *arg_ptr = malloc(sizeof *arg_ptr * arg_arr_len);

    arg_ptr[0] = create_proto(arg);
    arg_set_first(&arg_ptr[0], 0x56);
    arg_set_second(&arg_ptr[0], 0x43);

    arg_ptr[1] = create_proto(arg);
    arg_set_first (&arg_ptr[1], 0x256);
    arg_set_second(&arg_ptr[1], 0x243);

    arg_ptr[2] = create_proto(arg);
    arg_set_first (&arg_ptr[2], 0x456);
    arg_set_second(&arg_ptr[2], 0x443);

    new_set_pb_arr(&new, &arg_ptr, &arg_arr_len);

    char *buf = NULL;
    size_t buf_size = 0;

    proto_pack(new, &buf, &buf_size);

    FILE *f = fopen("new.txt", "w+");
    fwrite(buf, 1, buf_size, f);

    float *parr = NULL;
    size_t par_size = 0;
 
    struct proto_arg arg_parsed;
    size_t arg_parsed_len = 1;

    arg_parsed = create_proto(arg);
    arg_set_first(&arg_parsed, 0);
    arg_set_second(&arg_parsed, 0);
    struct proto_arg *arg_parsed_ptr = &arg_parsed;

    struct proto_parsed parsed = create_proto(parsed);
    parsed_set_third(&parsed, 0.0);
    parsed_set_arr(&parsed, &parr, &par_size);
    parsed_set_pb_arr(&parsed, &arg_parsed_ptr, &arg_parsed_len);
    parsed_set_only_int_uint32(&parsed, 0);

    proto_unpack(parsed, buf, buf_size);

    printf("Parsed third: %f\n", parsed_get_third(&parsed));

    for (size_t i = 0 ; i < par_size; i++) {
        printf("Parsed arr: %zu:%f\n", i, parsed_get_arr(&parsed)[i]);
    }

    struct proto_arg *parg = parsed_get_pb_arr(&parsed);
    for (size_t i = 0 ; i < arg_parsed_len; i++) {
        printf("Parsed arg: 1 = %u : 2 = %u\n", arg_get_first(parg + i), arg_get_second(parg + i));
    } 

    value_type_t onlyt = parsed_get_only_float_type_float(&parsed);
    switch (onlyt & ~ONEOF) {
    case UINT32:
        printf("Only int: %u\n", parsed_get_only_int_uint32(&parsed));
    case FLOAT:
        printf("Only float: %f\n", parsed_get_only_float_float(&parsed));
    }

    free(arr);
    free(parr);
    free(buf);
    free(parg);
    free(arg_ptr);
    fclose(f);
    /*
    */

}
