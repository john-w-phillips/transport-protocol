#ifndef _PACKET_BUFFER_H_
#define _PACKET_BUFFER_H_
#include "transport_common.h"
#include <stdlib.h>
#include <stdio.h>

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

struct packet_buffer
{
  unsigned window_size;
  unsigned base_ptr; /* one before the window base, pointer */
  unsigned base_seq; /* Window base #. */
  unsigned next_seq_ptr; /* Pointer to the next packet in the window */
  unsigned next_seq;  /* Next sequence # going out in a packet */
  unsigned push_pointer;
  unsigned qsize;
  struct pkt *packets;
};

static void
packet_buffer_allocate(struct packet_buffer *pb, unsigned qsize)
{
  pb->qsize = qsize;
  pb->packets = (struct pkt *)calloc(qsize, sizeof(struct pkt));
}

static struct pkt *
packet_buffer_get_next_for_window(struct packet_buffer *pb)
{
  if (pb->next_seq_ptr == (pb->base_ptr + pb->window_size + 1))
    return NULL;
  else
  {
    unsigned next_seq_old = (pb->base_ptr + pb->next_seq_ptr) % pb->qsize;
    pb->next_seq_ptr++;
    pb->packets[next_seq_old].seqnum = pb->next_seq;
    pb->next_seq++;

    return &pb->packets[next_seq_old];
  }
}

static bool
packet_buffer_push(struct packet_buffer *pb, struct msg message)
{
  unsigned next_buffer_offset = (pb->push_pointer + 1) % pb->qsize;
  //assert(next_buffer_offset != pb->base_ptr);
  if (next_buffer_offset == pb->base_ptr)
  {
    fprintf(stderr, "BUFFER OVERRUN\n");
    return false;
  }
  memcpy(pb->packets[next_buffer_offset].payload, message.data, sizeof(message.data));
  pb->push_pointer += 1;
  return true;
}

static void
packet_buffer_recv_ack(struct packet_buffer *pb, unsigned ack)
{
  printf("Old base: %d, base seq: %d, next seq ptr: %d\n",
	 pb->base_ptr,
	 pb->base_seq,
	 pb->next_seq_ptr);  
  unsigned number_of_acked_packets = (ack - pb->base_seq) + 1;
  pb->base_ptr = (number_of_acked_packets  + pb->base_ptr) % pb->qsize;
  pb->base_seq = ack + 1;
  // next_seq is the offset into the circular queue from base_seq. So it
  // needs to decrement to point to the same place it pointed to originally.
  pb->next_seq_ptr -= number_of_acked_packets;
  printf("New base: %d, base seq: %d, next seq ptr: %d\n",
	 pb->base_ptr,
	 pb->base_seq,
	 pb->next_seq_ptr);
}

static unsigned
packet_buffer_get_next_seqnum(struct packet_buffer *pb)
{
  return pb->next_seq;
}

static struct pkt *
packet_buffer_unacked_begin(struct packet_buffer *pb)
{
  return &pb->packets[(pb->base_ptr + 1) % pb->qsize];
}

static struct pkt *
packet_buffer_unacked_end(struct packet_buffer *pb)
{
  printf("END: %d\n", (pb->base_ptr + pb->next_seq_ptr) % pb->qsize);
  printf("next: %d\n", (pb->base_ptr + pb->next_seq_ptr));
  printf("next raw: %d\n", (pb->next_seq_ptr));  
  return &pb->packets[(pb->base_ptr + pb->next_seq_ptr) % pb->qsize];
}

static bool
packet_buffer_iter_is_past_window(struct packet_buffer *pb,
				  struct pkt *iter)
{
  /*
    If the iterator is equal to one-past the end (normal)
    or (edge case) we have no outstanding packets,
    and next_seq_ptr == 0.

    We could also write 
    'true if next_seq_ptr == 0 or the iterator is equal
    to base_ptr + next_seq_ptr'.
   */
  return (pb->next_seq_ptr == 0
	  || iter == &pb->packets[
	    (pb->base_ptr
	     + pb->next_seq_ptr) % pb->qsize]);
}

static struct pkt*
packet_buffer_next(struct packet_buffer *pb, struct pkt *iter)
{
  ++iter;
  if (iter > &pb->packets[pb->qsize-1])
  {
    iter = pb->packets;
  }
  return iter;
}

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x[0])))
#endif
