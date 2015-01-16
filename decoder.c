#include "struct.h"
#include <math.h>
#include <stdio.h>

static void ping(cc_msg_ptr in, cc_msg_ptr out) {
  out->type = MSG_PING;
}

static void request(cc_request_ptr in, cc_reply_ptr out) {
  out->status = in->status;
  out->cWnd = in->cWnd;
  out->ssThresh = in->ssThresh;
  out->recover = in->recover;

  if (in->event == CC_LOSS_EVENT) {
    out->status = CC_NORMAL;
    out->ssThresh = fmax ((double) 2 * in->segmentSize, (double) in->inflights / 2);
    out->cWnd = in->segmentSize;
  }

  if (in->event == CC_DUP_EVENT) {
    if (in->status == CC_NORMAL) {
      out->ssThresh = fmax ((double) 2 * in->segmentSize, (double) in->inflights / 2);
      out->cWnd = out->ssThresh + 3 * in->segmentSize;
      out->recover = in->highTxMark; // Fix It
      out->status = CC_FASTRECOVER;
    }

    if (in->status == CC_FASTRECOVER) {
       out->cWnd = in->cWnd + in->segmentSize;
    }
  }
  
  if (in->event == CC_ACK_EVENT) {

    if (in->status == CC_NORMAL) {
      if (in->cWnd < in->ssThresh) { 
        // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
        out->cWnd = in->cWnd + in->segmentSize; 
      } else { 
        // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
        // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
        double adder = (double) (in->segmentSize * in->segmentSize) / in->cWnd;
        adder = fmax (1.0, adder);
        out->cWnd = in->cWnd + (uint32_t) (adder); 
      }
    }

    if (in->status == CC_FASTRECOVER) {
       if (in->seq < in->recover) { 
         // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
         out->cWnd = in->cWnd + in->segmentSize - (in->seq - in->txBufferHead);
       } else if (in->seq >= in->recover) { 
         // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
         out->cWnd = fmin (in->ssThresh, in->inflights + in->segmentSize);
         out->status = CC_NORMAL;
       }
    }

  }
}

void msg_decoder(cc_msg_ptr in, cc_msg_ptr out) {

  msg_type_t in_type = in->type;

  switch (in_type) {
    case MSG_PING: 
      ping(in, out);
      break;
    case MSG_REQUEST:
      request(&in->data.request, &out->data.reply);
      out->type = MSG_REPLY;
      break;
    default:
      break;
  }

  out->hname = in->hname;
  out->hport = in->hport; 
  out->seqno = in->seqno + 1;

}

static int reply_verifier(cc_reply_ptr in, cc_reply_ptr ref) {
  if (in->cWnd != ref->cWnd) {
    printf("cWnd should be %d but is %d\n", ref->cWnd, in->cWnd);
    return -1;
  }

  if (in->ssThresh != ref->ssThresh) {
    printf("ssThresh should be %d but is %d\n", ref->ssThresh, in->ssThresh);
    return -1;
  }

  if (in->status != ref->status) {
    printf("status should be %d but is %d\n", ref->status, in->status);
    return -1;
  }
  
  return 0;
}

static int msg_verifier(cc_msg_ptr in, cc_msg_ptr ref) {
  if (in->type != ref->type) {
    printf("type should be %d but is %d\n", ref->type, in->type);
    return -1;
  }
 
  if (in->hname != ref->hname) {
    printf("hname should be %d but is %d\n", ref->hname, in->hname);
    return -1;
  }
 
  if (in->hport != ref->hport) {
    printf("hport should be %d but is %d\n", ref->hport, in->hport);
    return -1;
  }
 
  if (in->seqno != ref->seqno) {
    printf("seqno should be %ld but is %ld\n", ref->seqno, in->seqno);
    return -1;
  }

  return reply_verifier(&in->data.reply, &ref->data.reply);
}

int test_main() {
  cc_msg_t input, output, reference;
  input.hname = 643;
  input.hport = 12345;
  input.type = MSG_REQUEST;
  input.seqno = 23;
// Begin Request Data Setting
  input.data.request.status = CC_NORMAL;
  input.data.request.event = CC_ACK_EVENT;
  input.data.request.segmentSize = 1460;
  input.data.request.ssThresh = 25 * 1460;
  input.data.request.cWnd = 25 * 1460;
  msg_decoder(&input, &output);
  
  reference.hname = 643;
  reference.hport = 12345;
  reference.type = MSG_REPLY;
  reference.seqno = 24;
  reference.data.reply.status = CC_NORMAL;
  reference.data.reply.ssThresh = 25 * 1460;
  reference.data.reply.cWnd = 36558;
  if(msg_verifier(&output, &reference) == 0)
    printf("Verify OK!\n");
  return 0;
}
