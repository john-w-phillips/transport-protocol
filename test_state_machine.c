#define TESTING
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "transport_common.h"

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

int
main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_send_states),
    cmocka_unit_test(test_receive_states)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
