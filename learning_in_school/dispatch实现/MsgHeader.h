#pragma once
#include<stdint.h>
struct MsgHeader{
    uint16_t MsgType;
    uint32_t MsgLen;
};

enum MsgType:uint16_t{
    Msg_PING = 1,
    Msg_PONG = 2,
    Msg_CHAT=3,
    UNKNOWN=999,
};