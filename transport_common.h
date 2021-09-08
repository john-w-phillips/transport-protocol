#ifndef _TRANSPORT_COMMON_H_
#define _TRANSPORT_COMMON_H_
#include <assert.h>
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

bool isACK(struct pkt *pkt)
{
  return (pkt->acknum == ACK);
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
make_send_pkt(struct msg *msg, int checksum)
{
  struct pkt output;
  memcpy(output.payload, msg->data, sizeof(msg->data));
  output.checksum = checksum;
  output.acknum = 0;
  return output;
}

struct pkt
make_receive_pkt(enum ack_nack isack)
{
  struct pkt output = {.acknum = isack };
  return output;
}

enum sender_state
{
  INVALID = 0,
  WAIT_FOR_ACK_NACK = 1,
  WAIT_FOR_DATA = 2,
};

#define QSIZE 256
struct sender
{
  enum sender_state state;
  struct pkt last_packet;
};
#define DECLARE_SENDER(name) \
  struct sender name = {.state = WAIT_FOR_DATA}

static void
init_sender(struct sender *sender)
{
  sender->state = WAIT_FOR_DATA;
}

#define sender_state(sender) ((sender).state)

static void
sender_change_state(struct sender *sender, enum sender_state new_state)
{
  assert(new_state == WAIT_FOR_DATA || new_state == WAIT_FOR_ACK_NACK);
  if (new_state == WAIT_FOR_DATA)
    assert(sender->state != WAIT_FOR_DATA);
  sender->state = new_state;
}

#endif
