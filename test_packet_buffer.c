#define TESTING 1
#include "packet_buffer.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

static void fill_up_queue_alphabetically(
  struct packet_buffer *pb)
{
  struct msg input_msgs[pb->qsize];
  for (int i = 0; i < ARRAY_SIZE(input_msgs); ++i)
  {
    input_msgs[i].data[0] = i + 'a';
  }

  for (int i = 0; i < ARRAY_SIZE(input_msgs) -1; ++i)
  {
    // We can over-allocate the array relative to the window size of 8.
    packet_buffer_push(pb, input_msgs[i]);
  }
  
}
void test_packet_buffer()
{
  const unsigned window_size = 8;
  const unsigned queue_size = 2*window_size;
  struct packet_buffer buffer = {
    .window_size = window_size,
    .base_ptr = 0,
    .base_seq = 1,
    .next_seq_ptr = 1,
    .next_seq = 1,
    .push_pointer = 0,
    .qsize = 0,
  };
  packet_buffer_allocate(&buffer, queue_size);
  struct msg input_msgs[queue_size];
  for (int i = 0; i < ARRAY_SIZE(input_msgs); ++i)
  {
    input_msgs[i].data[0] = i + 'a';
  }

  for (int i = 0; i < ARRAY_SIZE(input_msgs) -1; ++i)
  {
    // We can over-allocate the array relative to the window size of 8.
    packet_buffer_push(&buffer, input_msgs[i]);
  }

  assert_int_equal(packet_buffer_get_next_seqnum(&buffer), 1);
  for (int i = 0; i < window_size; ++i)
  {
    assert_int_equal(packet_buffer_get_next_seqnum(&buffer), 1 + i);    
    struct pkt *next = packet_buffer_get_next_for_window(&buffer);
    assert_ptr_not_equal(next, NULL);
    assert_int_equal(next->payload[0], 'a' + i);
  }
  struct pkt *next = packet_buffer_get_next_for_window(&buffer);
  assert_ptr_equal(next, NULL);
  assert_int_equal(packet_buffer_get_next_seqnum(&buffer), window_size+1);

  // receive a cumulative ack for packets [1,3]. Window not yet ready.
  packet_buffer_recv_ack(&buffer, 3);

  for (int i = 0; i < 3; ++i)
  {
  assert_int_equal(packet_buffer_get_next_seqnum(&buffer), window_size+1+i);
    struct pkt *next = packet_buffer_get_next_for_window(&buffer);
    assert_ptr_not_equal(next, NULL);
    printf("i: %d char: %c\n", i, next->payload[0]);    
    // We already sent packets [1,window_size], so the new packet
    // that's ready to send has this payload.
    assert_int_equal(next->payload[0], 'a' + i + window_size); 
  }
  assert_int_equal(packet_buffer_get_next_seqnum(&buffer), window_size + 4);

  /*
    Next receive another ack and wrap up the buffer some more.
     1 2 3 4 5 6 7 8 9 a b c d e f g
    |a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|
     X X X ^-base                  ^-push_pointer
     X = already sent. What is stopping us from accidentally re-sending?
     i.e. we need to ake sure we don't overlap push_pointer.

     This _should not_ be possible since A_out always gives us data
     first.
   */
  /*
    Next get an ACK for 5, and push back on seqnums 
   */
  packet_buffer_recv_ack(&buffer, 5);
  unsigned amount_free = 5; // we have an empty item at all times which is base_ptr.
  char starting_character = 'a' + queue_size;
  for (int i = 0; i < amount_free; ++i)
  {
    struct msg message;
    message.data[0] = starting_character + i;
    packet_buffer_push(&buffer, message);
  }
  struct msg message;
  message.data[0] = 'Z';
  expect_assert_failure(packet_buffer_push(&buffer, message));

}

void test_resend_iteration()
{
  const unsigned window_size = 5;
  const unsigned queue_size = 2*window_size;
  struct packet_buffer buffer = {
    .window_size = window_size,
    .base_ptr = 0,
    .base_seq = 1,
    .next_seq_ptr = 1,
    .next_seq = 1,
    .push_pointer = 0,
    .qsize = 0,
  };

  packet_buffer_allocate(&buffer, queue_size);
  fill_up_queue_alphabetically(&buffer);
  for (int i = 0; i < window_size; ++i)
    packet_buffer_get_next_for_window(&buffer);

  packet_buffer_recv_ack(&buffer, 3);
  // window was, 'abcde' acked up to c.
  char start = 'd';
  char end = 'e';
  struct pkt *iterator;
  for (iterator = packet_buffer_unacked_begin(&buffer);
       iterator != packet_buffer_unacked_end(&buffer);
       ++iterator)
  {
    assert_int_equal(iterator->payload[0], start++);
  }
  assert_int_equal((iterator-1)->payload[0], end);
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_packet_buffer),
    cmocka_unit_test(test_resend_iteration)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);  
}
