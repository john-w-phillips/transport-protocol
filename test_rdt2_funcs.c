#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "transport_common.h"

void test_is_nack()
{
  struct pkt my_pkt;
  my_pkt.acknum = 0;
  assert_true(isACK(&my_pkt, 0));
}

void test_make_pkt_sender()
{
  struct msg my_msg  = {.data = "abcdefghij"};
  struct pkt output_pkt = make_send_pkt(&my_msg, 2);
  assert_int_equal(output_pkt.checksum, compute_checksum(&output_pkt));
  assert_int_equal(output_pkt.seqnum, 2);
  assert_string_equal(output_pkt.payload, my_msg.data);
}

void test_make_pkt_receiver()
{
  struct pkt output_pkt = make_receive_pkt(0);
  assert_true(isACK(&output_pkt, 0));
  assert_false(isACK(&output_pkt, 1));
  assert_int_equal(output_pkt.acknum, 0);
  output_pkt = make_receive_pkt(1);
  assert_int_equal(output_pkt.acknum, 1);  
  assert_true(isACK(&output_pkt, 1));

}

void test_corruption()
{
  struct pkt my_pkt = {
    .acknum = 0,
    .seqnum = 0,
    .payload = "abcdefgjih"
  };

  my_pkt.checksum = compute_checksum(&my_pkt);
  assert_false(is_corrupted(&my_pkt));
  my_pkt.payload[0] = 'z';
  assert_true(is_corrupted(&my_pkt));
  my_pkt.payload[0] = 'a';
  assert_false(is_corrupted(&my_pkt));

  my_pkt.acknum = 1;
  assert_true(is_corrupted(&my_pkt));
}

int
main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_is_nack),
    cmocka_unit_test(test_make_pkt_sender),
    cmocka_unit_test(test_make_pkt_receiver),
    cmocka_unit_test(test_corruption)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
