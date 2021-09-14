#ifndef _DEBUGGING_GBN_H_
#define _DEBUGGING_GBN_H_
#include <stdio.h>
#include <stdlib.h>
#include "transport_common.h"

struct sender_debug
{
  FILE *sent_seqnums;
  FILE *received_acks;
  FILE *checksum_valid;
  FILE *dropped;
};

static void
init_sender_debug(struct sender_debug *dbg)
{
  if (!(dbg->sent_seqnums = fopen("./seqnums_sender.txt", "w")))
    abort();
  if (!(dbg->received_acks = fopen("./received_acks.txt", "w")))
    abort();
  if (!(dbg->checksum_valid = fopen("./checksum_valid.txt", "w")))
    abort();
  if (!(dbg->dropped = fopen("./dropped_sender.txt", "w")))
    abort();
}

struct receiver_debug
{
  FILE *received_packets;
};

static void
init_receiver_debug(struct receiver_debug *dbg)
{
  if (!(dbg->received_packets = fopen("./received_packets.txt", "w")))
    abort();
}

static void A_send_packet(struct pkt packet, struct sender_debug *dbg)
{
  fprintf(dbg->sent_seqnums, "%d\n", packet.seqnum);
  fflush(dbg->sent_seqnums);
  tolayer3(0,packet);
}

static void dump_packet_payload(struct pkt packet, FILE *fp)
{
  static char message_string[sizeof(packet.payload)+1];

  memset(message_string, 0, sizeof(message_string));
  memcpy(message_string, packet.payload, sizeof(packet.payload));
  fprintf(fp, "%s\n", message_string);
  fflush(fp);  
}

static void B_debug_packet(struct pkt packet, struct receiver_debug *rcv_dbg)
{
  static char expected = 0;  
  dump_packet_payload(packet, rcv_dbg->received_packets);
#ifdef TESTING  
  if (packet.payload[0] != (expected + 'a'))
    abort();
  else
    expected = (expected + 1) % 26;
#endif  
}

#endif
