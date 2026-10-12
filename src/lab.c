#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

char *get_greeting(const char *restrict name)
{
  if (name == NULL)
  {
    return NULL;
  }

  // Allocate memory for the greeting message
  int length = snprintf(NULL, 0, "Hello, %s!", name);
  if (length < 0) // GCOVR_EXCL_START
  {
    return NULL; // snprintf failed
  } // GCOVR_EXCL_STOP

  //Casting is safe here because we know length is non-negative
  size_t alloc_size = (size_t) length + 1; // +1 for the null terminator
  char *greeting = malloc( alloc_size);


  if (greeting == NULL) // GCOVR_EXCL_START
  {
    return NULL; // Memory allocation failed
  }  // GCOVR_EXCL_STOP


  // Create the greeting message
  snprintf(greeting, alloc_size, "Hello, %s!", name);

  return greeting;
}

uint16_t packet_checksum(const uint8_t *bytes, size_t length)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < length; i += 2) {
        sum += (uint32_t)bytes[i] << 8;

        if (i + 1 < length) {
            sum += bytes[i + 1];
        }
    }

    while (sum >> 16) {
        sum = (sum & 0xffffu) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

size_t packet_encode(const Packet *packet, uint8_t *buffer,
                     size_t capacity)
{
    if (packet->type > FIN) {
        return 0;
    }

    if (packet->length > MAX_PAYLOAD) {
        return 0;
    }

    if (packet->type != DATA && packet->length != 0) {
        return 0;
    }

    size_t size = HEADER_SIZE + (size_t)packet->length;

    if (capacity < size) {
        return 0;
    }

    buffer[0] = packet->type;
    buffer[1] = 0;

    /* The checksum field must be zero during calculation. */
    buffer[2] = 0;
    buffer[3] = 0;

    uint32_t seq = htonl(packet->seq);
    uint16_t length = htons(packet->length);

    memcpy(buffer + 4, &seq, sizeof seq);
    memcpy(buffer + 8, &length, sizeof length);
    memcpy(buffer + HEADER_SIZE, packet->payload, packet->length);

    uint16_t checksum = htons(packet_checksum(buffer, size));
    memcpy(buffer + 2, &checksum, sizeof checksum);

    return size;
}

bool packet_decode(const uint8_t *buffer, size_t size, Packet *packet)
{
    if (size < HEADER_SIZE) {
        return false;
    }

    uint16_t length;
    memcpy(&length, buffer + 8, sizeof length);
    length = ntohs(length);

    if (length > MAX_PAYLOAD) {
        return false;
    }

    if (size != HEADER_SIZE + (size_t)length) {
        return false;
    }

    if (buffer[0] > FIN) {
        return false;
    }

    if (buffer[1] != 0) {
        return false;
    }

    if (buffer[0] != DATA && length != 0) {
        return false;
    }

    if (packet_checksum(buffer, size) != 0) {
        return false;
    }

    uint32_t seq;
    memcpy(&seq, buffer + 4, sizeof seq);

    packet->type = buffer[0];
    packet->seq = ntohl(seq);
    packet->length = length;

    memcpy(packet->payload, buffer + HEADER_SIZE, length);

    return true;
}

void receiver_init(Receiver *receiver, int64_t now)
{
    receiver->expected = 0;
    receiver->finished = false;
    receiver->deadline = now + 30000;
}

ReceiveAction receiver_step(Receiver *receiver,
                            const Packet *packet, int64_t now)
{
    ReceiveAction action = {0};

    /* Only packets that passed packet_decode belong here. */
    if (packet->type == ACK) {
        return action;
    }

    if (receiver->finished) {
        /* During linger, acknowledge a repeated FIN. */
        if (packet->type == FIN &&
            packet->seq == receiver->expected - 1) {
            action.send_ack = true;
        }
    } else {
        receiver->deadline = now + 30000;

        if (packet->type == DATA) {
            action.send_ack = true;

            if (packet->seq == receiver->expected) {
                action.deliver = true;
                action.length = packet->length;

                memcpy(action.payload, packet->payload, packet->length);
                receiver->expected++;
            }
        } else if (packet->seq == receiver->expected) {
            receiver->expected++;
            receiver->finished = true;
            receiver->deadline = now + 2000;

            action.close_file = true;
            action.send_ack = true;
        }
    }

    action.ack.type = ACK;
    action.ack.seq = receiver->expected;

    return action;
}

int receiver_status(const Receiver *receiver, int64_t now)
{
    if (now < receiver->deadline) {
        return 0;
    }

    if (receiver->finished) {
        return 1;
    }

    return 2;
}

void sender_init(Sender *sender, const uint8_t *bytes, size_t length,
                 unsigned window, unsigned timeout_ms)
{
    memset(sender, 0, sizeof *sender);

    sender->bytes = bytes;
    sender->length = length;
    sender->window = window;
    sender->timeout_ms = timeout_ms;

    sender->data_count =
        (uint32_t)((length + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
}

bool sender_done(const Sender *sender)
{
    return sender->base == sender->data_count + 1;
}

SendAction sender_step(Sender *sender, const Packet *ack, int64_t now)
{
    SendAction action = {0};

    if (sender->failed || sender_done(sender)) {
        return action;
    }

    /* Accept only ACKs for packets we actually sent. */
    if (ack != NULL &&
        ack->type == ACK &&
        ack->length == 0 &&
        ack->seq > sender->base &&
        ack->seq <= sender->next) {

        sender->base = ack->seq;
        sender->timeouts = 0;

        sender->timer_running = sender->base != sender->next;
        sender->deadline = now + sender->timeout_ms;
    }

    if (sender->timer_running && now >= sender->deadline) {
        sender->timeouts++;

        if (sender->timeouts >= 10) {
            sender->failed = true;
            return action;
        }

        /* Go back and resend every unacknowledged packet. */
        for (uint32_t i = sender->base; i < sender->next; i++) {
            action.packets[action.count] =
                sender->saved[i % MAX_WINDOW];

            action.count++;
        }

        sender->deadline = now + sender->timeout_ms;
        return action;
    }

    uint32_t limit = sender->data_count;

    /* FIN can be sent only after every DATA packet is acknowledged. */
    if (sender->base == sender->data_count) {
        limit++;
    }

    while (sender->next < limit &&
           sender->next - sender->base < sender->window) {

        Packet packet = {0};
        packet.seq = sender->next;

        if (sender->next == sender->data_count) {
            packet.type = FIN;
        } else {
            packet.type = DATA;

            size_t offset = (size_t)sender->next * MAX_PAYLOAD;
            size_t remaining = sender->length - offset;

            if (remaining > MAX_PAYLOAD) {
                packet.length = MAX_PAYLOAD;
            } else {
                packet.length = (uint16_t)remaining;
            }

            memcpy(packet.payload, sender->bytes + offset,
                   packet.length);
        }

        sender->saved[sender->next % MAX_WINDOW] = packet;

        action.packets[action.count] = packet;
        action.count++;
        sender->next++;

        if (!sender->timer_running) {
            sender->timer_running = true;
            sender->deadline = now + sender->timeout_ms;
        }
    }

    return action;
}
