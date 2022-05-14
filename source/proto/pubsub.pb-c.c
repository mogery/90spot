/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: pubsub.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "pubsub.pb-c.h"
void   subscription__init
                     (Subscription         *message)
{
  static const Subscription init_value = SUBSCRIPTION__INIT;
  *message = init_value;
}
size_t subscription__get_packed_size
                     (const Subscription *message)
{
  assert(message->base.descriptor == &subscription__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t subscription__pack
                     (const Subscription *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &subscription__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t subscription__pack_to_buffer
                     (const Subscription *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &subscription__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Subscription *
       subscription__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Subscription *)
     protobuf_c_message_unpack (&subscription__descriptor,
                                allocator, len, data);
}
void   subscription__free_unpacked
                     (Subscription *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &subscription__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor subscription__field_descriptors[3] =
{
  {
    "uri",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Subscription, uri),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "expiry",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    offsetof(Subscription, has_expiry),
    offsetof(Subscription, expiry),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "status_code",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    offsetof(Subscription, has_status_code),
    offsetof(Subscription, status_code),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned subscription__field_indices_by_name[] = {
  1,   /* field[1] = expiry */
  2,   /* field[2] = status_code */
  0,   /* field[0] = uri */
};
static const ProtobufCIntRange subscription__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor subscription__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "Subscription",
  "Subscription",
  "Subscription",
  "",
  sizeof(Subscription),
  3,
  subscription__field_descriptors,
  subscription__field_indices_by_name,
  1,  subscription__number_ranges,
  (ProtobufCMessageInit) subscription__init,
  NULL,NULL,NULL    /* reserved[123] */
};
