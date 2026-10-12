#include <stdlib.h>
#include <stdio.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include <string.h>


void setUp(void) {
  printf("Setting up tests...\n");
}

void tearDown(void) {
  printf("Tearing down tests...\n");
}

void test_get_greeting(void) {
  char *greeting = get_greeting("Alice");
  TEST_ASSERT_NOT_NULL(greeting);
  TEST_ASSERT_EQUAL_STRING("Hello, Alice!", greeting);
  free(greeting); // Free the allocated memory for the greeting

  greeting = get_greeting(NULL);
  TEST_ASSERT_NULL(greeting);

  greeting = get_greeting("");
  TEST_ASSERT_NOT_NULL(greeting);
  TEST_ASSERT_EQUAL_STRING("Hello, !", greeting);
  free(greeting);
}

void test_packet_checksum(void)
{
    const uint8_t example[] = {
        0x00, 0x01, 0xf2, 0x03,
        0xf4, 0xf5, 0xf6, 0xf7
    };

    TEST_ASSERT_EQUAL_HEX16(
        0x220d,
        packet_checksum(example, sizeof example)
    );

    const uint8_t odd[] = {0x48, 0x69, 0x21};

    TEST_ASSERT_EQUAL_HEX16(
        0x9696,
        packet_checksum(odd, sizeof odd)
    );

    uint8_t checked[] = {
        0x22, 0x0d,
        0x00, 0x01, 0xf2, 0x03,
        0xf4, 0xf5, 0xf6, 0xf7
    };

    TEST_ASSERT_EQUAL_HEX16(
        0,
        packet_checksum(checked, sizeof checked)
    );

    checked[3] ^= 0x01;

    TEST_ASSERT_NOT_EQUAL(
        0,
        packet_checksum(checked, sizeof checked)
    );
}

void test_packet_encode(void)
{
    Packet packet = {0};
    uint8_t buffer[MAX_PACKET_SIZE];

    packet.type = DATA;
    packet.seq = 2;
    packet.length = 3;
    packet.payload[0] = 'H';
    packet.payload[1] = 'i';
    packet.payload[2] = '!';

    const uint8_t expected[] = {
        0x00, 0x00, 0x96, 0x91,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x03, 0x48, 0x69, 0x21
    };

    size_t size = packet_encode(&packet, buffer, sizeof buffer);

    TEST_ASSERT_EQUAL_UINT(13, size);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof expected);
    TEST_ASSERT_EQUAL_HEX16(0, packet_checksum(buffer, size));

    /* A buffer that is too small must be rejected. */
    TEST_ASSERT_EQUAL_UINT(0, packet_encode(&packet, buffer, 12));

    packet.type = 3;
    TEST_ASSERT_EQUAL_UINT(
        0, packet_encode(&packet, buffer, sizeof buffer)
    );

    packet.type = DATA;
    packet.length = MAX_PAYLOAD + 1;
    TEST_ASSERT_EQUAL_UINT(
        0, packet_encode(&packet, buffer, sizeof buffer)
    );

    packet.type = ACK;
    packet.length = 1;
    TEST_ASSERT_EQUAL_UINT(
        0, packet_encode(&packet, buffer, sizeof buffer)
    );

    packet.length = 0;
    packet.seq = 3;

    const uint8_t expected_ack[] = {
        0x01, 0x00, 0xfe, 0xfc,
        0x00, 0x00, 0x00, 0x03,
        0x00, 0x00
    };

    size = packet_encode(&packet, buffer, sizeof buffer);

    TEST_ASSERT_EQUAL_UINT(10, size);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        expected_ack, buffer, sizeof expected_ack
    );

    packet.type = FIN;
    size = packet_encode(&packet, buffer, sizeof buffer);

    TEST_ASSERT_EQUAL_UINT(10, size);
    TEST_ASSERT_EQUAL_UINT8(FIN, buffer[0]);
    TEST_ASSERT_EQUAL_HEX16(0, packet_checksum(buffer, size));
}

