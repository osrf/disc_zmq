#include <stdlib.h>
#include <stdio.h>

#include "guid.h"

void dzmq_guid_to_str(uuid_t guid, char * guid_str, int guid_str_len)
{
    for (size_t i = 0; i < sizeof(uuid_t) && i != guid_str_len; i ++)
    {
         snprintf(guid_str, guid_str_len,
                  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  guid[0], guid[1], guid[2], guid[3], guid[4], guid[5], guid[6],
                  guid[7], guid[8], guid[9], guid[10], guid[11], guid[12],
                  guid[13], guid[14], guid[15]
        );
    }
}

int dzmq_guid_compare(uuid_t guid, uuid_t other)
{
    for (size_t i = 0; i < sizeof(uuid_t); i ++)
    {
        if (guid[i] != other[i])
        {
            return 0;
        }
    }
    return 1;
}
