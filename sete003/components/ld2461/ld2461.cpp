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

void get_version_number_and_id(ld2461_frame_t* frame)
{
    if (frame == NULL) return; // Verifica se o ponteiro é válido

    frame->header = LD2461_FRAME_HEADER;
    frame->data_length = 2;
    frame->command_word = LD2461_COMMAND_ID_AND_VERSION;
    frame->command_value[0] = 0x01;
    frame->checksum = 0;
    frame->end = LD2461_FRAME_END;

    ld2461_generate_checksum(frame); // Gera o checksum
}

void frame_to_string(ld2461_frame_t* frame)
{
    // Print frame in hexadecimal
    printf("Header: 0x%lX\n", frame->header);
    printf("Data Length: 0x%X\n", frame->data_length);
    printf("Command Word: 0x%X\n", frame->command_word);
    printf("Command Value: ");
    for(uint16_t i=0; i < frame->data_length; i++)
    {
        printf("0x%X ", frame->command_value[i]);
    }
    printf("\n");
    printf("Checksum: 0x%X\n", frame->checksum);
    printf("End: 0x%lX\n", frame->end);
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