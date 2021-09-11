#ifndef _DEBUGGING_GBN_H_
#define _DEBUGGING_GBN_H_
#include <stdio.h>
#include <stdlib.h>

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

#endif
