#ifndef LAB_H
#define LAB_H
#include <stddef.h>
#include <stdint.h>
#define MAX_PAYLOAD 1024
#define HEADER_SIZE 10
#define MAX_PACKET_SIZE (HEADER_SIZE + MAX_PAYLOAD)

#define DATA 0
#define ACK 1
#define FIN 2

#include <stdbool.h>

/** * @brief Returns a greeting message.
 *
 * This function returns a string that contains a greeting message.
 * The string is allocated with malloc and should be freed by the caller.
 * @param name The name to include in the greeting.
 * @return A greeting string.
 */
char* get_greeting(const char* restrict name);

uint16_t packet_checksum(const uint8_t *bytes, size_t length);

typedef struct {
    uint8_t type;
    uint32_t seq;
    uint16_t length;
    uint8_t payload[MAX_PAYLOAD];
} Packet;

size_t packet_encode(const Packet *packet, uint8_t *buffer,
                     size_t capacity);

bool packet_decode(const uint8_t *buffer, size_t size, Packet *packet);

typedef struct {
    uint32_t expected;
    bool finished;
    int64_t deadline;
} Receiver;

typedef struct {
    bool send_ack;
    bool deliver;
    bool close_file;
    Packet ack;
    size_t length;
    uint8_t payload[MAX_PAYLOAD];
} ReceiveAction;

void receiver_init(Receiver *receiver, int64_t now);

ReceiveAction receiver_step(Receiver *receiver,
                            const Packet *packet, int64_t now);

int receiver_status(const Receiver *receiver, int64_t now);

#define MAX_WINDOW 64

typedef struct {
    const uint8_t *bytes;
    size_t length;

    uint32_t data_count;
    uint32_t base;
    uint32_t next;

    unsigned window;
    unsigned timeout_ms;
    unsigned timeouts;

    bool timer_running;
    bool failed;
    int64_t deadline;

    Packet saved[MAX_WINDOW];
} Sender;

typedef struct {
    size_t count;
    Packet packets[MAX_WINDOW];
} SendAction;

/* bytes must stay available until the transfer ends.
 * window must be 1-64, timeout_ms positive, and length at most 16 MiB.
 */
void sender_init(Sender *sender, const uint8_t *bytes, size_t length,
                 unsigned window, unsigned timeout_ms);

bool sender_done(const Sender *sender);

SendAction sender_step(Sender *sender, const Packet *ack, int64_t now);

#endif // LAB_H
