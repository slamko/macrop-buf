#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include "custom.c"

def_proto(new, 4);
add_proto_uint16(new, first, 0);
add_proto_uint32(new, second, 1);
add_proto_float(new, third, 2);
add_proto_uint32_arr(new, arr, 5, 3);

def_proto(parsed, 3);

add_proto_uint16(parsed, first, 0);
add_proto_uint32(parsed, second, 1);
add_proto_float(parsed, third, 2)

int main() {
    struct proto_new new = create_proto(new);
    uint32_t arr [] = { 7, 8, 9 };
    
    new_set_first(new, 4);
    new_set_second(new, 32);
    new_set_third(new, 0.67);
    new_set_arr(arr);

    char *buf = NULL;
    size_t buf_size = 0;

    proto_pack(new, &buf, &buf_size);

    FILE *f = fopen("new.txt", "w");
    fwrite(buf, 1, buf_size, f);

    struct proto_parsed parsed = create_proto(parsed);
    parsed_set_first(parsed, 0);
    parsed_set_second(parsed, 0);
    parsed_set_third(parsed, 0.0);

    proto_unpack(&pb_parsed, buf, buf_size);

    printf("Parsed first: %u\n", parsed_get_first(parsed));
    printf("Parsed second: %u\n", parsed_get_second(parsed));
    printf("Parsed third: %f\n", parsed_get_third(parsed));
    /*
    */

}
