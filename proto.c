#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include "custom.c"

def_proto(arg, 2)
add_proto_uint16(arg, first, 0);
add_proto_uint32(arg, second, 1);

def_proto(new, 3);
add_proto_float(new, third, 0);
add_proto_float_arr(new, arr, 1);
add_proto_pb_array(new, pb_arr, arg, 2);

def_proto(parsed, 4);
add_proto_uint16(parsed, first, 0);
add_proto_uint32(parsed, second, 1);
add_proto_float(parsed, third, 2)
add_proto_float_arr(parsed, arr, 3);

int main() {
    struct proto_new new = create_proto(new);
    float *arr = malloc(sizeof *arr * 3);
    arr[0] = 87.0;
    arr[1] = 67.0;
    arr[2] = 57.0;

    size_t arr_len = 3;
    
    new_set_third(&new, 0.67);
    new_set_arr(&new, &arr, &arr_len);
    
    struct proto_arg *arg_ptr = malloc(sizeof *arg_ptr * 2);
    size_t arg_arr_len = 2;

    arg_ptr[0] = create_proto(arg);
    arg_set_first(&arg_ptr[0], 56);
    arg_set_second(&arg_ptr[0], 43);

    arg_ptr[1] = create_proto(arg);
    arg_set_first (&arg_ptr[1], 256);
    arg_set_second(&arg_ptr[1], 243);

    new_set_pb_arr(&new, &arg_ptr, &arg_arr_len);

    char *buf = NULL;
    size_t buf_size = 0;

    proto_pack(new, &buf, &buf_size);

    FILE *f = fopen("new.txt", "w+");
    fwrite(buf, 1, buf_size, f);

    /*
    float *parr = NULL;
    size_t par_size = 0;

    struct proto_parsed parsed = create_proto(parsed);
    parsed_set_first(&parsed, 0);
    parsed_set_second(&parsed, 0);
    parsed_set_third(&parsed, 0.0);
    parsed_set_arr(&parsed, &parr, &par_size);

    proto_unpack(parsed, buf, buf_size);

    printf("Parsed first: %u\n", parsed_get_first(&parsed));
    printf("Parsed second: %u\n", parsed_get_second(&parsed));
    printf("Parsed third: %f\n", parsed_get_third(&parsed));

    for (size_t i = 0 ; i < par_size; i++) {
        printf("Parsed arr: %zu:%f\n", i, parsed_get_arr(&parsed)[i]);
    }

    free(arr);
    free(parr);
    free(buf);
    */

}
