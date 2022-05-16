/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: playlist4issues.proto */

#ifndef PROTOBUF_C_playlist4issues_2eproto__INCLUDED
#define PROTOBUF_C_playlist4issues_2eproto__INCLUDED

#include "protobuf-c.h"

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1004000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct ClientIssue ClientIssue;
typedef struct ClientResolveAction ClientResolveAction;


/* --- enums --- */

typedef enum _ClientIssue__Level {
  CLIENT_ISSUE__LEVEL__LEVEL_UNKNOWN = 0,
  CLIENT_ISSUE__LEVEL__LEVEL_DEBUG = 1,
  CLIENT_ISSUE__LEVEL__LEVEL_INFO = 2,
  CLIENT_ISSUE__LEVEL__LEVEL_NOTICE = 3,
  CLIENT_ISSUE__LEVEL__LEVEL_WARNING = 4,
  CLIENT_ISSUE__LEVEL__LEVEL_ERROR = 5
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CLIENT_ISSUE__LEVEL)
} ClientIssue__Level;
typedef enum _ClientIssue__Code {
  CLIENT_ISSUE__CODE__CODE_UNKNOWN = 0,
  CLIENT_ISSUE__CODE__CODE_INDEX_OUT_OF_BOUNDS = 1,
  CLIENT_ISSUE__CODE__CODE_VERSION_MISMATCH = 2,
  CLIENT_ISSUE__CODE__CODE_CACHED_CHANGE = 3,
  CLIENT_ISSUE__CODE__CODE_OFFLINE_CHANGE = 4,
  CLIENT_ISSUE__CODE__CODE_CONCURRENT_CHANGE = 5
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CLIENT_ISSUE__CODE)
} ClientIssue__Code;
typedef enum _ClientResolveAction__Code {
  CLIENT_RESOLVE_ACTION__CODE__CODE_UNKNOWN = 0,
  CLIENT_RESOLVE_ACTION__CODE__CODE_NO_ACTION = 1,
  CLIENT_RESOLVE_ACTION__CODE__CODE_RETRY = 2,
  CLIENT_RESOLVE_ACTION__CODE__CODE_RELOAD = 3,
  CLIENT_RESOLVE_ACTION__CODE__CODE_DISCARD_LOCAL_CHANGES = 4,
  CLIENT_RESOLVE_ACTION__CODE__CODE_SEND_DUMP = 5,
  CLIENT_RESOLVE_ACTION__CODE__CODE_DISPLAY_ERROR_MESSAGE = 6
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CLIENT_RESOLVE_ACTION__CODE)
} ClientResolveAction__Code;
typedef enum _ClientResolveAction__Initiator {
  CLIENT_RESOLVE_ACTION__INITIATOR__INITIATOR_UNKNOWN = 0,
  CLIENT_RESOLVE_ACTION__INITIATOR__INITIATOR_SERVER = 1,
  CLIENT_RESOLVE_ACTION__INITIATOR__INITIATOR_CLIENT = 2
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CLIENT_RESOLVE_ACTION__INITIATOR)
} ClientResolveAction__Initiator;

/* --- messages --- */

struct  ClientIssue
{
  ProtobufCMessage base;
  protobuf_c_boolean has_level;
  ClientIssue__Level level;
  protobuf_c_boolean has_code;
  ClientIssue__Code code;
  protobuf_c_boolean has_repeatcount;
  int32_t repeatcount;
};
#define CLIENT_ISSUE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&client_issue__descriptor) \
    , 0, CLIENT_ISSUE__LEVEL__LEVEL_UNKNOWN, 0, CLIENT_ISSUE__CODE__CODE_UNKNOWN, 0, 0 }


struct  ClientResolveAction
{
  ProtobufCMessage base;
  protobuf_c_boolean has_code;
  ClientResolveAction__Code code;
  protobuf_c_boolean has_initiator;
  ClientResolveAction__Initiator initiator;
};
#define CLIENT_RESOLVE_ACTION__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&client_resolve_action__descriptor) \
    , 0, CLIENT_RESOLVE_ACTION__CODE__CODE_UNKNOWN, 0, CLIENT_RESOLVE_ACTION__INITIATOR__INITIATOR_UNKNOWN }


/* ClientIssue methods */
void   client_issue__init
                     (ClientIssue         *message);
size_t client_issue__get_packed_size
                     (const ClientIssue   *message);
size_t client_issue__pack
                     (const ClientIssue   *message,
                      uint8_t             *out);
size_t client_issue__pack_to_buffer
                     (const ClientIssue   *message,
                      ProtobufCBuffer     *buffer);
ClientIssue *
       client_issue__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   client_issue__free_unpacked
                     (ClientIssue *message,
                      ProtobufCAllocator *allocator);
/* ClientResolveAction methods */
void   client_resolve_action__init
                     (ClientResolveAction         *message);
size_t client_resolve_action__get_packed_size
                     (const ClientResolveAction   *message);
size_t client_resolve_action__pack
                     (const ClientResolveAction   *message,
                      uint8_t             *out);
size_t client_resolve_action__pack_to_buffer
                     (const ClientResolveAction   *message,
                      ProtobufCBuffer     *buffer);
ClientResolveAction *
       client_resolve_action__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   client_resolve_action__free_unpacked
                     (ClientResolveAction *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*ClientIssue_Closure)
                 (const ClientIssue *message,
                  void *closure_data);
typedef void (*ClientResolveAction_Closure)
                 (const ClientResolveAction *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor client_issue__descriptor;
extern const ProtobufCEnumDescriptor    client_issue__level__descriptor;
extern const ProtobufCEnumDescriptor    client_issue__code__descriptor;
extern const ProtobufCMessageDescriptor client_resolve_action__descriptor;
extern const ProtobufCEnumDescriptor    client_resolve_action__code__descriptor;
extern const ProtobufCEnumDescriptor    client_resolve_action__initiator__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_playlist4issues_2eproto__INCLUDED */