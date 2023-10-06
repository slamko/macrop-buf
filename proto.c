#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include "custom.c"

def_proto(new, 3);
add_proto_uint16(new, first, 0);
add_proto_uint32(new, second, 1);
add_proto_float(new, third, 2)

def_proto(parsed, 3);

add_proto_uint16(parsed, first, 0);
add_proto_uint32(parsed, second, 1);
add_proto_float(parsed, third, 2)

int main() {
    new_set_first(4);
    new_set_second(32);
    new_set_third(0.67);

    char *buf = NULL;
    size_t buf_size = 0;

    proto_pack(pb_new, &buf, &buf_size);

    FILE *f = fopen("new.txt", "w");
    fwrite(buf, 1, buf_size, f);

    parsed_set_first(0);
    parsed_set_second(0);
    parsed_set_third(0.0);

    proto_unpack(&pb_parsed, buf, buf_size);

    printf("Parsed first: %u\n", parsed_get_first());
    printf("Parsed second: %u\n", parsed_get_second());
    printf("Parsed third: %f\n", parsed_get_third());
    /*
    */

}