void test_packet_decode(void)
{
    const uint8_t data[] = {
        0x00, 0x00, 0x96, 0x91,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x03, 0x48, 0x69, 0x21
    };

    Packet packet = {0};

    TEST_ASSERT_TRUE(packet_decode(data, sizeof data, &packet));
    TEST_ASSERT_EQUAL_UINT8(DATA, packet.type);
    TEST_ASSERT_EQUAL_UINT32(2, packet.seq);
    TEST_ASSERT_EQUAL_UINT16(3, packet.length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data + HEADER_SIZE, packet.payload, 3);

    const uint8_t ack[] = {
        0x01, 0x00, 0xfe, 0xfc,
        0x00, 0x00, 0x00, 0x03,
        0x00, 0x00
    };

    TEST_ASSERT_TRUE(packet_decode(ack, sizeof ack, &packet));
    TEST_ASSERT_EQUAL_UINT8(ACK, packet.type);
    TEST_ASSERT_EQUAL_UINT32(3, packet.seq);
    TEST_ASSERT_EQUAL_UINT16(0, packet.length);

    Packet original = {0};
    uint8_t buffer[MAX_PACKET_SIZE];

    original.type = FIN;
    original.seq = 7;

    size_t size = packet_encode(&original, buffer, sizeof buffer);

    TEST_ASSERT_TRUE(packet_decode(buffer, size, &packet));
    TEST_ASSERT_EQUAL_UINT8(FIN, packet.type);
    TEST_ASSERT_EQUAL_UINT32(7, packet.seq);
    TEST_ASSERT_EQUAL_UINT16(0, packet.length);

    /* Check the largest allowed DATA packet and a multi-byte sequence. */
    original.type = DATA;
    original.seq = 0x12345678;
    original.length = MAX_PAYLOAD;

    for (size_t i = 0; i < MAX_PAYLOAD; i++) {
        original.payload[i] = (uint8_t)(i % 256);
    }

    size = packet_encode(&original, buffer, sizeof buffer);

    TEST_ASSERT_EQUAL_UINT(MAX_PACKET_SIZE, size);
    TEST_ASSERT_TRUE(packet_decode(buffer, size, &packet));
    TEST_ASSERT_EQUAL_UINT32(original.seq, packet.seq);
    TEST_ASSERT_EQUAL_UINT16(MAX_PAYLOAD, packet.length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        original.payload, packet.payload, MAX_PAYLOAD
    );
}

void test_packet_decode_invalid(void)
{
    const uint8_t good[] = {
        0x00, 0x00, 0x96, 0x91,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x03, 0x48, 0x69, 0x21
    };

    uint8_t buffer[sizeof good];
    Packet packet = {0};

    TEST_ASSERT_FALSE(packet_decode(good, 0, &packet));
    TEST_ASSERT_FALSE(packet_decode(good, 9, &packet));

    /* The header says three payload bytes, but one is missing. */
    TEST_ASSERT_FALSE(
        packet_decode(good, sizeof good - 1, &packet)
    );

    for (size_t i = 0; i < sizeof good; i++) {
        buffer[i] = good[i];
    }

    /* Length 1025 exceeds the payload limit. */
    buffer[8] = 0x04;
    buffer[9] = 0x01;
    TEST_ASSERT_FALSE(packet_decode(buffer, sizeof buffer, &packet));

    buffer[8] = good[8];
    buffer[9] = good[9];

    buffer[0] = 3;
    TEST_ASSERT_FALSE(packet_decode(buffer, sizeof buffer, &packet));
    buffer[0] = DATA;

    buffer[1] = 1;
    TEST_ASSERT_FALSE(packet_decode(buffer, sizeof buffer, &packet));
    buffer[1] = 0;

    /* ACK and FIN cannot carry payloads. */
    buffer[0] = ACK;
    TEST_ASSERT_FALSE(packet_decode(buffer, sizeof buffer, &packet));

    buffer[0] = FIN;
    TEST_ASSERT_FALSE(packet_decode(buffer, sizeof buffer, &packet));
    buffer[0] = DATA;

    /* Flip one payload bit without updating the checksum. */
    buffer[10] ^= 0x01;
    TEST_ASSERT_FALSE(packet_decode(buffer, sizeof buffer, &packet));
}

