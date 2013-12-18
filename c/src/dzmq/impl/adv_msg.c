#include <assert.h>
#include <string.h>

#include "adv_msg.h"

size_t serialize_adv_msg(uint8_t * buffer, dzmq_adv_msg_t * adv_msg)
{
    size_t bytes_written = serialize_msg_header(buffer, &adv_msg->header);
    uint8_t addr_len = strlen(adv_msg->addr);
    memcpy(buffer + bytes_written, &addr_len, 1);
    bytes_written += 1;
    memcpy(buffer + bytes_written, adv_msg->addr, addr_len);
    bytes_written += addr_len;
    return bytes_written;
}

size_t deserialize_adv_msg(dzmq_adv_msg_t * adv_msg, uint8_t * buffer, size_t len)
{
    size_t msg_len = 0, available_bytes = len;
    uint8_t addr_len;
    assert (available_bytes > 1);
    memcpy(&addr_len, buffer, 1);
    msg_len += 1;
    available_bytes -= 1;
    assert (available_bytes >= addr_len);
    memcpy(&adv_msg->addr, buffer + msg_len, addr_len);
    adv_msg->addr[addr_len] = 0;
    msg_len += addr_len;
    return msg_len;
}
