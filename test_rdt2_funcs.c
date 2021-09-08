#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "transport_common.h"

void test_is_nack()
{
  struct pkt my_pkt;
  my_pkt.acknum = NAK;
  assert_true(isNAK(&my_pkt));
  my_pkt.acknum = ACK;
  assert_false(isNAK(&my_pkt));
}

void test_make_pkt_sender()
{
  struct msg my_msg  = {.data = "abcdefghij"};
  int checksum = 30;
  struct pkt output_pkt = make_send_pkt(&my_msg, checksum);
  assert_int_equal(output_pkt.checksum, checksum);
  assert_string_equal(output_pkt.payload, my_msg.data);
}

int
main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_is_nack),
    cmocka_unit_test(test_make_pkt_sender)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
