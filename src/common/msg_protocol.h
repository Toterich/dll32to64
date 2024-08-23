/**
 * A message of our serialization protocol has the following format:
 *
 *            <------===----HEADER-------->  <-------------------------------------------BODY------------------------------------------------->
 * BYTESIZE                  1          1     sizeof(StaticData::<MsgSpecific>)                    X                  Y                   Z...
 * CONTENT    PROTOCOL_VERSION      MsgId                            StaticData     [VariableArray1]   [VariableArray2]    [VariableArrayN...]
 *
 * Each message starts with a header consisting of
 * * Message Version (1 Byte),
 * * MsgId (1 Byte). See enum MsgId below.
 *
 * After that, the static portion of the message data follows as a packed struct. For outgoing calls, this is the SD_<MessageName> struct
 * that corresponds to the MsgId. For incoming responses, it is the SD_<MessageName>_Response struct. All these structs are defined in this header.
 *
 * If the message payload contains one ore more variable length arrays, the SD struct contains "VariableArray" field containing an offset
 * and a length field. With these, the array's contents can be located in the remaining message payload. The offset points to the byte offset
 * after the end of the SD struct (so an offset of 0 means the array starts immediately after the end of SD), while length determines the
 * array length in bytes.
 */


#ifndef DLL32TO64_MSG_PROTOCOL_H
#define DLL32TO64_MSG_PROTOCOL_H

#include <cstdint>

namespace msg {

/* Version number of the message protocol. */
unsigned const PROTOCOL_VERSION = 1;
/* Size of Message Header. */
unsigned const MSG_HEADER_SIZE = 2;
/* Maximum supported size of a message. */
unsigned const MSG_MAX_SIZE = 2048;
/* Maximum number of supported signals. */
unsigned const MAX_NUM_SIGNALS = 30;

// Tightly pack everything to serialize it
#pragma pack(1)

/* Structs for defining variable length array inside a message.
 *
 * When one of these is included in a message's static data (see SD_... structs), this means that the corresponding array
 * begins at the specified byte offset following the end of the static data. So if byte_offset == 0, the array begins immediately
 * after the SD_... struct ends.
 */
struct VariableArray {
    int16_t byte_length;
    int16_t byte_offset;
};

/* Defines a unique ID for each of the DLL functions and callbacks exposed by the wrapped DLL. */
// TODO: Autogenerate this
enum MsgId {
    MSGID_Invert = 0,
    MSGID_Concat = 1,
    MSGID_Callback = 2,
    MSGID_LAST = MSGID_Callback,
};

static_assert(MSGID_LAST <= 255, "MsgId does not fit into 1 byte.");

/* Direction of a message. */
enum Direction
{
    DIRECTION_Request,  // Client -> Server
    DIRECTION_Response  // Server -> Client
};

/*
 * Static Message Data included in every message.
 *
 * Depending on the MessageId, the corresponding union member is the active one.
 */
// TODO: Autogenerate this
union StaticData
{
    /*
    * Static Message Data for Callbacks that are executed by the DLL.
    *
    * Callbacks are triggered independently and parallel to the regular request-response scheme for the other messages.
    * We anyway define empty "Request" structs to allow unified parsing.
    */
    struct {} Callback;
    struct {
        int32_t val;
    } CallbackResponse;

    struct {
        bool input;
    } Invert;
    bool InvertResponse;

    struct {
        VariableArray s1;
        VariableArray s2;
    } Concat;
    struct {
        VariableArray out;
    } ConcatResponse;
};

/*
 * Represents a message to be exchanged between Client and Server.
 */
struct MessageData
{
    MsgId id;
    Direction direction;
    StaticData staticData;
    size_t variableDataLength;
    char variableData[MSG_MAX_SIZE];  // Offsets inside StaticData point into this buffer
};

/* Initialize a message. */
void InitMessageData(MessageData& message, MsgId id, Direction direction);

/* Parse the contents of buffer into message. */
bool ParseMessage(MessageData& message, Direction direction, char const *buffer, int bufferSize);

/* Serialize message into buffer. */
void SerializeMessage(MessageData const& message, char *buffer, int &messageSize);

} // end namespace

// Restore original alignment
#pragma pack()

#endif
