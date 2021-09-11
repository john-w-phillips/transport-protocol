#include "packet_buffer.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

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

  for (int i = 0; i < window_size; ++i)
  {
    struct pkt *next = packet_buffer_get_next_for_window(&buffer);
    assert_ptr_not_equal(next, NULL);
    assert_int_equal(next->payload[0], 'a' + i);
  }
  struct pkt *next = packet_buffer_get_next_for_window(&buffer);
  assert_ptr_equal(next, NULL);

  // receive a cumulative ack for packets [1,3]. Window not yet ready.
  packet_buffer_recv_ack(&buffer, 3);

  for (int i = 0; i < 3; ++i)
  {

    struct pkt *next = packet_buffer_get_next_for_window(&buffer);
    assert_ptr_not_equal(next, NULL);
    printf("i: %d char: %c\n", i, next->payload[0]);    
    // We already sent packets [1,window_size], so the new packet
    // that's ready to send has this payload.
    assert_int_equal(next->payload[0], 'a' + i + window_size); 
  }


}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_packet_buffer),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);  
}