void test_receiver_data(void)
{
    Receiver receiver;
    receiver_init(&receiver, 0);

    TEST_ASSERT_EQUAL_UINT32(0, receiver.expected);
    TEST_ASSERT_FALSE(receiver.finished);
    TEST_ASSERT_EQUAL_INT64(30000, receiver.deadline);

    Packet packet = {0};
    packet.type = DATA;
    packet.seq = 0;
    packet.length = 2;
    packet.payload[0] = 'H';
    packet.payload[1] = 'i';

    ReceiveAction action = receiver_step(&receiver, &packet, 100);

    TEST_ASSERT_TRUE(action.deliver);
    TEST_ASSERT_TRUE(action.send_ack);
    TEST_ASSERT_FALSE(action.close_file);
    TEST_ASSERT_EQUAL_UINT(2, action.length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(packet.payload, action.payload, 2);
    TEST_ASSERT_EQUAL_UINT8(ACK, action.ack.type);
    TEST_ASSERT_EQUAL_UINT16(0, action.ack.length);
    TEST_ASSERT_EQUAL_UINT32(1, action.ack.seq);
    TEST_ASSERT_EQUAL_UINT32(1, receiver.expected);
    TEST_ASSERT_EQUAL_INT64(30100, receiver.deadline);

    /* The same DATA packet must not be delivered twice. */
    action = receiver_step(&receiver, &packet, 200);

    TEST_ASSERT_FALSE(action.deliver);
    TEST_ASSERT_TRUE(action.send_ack);
    TEST_ASSERT_EQUAL_UINT32(1, action.ack.seq);

    /* Packet 2 arrives before the missing packet 1. */
    packet.seq = 2;
    action = receiver_step(&receiver, &packet, 300);

    TEST_ASSERT_FALSE(action.deliver);
    TEST_ASSERT_TRUE(action.send_ack);
    TEST_ASSERT_EQUAL_UINT32(1, action.ack.seq);

    packet.seq = 1;
    action = receiver_step(&receiver, &packet, 400);

    TEST_ASSERT_TRUE(action.deliver);
    TEST_ASSERT_EQUAL_UINT32(2, action.ack.seq);
}

void test_receiver_fin(void)
{
    Receiver receiver;
    receiver_init(&receiver, 0);

    Packet packet = {0};
    packet.type = FIN;

    /* A premature FIN cannot finish the transfer. */
    packet.seq = 1;
    ReceiveAction action = receiver_step(&receiver, &packet, 50);

    TEST_ASSERT_FALSE(action.close_file);
    TEST_ASSERT_FALSE(action.send_ack);
    TEST_ASSERT_FALSE(receiver.finished);

    /* An empty file starts with FIN 0. */
    packet.seq = 0;
    action = receiver_step(&receiver, &packet, 100);

    TEST_ASSERT_TRUE(action.close_file);
    TEST_ASSERT_TRUE(action.send_ack);
    TEST_ASSERT_FALSE(action.deliver);
    TEST_ASSERT_TRUE(receiver.finished);
    TEST_ASSERT_EQUAL_UINT32(1, action.ack.seq);
    TEST_ASSERT_EQUAL_INT64(2100, receiver.deadline);

    /* Repeat the FIN as if its first ACK was lost. */
    action = receiver_step(&receiver, &packet, 350);

    TEST_ASSERT_TRUE(action.send_ack);
    TEST_ASSERT_FALSE(action.close_file);
    TEST_ASSERT_EQUAL_UINT32(1, action.ack.seq);
    TEST_ASSERT_EQUAL_INT64(2100, receiver.deadline);

    packet.seq = 1;
    action = receiver_step(&receiver, &packet, 400);
    TEST_ASSERT_FALSE(action.send_ack);

    packet.type = DATA;
    packet.seq = 0;
    action = receiver_step(&receiver, &packet, 450);

    TEST_ASSERT_FALSE(action.deliver);
    TEST_ASSERT_FALSE(action.send_ack);

    TEST_ASSERT_EQUAL_INT(0, receiver_status(&receiver, 2099));
    TEST_ASSERT_EQUAL_INT(1, receiver_status(&receiver, 2100));
}

void test_receiver_idle(void)
{
    Receiver receiver;
    receiver_init(&receiver, 100);

    TEST_ASSERT_EQUAL_INT(0, receiver_status(&receiver, 30099));
    TEST_ASSERT_EQUAL_INT(2, receiver_status(&receiver, 30100));

    /* Wrong-direction ACKs do not produce replies or reset idle time. */
    Packet packet = {0};
    packet.type = ACK;

    ReceiveAction action = receiver_step(&receiver, &packet, 200);

    TEST_ASSERT_FALSE(action.send_ack);
    TEST_ASSERT_FALSE(action.deliver);
    TEST_ASSERT_FALSE(action.close_file);
    TEST_ASSERT_EQUAL_INT64(30100, receiver.deadline);
}

void test_sender_window(void)
{
    uint8_t bytes[6 * MAX_PAYLOAD] = {0};
    Sender sender;

    sender_init(&sender, bytes, sizeof bytes, 4, 250);

    TEST_ASSERT_EQUAL_UINT32(6, sender.data_count);
    TEST_ASSERT_FALSE(sender_done(&sender));

    SendAction action = sender_step(&sender, NULL, 0);

    TEST_ASSERT_EQUAL_UINT(4, action.count);
    TEST_ASSERT_EQUAL_UINT32(0, sender.base);
    TEST_ASSERT_EQUAL_UINT32(4, sender.next);
    TEST_ASSERT_TRUE(sender.timer_running);
    TEST_ASSERT_EQUAL_INT64(250, sender.deadline);

    for (size_t i = 0; i < action.count; i++) {
        TEST_ASSERT_EQUAL_UINT8(DATA, action.packets[i].type);
        TEST_ASSERT_EQUAL_UINT32(i, action.packets[i].seq);
        TEST_ASSERT_EQUAL_UINT16(MAX_PAYLOAD, action.packets[i].length);
    }

    /* A full window cannot send more new packets. */
    action = sender_step(&sender, NULL, 100);
    TEST_ASSERT_EQUAL_UINT(0, action.count);

    Packet ack = {0};
    ack.type = ACK;
    ack.seq = 3;

    /* ACK 3 acknowledges packets 0, 1, and 2 together. */
    action = sender_step(&sender, &ack, 150);

    TEST_ASSERT_EQUAL_UINT32(3, sender.base);
    TEST_ASSERT_EQUAL_UINT32(6, sender.next);
    TEST_ASSERT_EQUAL_UINT(2, action.count);
    TEST_ASSERT_EQUAL_UINT32(4, action.packets[0].seq);
    TEST_ASSERT_EQUAL_UINT32(5, action.packets[1].seq);
    TEST_ASSERT_EQUAL_INT64(400, sender.deadline);

    /* A duplicate ACK must not change the timer. */
    action = sender_step(&sender, &ack, 200);

    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_EQUAL_UINT32(3, sender.base);
    TEST_ASSERT_EQUAL_INT64(400, sender.deadline);

    /* Ignore an ACK for a packet that has never been sent. */
    ack.seq = 20;
    action = sender_step(&sender, &ack, 225);

    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_EQUAL_UINT32(3, sender.base);

    /* The timeout resends the whole outstanding window. */
    action = sender_step(&sender, NULL, 400);

    TEST_ASSERT_EQUAL_UINT(3, action.count);
    TEST_ASSERT_EQUAL_UINT32(3, action.packets[0].seq);
    TEST_ASSERT_EQUAL_UINT32(4, action.packets[1].seq);
    TEST_ASSERT_EQUAL_UINT32(5, action.packets[2].seq);
    TEST_ASSERT_EQUAL_UINT(1, sender.timeouts);
    TEST_ASSERT_EQUAL_INT64(650, sender.deadline);

    ack.seq = 6;
    action = sender_step(&sender, &ack, 450);

    TEST_ASSERT_EQUAL_UINT(0, sender.timeouts);
    TEST_ASSERT_EQUAL_UINT(1, action.count);
    TEST_ASSERT_EQUAL_UINT8(FIN, action.packets[0].type);
    TEST_ASSERT_EQUAL_UINT32(6, action.packets[0].seq);
    TEST_ASSERT_FALSE(sender_done(&sender));

    ack.seq = 7;
    action = sender_step(&sender, &ack, 500);

    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_TRUE(sender_done(&sender));
    TEST_ASSERT_FALSE(sender.timer_running);

    action = sender_step(&sender, NULL, 800);
    TEST_ASSERT_EQUAL_UINT(0, action.count);
}

void test_sender_timeout_limit(void)
{
    const uint8_t bytes[] = {'x'};
    Sender sender;

    sender_init(&sender, bytes, sizeof bytes, 1, 250);

    SendAction action = sender_step(&sender, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(1, action.count);

    for (unsigned i = 1; i < 10; i++) {
        action = sender_step(&sender, NULL, (int64_t)i * 250);

        TEST_ASSERT_EQUAL_UINT(1, action.count);
        TEST_ASSERT_EQUAL_UINT32(0, action.packets[0].seq);
        TEST_ASSERT_EQUAL_UINT8('x', action.packets[0].payload[0]);
        TEST_ASSERT_FALSE(sender.failed);
    }

    action = sender_step(&sender, NULL, 2500);

    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_EQUAL_UINT(10, sender.timeouts);
    TEST_ASSERT_TRUE(sender.failed);
    TEST_ASSERT_FALSE(sender_done(&sender));

    action = sender_step(&sender, NULL, 2750);
    TEST_ASSERT_EQUAL_UINT(0, action.count);
}

void test_sender_empty_file(void)
{
    Sender sender;
    sender_init(&sender, NULL, 0, 1, 250);

    SendAction action = sender_step(&sender, NULL, 0);

    TEST_ASSERT_EQUAL_UINT32(0, sender.data_count);
    TEST_ASSERT_EQUAL_UINT(1, action.count);
    TEST_ASSERT_EQUAL_UINT8(FIN, action.packets[0].type);
    TEST_ASSERT_EQUAL_UINT32(0, action.packets[0].seq);
    TEST_ASSERT_EQUAL_UINT16(0, action.packets[0].length);

    /* A lost final ACK causes FIN to be retransmitted. */
    action = sender_step(&sender, NULL, 250);

    TEST_ASSERT_EQUAL_UINT(1, action.count);
    TEST_ASSERT_EQUAL_UINT8(FIN, action.packets[0].type);

    Packet ack = {0};
    ack.type = ACK;
    ack.seq = 1;

    action = sender_step(&sender, &ack, 300);

    TEST_ASSERT_TRUE(sender_done(&sender));
    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_EQUAL_UINT(0, sender.timeouts);
}

void test_sender_invalid_ack(void)
{
    const uint8_t bytes[] = {'x'};
    Sender sender;

    sender_init(&sender, bytes, sizeof bytes, 1, 250);

    SendAction action = sender_step(&sender, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(1, action.count);

    /* A DATA packet cannot acknowledge our transmission. */
    Packet packet = {0};
    packet.type = DATA;
    packet.seq = 1;

    action = sender_step(&sender, &packet, 100);

    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_EQUAL_UINT32(0, sender.base);
    TEST_ASSERT_EQUAL_UINT32(1, sender.next);
    TEST_ASSERT_EQUAL_INT64(250, sender.deadline);

    /* An ACK must have no payload. */
    packet.type = ACK;
    packet.length = 1;

    action = sender_step(&sender, &packet, 150);

    TEST_ASSERT_EQUAL_UINT(0, action.count);
    TEST_ASSERT_EQUAL_UINT32(0, sender.base);
    TEST_ASSERT_EQUAL_INT64(250, sender.deadline);
    TEST_ASSERT_FALSE(sender_done(&sender));
}

/* A fixed seed makes the simulated damage repeatable. */
static uint32_t random_number(uint32_t *seed)
{
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;

    return *seed;
}

/* Return zero, one, or two copies of a decoded packet.
 * Damage follows the relay's order: loss, corruption, duplication.
 */
static unsigned channel_packet(const Packet *packet, Packet *received,
                               uint32_t *seed, unsigned damage[3])
{
    uint8_t buffer[MAX_PACKET_SIZE];
    size_t size = packet_encode(packet, buffer, sizeof buffer);

    TEST_ASSERT_TRUE(size > 0);

    if (random_number(seed) % 100 < 20) {
        damage[0]++;
        return 0;
    }

    if (random_number(seed) % 100 < 20) {
        size_t position = random_number(seed) % size;
        unsigned bit = random_number(seed) % 8;

        buffer[position] ^= (uint8_t)(1u << bit);
        damage[1]++;

        TEST_ASSERT_FALSE(packet_decode(buffer, size, received));
        return 0;
    }

    TEST_ASSERT_TRUE(packet_decode(buffer, size, received));

    if (random_number(seed) % 100 < 20) {
        damage[2]++;
        return 2;
    }

    return 1;
}

static void simulate_transfer(size_t length, unsigned window,
                              uint32_t seed)
{
    /* Large enough to reuse the sender's saved-packet slots. */
    uint8_t input[70 * MAX_PAYLOAD + 17];
    uint8_t output[sizeof input];
    size_t delivered = 0;

    for (size_t i = 0; i < length; i++) {
        input[i] = (uint8_t)(i % 251);
    }

    Sender sender;
    Receiver receiver;

    sender_init(&sender, input, length, window, 250);
    receiver_init(&receiver, 0);

    unsigned forward_damage[3] = {0};
    unsigned reverse_damage[3] = {0};

    int64_t now = 0;
    SendAction pending = sender_step(&sender, NULL, now);

    while (!sender_done(&sender) && now < 1000000) {
        SendAction following = {0};

        for (size_t i = 0; i < pending.count; i++) {
            Packet packet;
            unsigned copies = channel_packet(
                &pending.packets[i], &packet, &seed, forward_damage
            );

            for (unsigned copy = 0; copy < copies; copy++) {
                ReceiveAction received =
                    receiver_step(&receiver, &packet, now);

                if (received.deliver) {
                    TEST_ASSERT_TRUE(
                        delivered + received.length <= length
                    );

                    memcpy(output + delivered, received.payload,
                           received.length);

                    delivered += received.length;
                }

                if (received.send_ack) {
                    Packet ack;
                    unsigned ack_copies = channel_packet(
                        &received.ack, &ack, &seed, reverse_damage
                    );

                    for (unsigned j = 0; j < ack_copies; j++) {
                        SendAction action =
                            sender_step(&sender, &ack, now);

                        for (size_t k = 0; k < action.count; k++) {
                            TEST_ASSERT_TRUE(
                                following.count < MAX_WINDOW
                            );

                            following.packets[following.count] =
                                action.packets[k];

                            following.count++;
                        }
                    }
                }
            }
        }

        TEST_ASSERT_FALSE(sender.failed);

        if (sender_done(&sender)) {
            break;
        }

        /* Move the fake clock forward without sleeping. */
        now += 10;

        TEST_ASSERT_EQUAL_INT(0, receiver_status(&receiver, now));

        SendAction action = sender_step(&sender, NULL, now);

        for (size_t i = 0; i < action.count; i++) {
            TEST_ASSERT_TRUE(following.count < MAX_WINDOW);

            following.packets[following.count] = action.packets[i];
            following.count++;
        }

        pending = following;
    }

    TEST_ASSERT_FALSE(sender.failed);
    TEST_ASSERT_TRUE(sender_done(&sender));
    TEST_ASSERT_TRUE(receiver.finished);
    TEST_ASSERT_EQUAL_UINT(length, delivered);

    if (length > 0) {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(input, output, length);
    }

    TEST_ASSERT_EQUAL_INT(
        1, receiver_status(&receiver, receiver.deadline)
    );

    /* Larger transfers should exercise every damage category. */
    if (length > 10 * MAX_PAYLOAD) {
        for (unsigned i = 0; i < 3; i++) {
            TEST_ASSERT_TRUE(forward_damage[i] > 0);
            TEST_ASSERT_TRUE(reverse_damage[i] > 0);
        }
    }
}

void test_transfer_lossy(void)
{
    const uint32_t seeds[] = {7, 42, 2026};

    for (size_t i = 0; i < 3; i++) {
        simulate_transfer(70 * MAX_PAYLOAD + 17, 1, seeds[i]);
        simulate_transfer(70 * MAX_PAYLOAD + 17, 16, seeds[i]);
    }
}

void test_transfer_file_sizes(void)
{
    simulate_transfer(0, 1, 7);
    simulate_transfer(2 * MAX_PAYLOAD, 16, 42);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_get_greeting);
  RUN_TEST(test_packet_checksum);
  RUN_TEST(test_packet_encode);
  RUN_TEST(test_packet_decode);
  RUN_TEST(test_packet_decode_invalid);
  RUN_TEST(test_receiver_data);
  RUN_TEST(test_receiver_fin);
  RUN_TEST(test_receiver_idle);
  RUN_TEST(test_sender_window);
  RUN_TEST(test_sender_timeout_limit);
  RUN_TEST(test_sender_empty_file);
  RUN_TEST(test_sender_invalid_ack);
  RUN_TEST(test_transfer_lossy);
  RUN_TEST(test_transfer_file_sizes);
  return UNITY_END();
}
