#ifndef _TRANSPORT_COMMON_H_
#define _TRANSPORT_COMMON_H_

/*
  For cmocka asserts.
 */
#ifndef TESTING
#include <assert.h>
#else
extern void mock_assert(const int result, const char  *const expression,
			const char * const file, const int line);
#undef assert
#define assert(expr) \
  mock_assert((int)(expr), #expr, __FILE__, __LINE__)
#endif

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
  int seqnum;
  int acknum;
  int checksum;
  char payload[20];
};

enum ack_nack
{
  ACK = 0,
  NAK = 1,
};

bool
isNAK(struct pkt *pkt)
{
  return (pkt->acknum == NAK);
}

bool isACK(struct pkt *pkt, int acknum)
{
  return (pkt->acknum == acknum);
}



static inline struct msg
extract_msg(struct pkt *pkt)
{
  struct msg output;
  memcpy(output.data, pkt->payload, sizeof(pkt->payload));
  return output;
}

int
compute_checksum(struct pkt *pkt)
{
  int checksum = pkt->seqnum + pkt->acknum;
  for (int i = 0; i < sizeof(pkt->payload) / sizeof(pkt->payload[0]); ++i)
    checksum += pkt->payload[i];
  return checksum;
}

bool
is_corrupted(struct pkt *pkt)
{
  return (compute_checksum(pkt) != pkt->checksum);
}

struct pkt
make_send_pkt(struct msg *msg, int seqnum)
{
  struct pkt output;
  memcpy(output.payload, msg->data, sizeof(msg->data));
  output.acknum = 0;
  output.seqnum = seqnum;
  output.checksum = compute_checksum(&output);
  return output;
}



enum host_state
{
  INVALID = 0,
  WAIT_FOR_ACK_NACK_0 = 1,
  WAIT_FOR_DATA_0 = 2,
  WAIT_FOR_ACK_NACK_1 = 3,
  WAIT_FOR_DATA_1 = 4
};



struct sender
{
  enum host_state state;
  struct pkt last_packet;
  int next_seq;
  double timer;
};
#define DECLARE_SENDER(name) \
  struct sender name = {			\
    .state = WAIT_FOR_DATA_0,			\
    .next_seq = 0,				\
    .timer = 0,					\
  }

#define sender_next_seq(sender) ((sender).next_seq)
#define sender_incr_seq(sender) ((sender).next_seq = ((sender).next_seq + 1) % 2)
#define sender_expected_ack(sender) (((sender).next_seq + 1)%2)
#define sender_timer(sender) ((sender).timer)
static void
init_sender(struct sender *sender, double timer)
{
  sender->state = WAIT_FOR_DATA_0;
  sender->next_seq = 0;
  sender->timer = timer;
  return;
}

#define sender_state(sender) ((sender).state)

static void
sender_change_state(struct sender *sender, enum host_state new_state)
{
  switch (sender->state)
  {
  case INVALID:
    abort();
  case WAIT_FOR_DATA_0:
    assert(new_state == WAIT_FOR_ACK_NACK_0);
    break;
  case WAIT_FOR_DATA_1:
    assert(new_state == WAIT_FOR_ACK_NACK_1);
    break;
  case WAIT_FOR_ACK_NACK_0:
    assert(new_state == WAIT_FOR_DATA_1 || new_state == WAIT_FOR_ACK_NACK_0);
    break;
  case WAIT_FOR_ACK_NACK_1:
    assert(new_state == WAIT_FOR_DATA_0|| new_state == WAIT_FOR_ACK_NACK_1);
    break;
  default:
    abort();
  }
  sender->state = new_state;  
}

struct receiver
{
  enum host_state state;
  int next_seq;
  double timer;
};

static void
init_receiver(struct receiver *receiver, double timer)
{
  receiver->state = WAIT_FOR_DATA_0;
  receiver->next_seq = 0;
  receiver->timer = timer;
  return;
}
#define DECLARE_RECEIVER(name)			\
  struct receiver name = {			\
    .state = WAIT_FOR_DATA_0,			\
    .next_seq = 0,				\
    .timer = 0,					\
}				

#define receiver_state(receiver) ((receiver).state)
#define receiver_next_seq(receiver) ((receiver).next_seq)
#define receiver_incr_seq(receiver) ((receiver).next_seq = ((receiver).next_seq + 1) % 2)
#define receiver_timeout(receiver) ((receiver).timer);
static void
receiver_change_state(struct receiver *receiver, enum host_state new_state)
{
  switch (receiver->state)
  {
  case WAIT_FOR_DATA_0:
  case WAIT_FOR_DATA_1:
    break;
  default:
    assert(0);
  }
  switch (new_state)
  {
  case WAIT_FOR_ACK_NACK_0:
  case WAIT_FOR_ACK_NACK_1:
    assert(0);
  default:
    break;
  }
  receiver->state = new_state;
}


struct pkt
make_receive_pkt(int acknum)
{
  struct pkt output = {
    .seqnum = 0,    
    .acknum = acknum,
    .checksum = 0,
    .payload = {0}
  };
  output.checksum = compute_checksum(&output);
  return output;
  
}



#endif
