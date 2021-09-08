#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "transport_common.h"

void test_states()
{
  DECLARE_SENDER(sender);
  assert_int_equal(sender_state(sender), WAIT_FOR_DATA);
  sender_change_state(&sender, WAIT_FOR_ACK_NACK);
  assert_int_equal(sender_state(sender), WAIT_FOR_ACK_NACK);
}

int
main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_states)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
