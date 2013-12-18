#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "header.h"
#include "guid.h"

size_t serialize_msg_header(uint8_t * buffer, dzmq_msg_header_t * header)
{
    size_t index = 0;
    memcpy(buffer, &header->version, 2);
    index += 2;
    memcpy(buffer + index, &header->guid, GUID_LEN);
    index += GUID_LEN;
    uint8_t topic_length = strlen(header->topic);
    memcpy(buffer + index, &topic_length, 1);
    index += 1;
    memcpy(buffer + index, header->topic, topic_length);
    index += topic_length;
    memcpy(buffer + index, &header->type, 1);
    index += 1;
    memcpy(buffer + index, header->flags, 16);
    index += 16;
    return index;
}

size_t deserialize_msg_header(dzmq_msg_header_t * header, uint8_t * buffer, size_t len)
{
    size_t header_length = 0, available_bytes = len;
    assert (available_bytes > 2);
    memcpy(&header->version, buffer, 2);
    header_length += 2;
    available_bytes -= 2;
    assert (available_bytes > GUID_LEN);
    memcpy(&header->guid, buffer + header_length, GUID_LEN);
    header_length += GUID_LEN;
    available_bytes -= GUID_LEN;
    assert (available_bytes > 1);
    uint8_t topic_len;
    memcpy(&topic_len, buffer + header_length, 1);
    header_length += 1;
    available_bytes -= 1;
    assert (available_bytes > topic_len);
    memcpy(&header->topic, buffer + header_length, topic_len);
    header_length += topic_len;
    available_bytes -= topic_len;
    assert (available_bytes > 1);
    memcpy(&header->type, buffer + header_length, 1);
    header_length += 1;
    available_bytes -= 1;
    assert (available_bytes >= 16);
    memcpy(&header->flags, buffer + header_length, 16);
    header_length += 16;
    return header_length;
}

void dzmq_print_header(dzmq_msg_header_t * header)
{
    char guid_str[GUID_STR_LEN];
    dzmq_guid_to_str(header->guid, guid_str, GUID_STR_LEN);
    printf("---------------------------------\n");
    printf("header.version: 0x%02x\n", header->version);
    printf("header.guid:    %s\n", guid_str);
    printf("header.topic:   %s\n", header->topic);
    printf("header.type:    0x%02x\n", header->type);
    printf("---------------------------------\n");
}
