#include <stdio.h>
#include "ld2461.hpp"

ld2461_frame_t ld2461_setup_frame()
{
    ld2461_frame_t frame = {
        .data_length = NULL,
        .command_word = LD2461_COMMAND_NULL,
        .command_value = NULL,
        .checksum = 0,
    };
    return frame;
}

void ld2461_generate_checksum(ld2461_frame_t* frame)
{
    if(frame == NULL) return;
    int sum = 0;
    sum += frame->command_word;
    for(uint16_t i=0; i < frame->data_length-1; i++)
    {
        sum += frame->command_value[i];
    }
    (frame->checksum )= sum & 0xFF;
}

void get_version_number_and_uid(ld2461_frame_t* frame)
{
    if (frame == NULL) return;

    frame->data_length = 0x02;
    frame->command_word = LD2461_COMMAND_ID_AND_VERSION;

    frame->command_value = (uint8_t*)malloc(1);
    if(frame->command_value != (void *)0)
    {
        *(frame->command_value) = 0x09;
    }
    frame->checksum = 0;

    ld2461_generate_checksum(frame);
}

void frame_to_string(ld2461_frame_t* frame)
{
    // Print frame in hexadecimal
    printf("Data Length: 0x%2X\n", (frame->data_length));
    printf("Command Word: 0x%X\n", frame->command_word);
    printf("Command Value: ");
    for(uint16_t i=0; i < (frame->data_length); i++)
    {
        printf("0x%X ", frame->command_value[i]);
    }
    printf("\n");
    printf("Checksum: 0x%X\n", (frame->checksum));
}

void print_frame(ld2461_frame_t* frame)
{
    // Print frame in hexadecimal
    for(uint16_t i=0; i < sizeof(ld2461_frame_t); i++)
    {
        printf("%02X", ((unsigned int*)frame)[i]);
    }
    printf("\n");
}