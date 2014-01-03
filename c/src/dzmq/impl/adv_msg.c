#include <assert.h>
#include <string.h>

#include "adv_msg.h"

size_t serialize_adv_msg(uint8_t * buffer, dzmq_adv_msg_t * adv_msg)
{
    size_t bytes_written = serialize_msg_header(buffer, &adv_msg->header);
    uint16_t addr_len = strlen(adv_msg->addr);
    memcpy(buffer + bytes_written, &addr_len, sizeof(addr_len));
    bytes_written += sizeof(addr_len);
    memcpy(buffer + bytes_written, adv_msg->addr, addr_len);
    bytes_written += addr_len;
    return bytes_written;
}

size_t deserialize_adv_msg(dzmq_adv_msg_t * adv_msg, uint8_t * buffer, size_t len)
{
    size_t msg_len = 0, available_bytes = len;
    uint16_t addr_len;
    assert (available_bytes > sizeof(addr_len));
    memcpy(&addr_len, buffer, sizeof(addr_len));
    msg_len += sizeof(addr_len);
    available_bytes -= sizeof(addr_len);
    assert (available_bytes >= addr_len);
    memcpy(&adv_msg->addr, buffer + msg_len, addr_len);
    adv_msg->addr[addr_len] = 0;
    msg_len += addr_len;
    return msg_len;
}
