#include "msg_protocol.h"

#include <plog/Log.h>

#include <cstdio>
#include <cstring>
#include <cassert>

namespace msg {

#define SIZEOF_CASE_REQUEST(MSG_NAME) \
    case MSGID_##MSG_NAME: return sizeof(StaticData::MSG_NAME)

#define SIZEOF_CASE_RESPONSE(MSG_NAME) \
    case MSGID_##MSG_NAME: return sizeof(StaticData::MSG_NAME##Response)

// TODO: AUTOGEN
static int SizeOfStaticData(MsgId msgId, Direction direction)
{
    if (direction == DIRECTION_Request)
    {
        switch (msgId)
        {
            SIZEOF_CASE_REQUEST(Callback);
            SIZEOF_CASE_REQUEST(Invert);
            SIZEOF_CASE_REQUEST(Concat);
            SIZEOF_CASE_REQUEST(SetCallback);
        }
    }
    else if (direction == DIRECTION_Response)
    {
        switch (msgId)
        {
            SIZEOF_CASE_RESPONSE(Callback);
            SIZEOF_CASE_RESPONSE(Invert);
            SIZEOF_CASE_RESPONSE(Concat);
            SIZEOF_CASE_RESPONSE(SetCallback);
        }
    }

    assert(false);
    return -1;
}

void InitMessageData(MessageData& message, MsgId id, Direction direction)
{
    message.id = id;
    message.direction = direction;
    message.variableDataLength = 0;
    std::memset(&message.staticData, 0, sizeof(message.staticData));
    std::memset(&message.variableData, 0, sizeof(message.variableData));
}

bool ParseMessage(MessageData& message, Direction direction, char const *buffer, int bufferSize)
{
    // Incomplete Message
    if (bufferSize < 2)
    {
        PLOG_ERROR << "ParseMessage(): Message is incomplete";
        return false;
    }

    int const msgVersion = buffer[0];
    if (msgVersion != PROTOCOL_VERSION)
    {
        PLOG_ERROR << "ParseMessage() Protocol version mismatch, Client: " << msgVersion << ", Server: " << PROTOCOL_VERSION;
        return false;
    }

    MsgId const id = (MsgId)(buffer[1]);

    static_assert(MSG_HEADER_SIZE == 2);

    int const sdSize = SizeOfStaticData(id, direction);

    // All the rest of the buffer is variable Data
    int const vdSize = bufferSize - (MSG_HEADER_SIZE + sdSize);
    std::memcpy(&message.staticData, &buffer[MSG_HEADER_SIZE], sdSize);
    std::memcpy(&message.variableData, &buffer[MSG_HEADER_SIZE + sdSize], vdSize);
    message.id = id;
    message.direction = direction;
    message.variableDataLength = vdSize;

    return true;
}

void SerializeMessage(MessageData const& message, char *buffer, int &messageSize)
{
    buffer[0] = PROTOCOL_VERSION;
    buffer[1] = message.id;

    static_assert(MSG_HEADER_SIZE == 2);

    int const sdSize = SizeOfStaticData(message.id, message.direction);

    std::memcpy(&buffer[MSG_HEADER_SIZE], &message.staticData, sdSize);
    std::memcpy(&buffer[MSG_HEADER_SIZE + sdSize], message.variableData, message.variableDataLength);

    messageSize = MSG_HEADER_SIZE + sdSize + message.variableDataLength;
}

} // end namespace
