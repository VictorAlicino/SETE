#include <stdio.h>
#include "ld2461.hpp"

void ld2461_generate_checksum(ld2461_frame_t* frame)
{
    if(frame == NULL) return;
    int sum = 0;
    sum += frame->command_word;
    for(uint16_t i=0; i < frame->data_length; i++)
    {
        sum += frame->command_value[i];
    }
    frame->checksum = sum & 0xFF;
}

char* get_version_number_and_id()
{
    char data[20];

    ld2461_frame frame = {
        .header = LD2461_FRAME_HEADER,
        .data_length = 2,
        .command_word = LD2461_COMMAND_ID_AND_VERSION,
        .command_value = 0x01,
        .end = LD2461_FRAME_END
    };
    ld2461_generate_checksum(&frame);

    // Send via UART to LD2461

}
