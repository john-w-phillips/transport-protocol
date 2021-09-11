#define TESTING
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "abp.h"

void test_send_states()
{
  DECLARE_SENDER(sender);
  assert_int_equal(sender_state(sender), WAIT_FOR_DATA_0);
  sender_change_state(&sender, WAIT_FOR_ACK_NACK_0);
  assert_int_equal(sender_state(sender), WAIT_FOR_ACK_NACK_0);
  sender_change_state(&sender, WAIT_FOR_ACK_NACK_0);  
  expect_assert_failure(sender_change_state(&sender, WAIT_FOR_ACK_NACK_1));
}

void test_receive_states()
{
  DECLARE_RECEIVER(receiver);
  assert_int_equal(receiver_state(receiver), WAIT_FOR_DATA_0);
  receiver_change_state(&receiver, WAIT_FOR_DATA_1);
  receiver_change_state(&receiver, WAIT_FOR_DATA_0);
  receiver_change_state(&receiver, WAIT_FOR_DATA_1);
  expect_assert_failure(receiver_change_state(&receiver, WAIT_FOR_ACK_NACK_0));
}

void test_seqnums_receiver()
{
  DECLARE_RECEIVER(receiver);
  assert_int_equal(receiver_next_seq(receiver), 0);
  receiver_incr_seq(receiver);
  assert_int_equal(receiver_next_seq(receiver), 1);
  receiver_incr_seq(receiver);
  assert_int_equal(receiver_next_seq(receiver), 0);    
}

void test_seqnums_sender()
{
  DECLARE_RECEIVER(sender);
  assert_int_equal(sender_next_seq(sender), 0);
  sender_incr_seq(sender);
  assert_int_equal(sender_next_seq(sender), 1);
  sender_incr_seq(sender);
  assert_int_equal(sender_next_seq(sender), 0);
  assert_int_equal(sender_expected_ack(sender), 1);
}

int
main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_send_states),
    cmocka_unit_test(test_receive_states),
    cmocka_unit_test(test_seqnums_receiver),
    cmocka_unit_test(test_seqnums_sender)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
