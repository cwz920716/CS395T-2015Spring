#ifndef _STRUCT_H
#define _STRUCT_H

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define msg_type_t             uint8_t
#define host_name_t            uint32_t 
#define host_port_t            uint16_t 
#define msg_id_t               uint32_t
#define cc_event_t             uint8_t
#define cc_status_t            uint8_t

#define MSG_PING               0
#define MSG_REQUEST            1
#define MSG_REPLY              2

#define CC_ACK_EVENT           0
#define CC_DUP_EVENT           1
#define CC_LOSS_EVENT          2

#define CC_NORMAL              0 
#define CC_FASTRECOVER         1

struct __attribute__((__packed__)) cc_request {
  cc_status_t status;
  cc_event_t event;
  uint32_t segmentSize;
  uint32_t cWnd;
  uint32_t ssThresh;
  uint32_t recover;
  uint32_t inflights; // in bytes
  uint32_t highTxMark; // Last Frame Sent
  uint32_t txBufferHead; // Highest Unacked Frame
  uint32_t seq;
};

typedef struct cc_request cc_request_t;
typedef struct cc_request *cc_request_ptr;

struct __attribute__((__packed__)) cc_reply {
  uint32_t cWnd;
  uint32_t ssThresh;
  cc_status_t status;
  uint32_t recover;
};

typedef struct cc_reply cc_reply_t;
typedef struct cc_reply *cc_reply_ptr;

struct __attribute__((__packed__)) cc_msg {
  msg_type_t type;
  host_name_t hname;
  host_port_t hport;
  msg_id_t seqno;
  union {
    struct cc_request request;
    struct cc_reply reply;
  } data;
};

typedef struct cc_msg cc_msg_t;
typedef struct cc_msg *cc_msg_ptr;

void msg_decoder(cc_msg_ptr in, cc_msg_ptr out);

#endif
