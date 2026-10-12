# Submission Report

- Submission generated at 10/12/2026 at 05:05:07

- Machine info: Linux runnervmmprz5 6.17.0-1022-azure #22-Ubuntu SMP Mon Jul 27 17:24:03 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux

## Note to Students

Please read this report carefully before submission.
Ensure that all sections are complete and accurate.
Look for any errors in the build or test outputs.
If you find any issues, correct them before submitting.
Post any questions on the class discussion board for help.


---

## README

# Project 2

- Name: Andy Lopez-Martinez
- Email: andylopezmartine@u.boisestate.edu
- Class: CS425-001

## Known Bugs or Issues

I haven't found any bugs in the transfers I tested in Codespaces.
The received files matched the originals, including when packets were
lost, corrupted, or duplicated.

## Experience

I worked through this project one part at a time and tested as I went.
That helped me understand how the sender and receiver work together.
I did accidentally put some functions after main, which caused compiler
errors. Once I moved them above main, the program built again.

The relay tests helped me see why the timer and sequence numbers matter.
Even when packets were damaged or missing, the sender could resend them
and the receiver could keep the file in order without writing duplicates.

## Design

I separated the program into three layers. The packet functions calculate
the checksum and convert packets to and from bytes. I copied each field
separately instead of copying the whole struct. The decoder checks the
header, length, type, reserved byte, and checksum before copying the payload.

The sender and receiver functions handle the protocol. The sender tracks
base, next, the saved packets, and one timer. ACKs move the window forward.
When the timer expires, it resends everything still waiting for an ACK.
The receiver only accepts the next expected DATA packet. Duplicates and
packets ahead of a gap get the current ACK again. After FIN, the receiver
waits two seconds so it can respond if FIN is sent again.

main.c handles the command line, relay registration, socket, clock,
polling, and files. The protocol functions don't do those operations.
They receive the time and return what should happen next. This let me
test them with a fake clock and packets kept in memory.

## Results

I went ahead and transferred a 1 MiB file three times for each combination below.
The relay ran with --delay 50, so it added 50 ms in each direction.
I also kept the default 250 ms timeout and set corruption and duplication
to zero. All twelve transfers passed, and every file matched with cmp.

I included Bash's time command to measure the sender. Throughput is
1024 KiB divided by the mean time. Each result is in
measurements/results.csv.

| Window | Loss | Mean time (s) | Throughput (KiB/s) |
|--------|------|---------------|-------------------|
| 1      | 0%   | 103.001       | 9.942             |
| 16     | 0%   | 6.589         | 155.411           |
| 1      | 5%   | 129.991       | 7.877             |
| 16     | 5%   | 22.534        | 45.442            |


## Analysis

With window 1 and no loss, the sender waited for an ACK for each of
the 1024 DATA packets and the FIN. Dividing 103.001 seconds by 1025
gives about 100.488 ms per round trip. The relay adds 100 ms.
The small difference comes from processing the packets, socket
operations, scheduling, and timer precision. The measured time also
includes starting the program, registering, and reading the file,
so this is an estimate.

Window 16 was about 15.63 times faster with no loss, which is close
to 16. The sender can send more packets while waiting for ACKs.
It doesn't reach exactly 16 because processing and startup still
take time, and FIN is sent after all DATA has been acknowledged.

With 5% loss, window 1 took about 1.26 times as long as its clean run.
Window 16 took about 3.42 times as long. With window 1, a timeout
only resends one packet. With window 16, it resends the whole
outstanding window. When a DATA packet is missing, the receiver
discards the later packets that arrive ahead of it, which adds
more repeated work. ACKs can be lost too, but a later cumulative
ACK can cover earlier ones that didn't arrive.

## Testing

All 14 Unity tests passed. src/lab.c had 100% line and branch
coverage under the starter coverage scope. make leak and make
leak-test finished without reported sanitizer errors.

The tests check the checksum, malformed packets, duplicates,
out-of-order packets, repeated FINs, cumulative and invalid ACKs,
full windows, and timeouts. They also check that the sender gives
up after ten timeouts without progress.

The in-memory transfers use seeds 7, 42, and 2026 with windows
1 and 16. They apply 20% loss, corruption, and duplication in
both directions, using the same conditional order as the relay.
The received bytes matched. Empty files and exact multiples of
1024 bytes were tested too.

For the real relay test, I sent a 64 KiB file with 10% loss,
5% corruption, and 10% duplication. Both programs returned 0,
and cmp confirmed the files matched.
---


## Build Output

This section was generated by running `make all` in the project root directory.

```bash
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug/lab.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug/main.c.o
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address build/debug/lab.c.o build/debug/main.c.o -o build/debug/myapp_d -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/lab.c -o build/release/lab.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/main.c -o build/release/main.c.o
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion build/release/lab.c.o build/release/main.c.o -o build/release/myapp 
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/lab.c -o build/tests/lab.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/main.c -o build/tests/main.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/lab-test.c -o build/tests/lab-test.c.o
mkdir -p build/tests/harness/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/harness/unity.c -o build/tests/harness/unity.c.o
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage build/tests/lab.c.o build/tests/main.c.o build/tests/lab-test.c.o build/tests/harness/unity.c.o -o build/tests/myapp_t -fprofile-arcs -ftest-coverage
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug-test/lab.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug-test/main.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/lab-test.c -o build/debug-test/lab-test.c.o
mkdir -p build/debug-test/harness/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/harness/unity.c -o build/debug-test/harness/unity.c.o
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address build/debug-test/lab.c.o build/debug-test/main.c.o build/debug-test/lab-test.c.o build/debug-test/harness/unity.c.o -o build/debug-test/myapp_td -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
Builds completed. You can run the application with: ./build/release/myapp
You can run the debug build with: ./build/debug/myapp_d
You can run the test build with: ./build/tests/myapp_t
You can run the debug-test build with: ./build/debug-test/myapp_td
```

---

## Coverage Report

This section was generated by running `make report` in the project root directory.

```bash
Setting up tests...
Tearing down tests...
tests/lab-test.c:733:test_get_greeting:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:734:test_packet_checksum:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:735:test_packet_encode:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:736:test_packet_decode:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:737:test_packet_decode_invalid:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:738:test_receiver_data:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:739:test_receiver_fin:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:740:test_receiver_idle:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:741:test_sender_window:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:742:test_sender_timeout_limit:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:743:test_sender_empty_file:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:744:test_sender_invalid_ack:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:745:test_transfer_lossy:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:746:test_transfer_file_sizes:PASS

-----------------------
14 Tests 0 Failures 0 Ignored 
OK
./build/tests/myapp_t
Setting up tests...
Tearing down tests...
tests/lab-test.c:733:test_get_greeting:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:734:test_packet_checksum:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:735:test_packet_encode:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:736:test_packet_decode:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:737:test_packet_decode_invalid:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:738:test_receiver_data:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:739:test_receiver_fin:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:740:test_receiver_idle:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:741:test_sender_window:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:742:test_sender_timeout_limit:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:743:test_sender_empty_file:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:744:test_sender_invalid_ack:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:745:test_transfer_lossy:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:746:test_transfer_file_sizes:PASS

-----------------------
14 Tests 0 Failures 0 Ignored 
OK
mkdir -p ./build/report/html
mkdir -p ./build/report/txt
gcovr -r . --html --html-details --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$' -o ./build/report/html/coverage_report.html
(INFO) Reading coverage data...

(INFO) Writing coverage report...

gcovr -r . --txt                 --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$'
(INFO) Reading coverage data...

(INFO) Writing coverage report...

------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                       Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
src/lab.c                                    158      158   100%
------------------------------------------------------------------------------
TOTAL                                        158      158   100%
------------------------------------------------------------------------------
```

---

## Address Sanitizer Report

This section was generated by running `make leak-test` in the project root directory.

```bash
Setting up tests...
Tearing down tests...
tests/lab-test.c:733:test_get_greeting:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:734:test_packet_checksum:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:735:test_packet_encode:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:736:test_packet_decode:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:737:test_packet_decode_invalid:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:738:test_receiver_data:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:739:test_receiver_fin:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:740:test_receiver_idle:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:741:test_sender_window:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:742:test_sender_timeout_limit:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:743:test_sender_empty_file:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:744:test_sender_invalid_ack:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:745:test_transfer_lossy:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:746:test_transfer_file_sizes:PASS

-----------------------
14 Tests 0 Failures 0 Ignored 
OK
```

---

## Src Files
### lab.c

```c

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

```

### lab.h

```c

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

```

### main.c

```c

#define _POSIX_C_SOURCE 200809L

#include "lab.h"
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <time.h>

#ifdef TEST
#define main main_exclude
#endif

typedef struct {
    bool sending;
    const char *session;
    const char *host;
    const char *file;

    unsigned window;
    unsigned timeout_ms;
    unsigned port;

    double loss;
    double corrupt;
    double dup;
} Options;

static void usage(void)
{
    puts(
        "Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]\n"
        "                  [-c corrupt] [-d dup] [-p port] <relay> <file>\n"
        "       myapp recv -s <session> [-p port] <relay> <file>\n"
        "\n"
        "Defaults: window 8, timeout 250 ms, loss/corrupt/dup 0, port 4250"
    );
}

static bool parse_number(const char *text, unsigned min,
                         unsigned max, unsigned *value)
{
    char *end;
    errno = 0;

    unsigned long number = strtoul(text, &end, 10);

    if (text[0] == '\0' || text[0] == '-' ||
        *end != '\0' || errno != 0 ||
        number < min || number > max) {
        return false;
    }

    *value = (unsigned)number;
    return true;
}

static bool parse_probability(const char *text, double *value)
{
    char *end;
    errno = 0;

    double number = strtod(text, &end);

    if (text[0] == '\0' || *end != '\0' || errno != 0 ||
        !isfinite(number) || number < 0 || number > 0.5) {
        return false;
    }

    *value = number;
    return true;
}

static bool parse_options(int argc, char **argv, Options *options)
{
    memset(options, 0, sizeof *options);

    options->window = 8;
    options->timeout_ms = 250;
    options->port = 4250;

    if (argc < 2) {
        return false;
    }

    if (strcmp(argv[1], "send") == 0) {
        options->sending = true;
    } else if (strcmp(argv[1], "recv") != 0) {
        return false;
    }

    optind = 2;
    opterr = 0;

    int option;

    while ((option = getopt(argc, argv, "s:w:T:l:c:d:p:")) != -1) {
        if (!options->sending && option != 's' && option != 'p') {
            return false;
        }

        switch (option) {
        case 's':
            options->session = optarg;
            break;

        case 'w':
            if (!parse_number(optarg, 1, 64, &options->window)) {
                return false;
            }
            break;

        case 'T':
            if (!parse_number(optarg, 1, INT_MAX, &options->timeout_ms)) {
                return false;
            }
            break;

        case 'p':
            if (!parse_number(optarg, 1, 65535, &options->port)) {
                return false;
            }
            break;

        case 'l':
            if (!parse_probability(optarg, &options->loss)) {
                return false;
            }
            break;

        case 'c':
            if (!parse_probability(optarg, &options->corrupt)) {
                return false;
            }
            break;

        case 'd':
            if (!parse_probability(optarg, &options->dup)) {
                return false;
            }
            break;

        default:
            return false;
        }
    }

    if (options->session == NULL) {
        return false;
    }

    size_t length = strlen(options->session);

    if (length < 1 || length > 32) {
        return false;
    }

    if (strspn(options->session, "abcdefghijklmnopqrstuvwxyz0123456789-")
        != length) {
        return false;
    }

    if (argc - optind != 2) {
        return false;
    }

    options->host = argv[optind];
    options->file = argv[optind + 1];

    return true;
}

static int64_t now_ms(void)
{
    struct timespec time;

    if (clock_gettime(CLOCK_MONOTONIC, &time) != 0) {
        perror("clock_gettime");
        exit(2);
    }

    return (int64_t)time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

static int wait_for_packet(int socket_fd, int64_t deadline)
{
    for (;;) {
        int64_t remaining = deadline - now_ms();
        int timeout = 0;

        if (remaining > INT_MAX) {
            timeout = INT_MAX;
        } else if (remaining > 0) {
            timeout = (int)remaining;
        }

        struct pollfd descriptor = {0};
        descriptor.fd = socket_fd;
        descriptor.events = POLLIN;

        int result = poll(&descriptor, 1, timeout);

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            return -1;
        }

        if (result > 0 &&
            (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            fprintf(stderr, "UDP socket error\n");
            return -1;
        }

        return result;
    }
}

static int open_relay(const Options *options)
{
    char port[6];
    snprintf(port, sizeof port, "%u", options->port);

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *addresses = NULL;

    int result = getaddrinfo(options->host, port, &hints, &addresses);

    if (result != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    int socket_fd = -1;

    for (struct addrinfo *address = addresses;
         address != NULL; address = address->ai_next) {

        socket_fd = socket(address->ai_family,
                           address->ai_socktype,
                           address->ai_protocol);

        if (socket_fd < 0) {
            continue;
        }

        if (connect(socket_fd, address->ai_addr,
                    address->ai_addrlen) == 0) {
            break;
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(addresses);

    if (socket_fd < 0) {
        perror("open relay");
    }

    return socket_fd;
}

static bool send_bytes(int socket_fd, const void *bytes, size_t length)
{
    ssize_t sent;

    do {
        sent = send(socket_fd, bytes, length, 0);
    } while (sent < 0 && errno == EINTR);

    if (sent < 0) {
        perror("send");
        return false;
    }

    if ((size_t)sent != length) {
        fprintf(stderr, "Incomplete datagram send\n");
        return false;
    }

    return true;
}

static bool register_relay(int socket_fd, const Options *options)
{
    char hello[256];
    int length;

    if (options->sending) {
        length = snprintf(
            hello, sizeof hello, "HELLO %s send %.17g %.17g %.17g",
            options->session,
            options->loss, options->corrupt, options->dup
        );
    } else {
        length = snprintf(
            hello, sizeof hello, "HELLO %s recv", options->session
        );
    }

    if (length < 0 || (size_t)length >= sizeof hello) {
        fprintf(stderr, "Could not format HELLO\n");
        return false;
    }

    for (unsigned attempt = 0; attempt < 5; attempt++) {
        if (!send_bytes(socket_fd, hello, (size_t)length)) {
            return false;
        }

        int64_t deadline = now_ms() + 1000;

        for (;;) {
            int ready = wait_for_packet(socket_fd, deadline);

            if (ready < 0) {
                return false;
            }

            if (ready == 0) {
                break;
            }

            char reply[256];
            ssize_t received =
                recv(socket_fd, reply, sizeof reply - 1, 0);

            if (received < 0) {
                if (errno == EINTR) {
                    continue;
                }

                perror("HELLO reply");
                return false;
            }

            reply[received] = '\0';

            if (received == 2 && memcmp(reply, "OK", 2) == 0) {
                return true;
            }

            if (received >= 4 && memcmp(reply, "ERR ", 4) == 0) {
                fprintf(stderr, "Relay: %s\n", reply + 4);
                return false;
            }

            /* Other datagrams do not extend the HELLO deadline. */
        }
    }

    fprintf(stderr, "Relay did not reply after five HELLO attempts\n");
    return false;
}

static bool send_packet(int socket_fd, const Packet *packet)
{
    uint8_t buffer[MAX_PACKET_SIZE];

    size_t size = packet_encode(packet, buffer, sizeof buffer);

    if (size == 0) {
        fprintf(stderr, "Could not encode packet\n");
        return false;
    }

    return send_bytes(socket_fd, buffer, size);
}

/* Return 1 for a valid packet, 0 for a discarded packet,
 * and -1 for a network error.
 */
static int receive_packet(int socket_fd, Packet *packet)
{
    /* Read the whole datagram so an oversized packet cannot
     * appear valid just because its beginning was truncated.
     */
    uint8_t buffer[65536];

    ssize_t size = recv(socket_fd, buffer, sizeof buffer, 0);

    if (size < 0) {
        if (errno == EINTR) {
            return 0;
        }

        perror("recv");
        return -1;
    }

    if (!packet_decode(buffer, (size_t)size, packet)) {
        return 0;
    }

    return 1;
}

static int send_file(int socket_fd, const Options *options)
{
    const size_t max_file_size = 16u * 1024u * 1024u;

    FILE *file = fopen(options->file, "rb");

    if (file == NULL) {
        perror(options->file);
        return 2;
    }

    /* One extra byte lets us detect an oversized file. */
    uint8_t *bytes = malloc(max_file_size + 1);

    if (bytes == NULL) {
        perror("malloc");
        fclose(file);
        return 2;
    }

    size_t length = fread(bytes, 1, max_file_size + 1, file);
    bool read_error = ferror(file) != 0;

    if (fclose(file) != 0) {
        read_error = true;
    }

    if (read_error || length > max_file_size) {
        fprintf(stderr, "Input read failed or file exceeds 16 MiB\n");
        free(bytes);
        return 2;
    }

    Sender sender;
    sender_init(&sender, bytes, length,
                options->window, options->timeout_ms);

    Packet ack;
    const Packet *event = NULL;
    int result = 2;

    for (;;) {
        SendAction action = sender_step(&sender, event, now_ms());
        event = NULL;

        if (sender.failed) {
            fprintf(stderr,
                    "Transfer gave up after 10 timeouts without progress\n");
            break;
        }

        if (sender_done(&sender)) {
            result = 0;
            break;
        }

        bool sent_all = true;

        for (size_t i = 0; i < action.count; i++) {
            if (!send_packet(socket_fd, &action.packets[i])) {
                sent_all = false;
                break;
            }
        }

        if (!sent_all) {
            break;
        }

        int ready = wait_for_packet(socket_fd, sender.deadline);

        if (ready < 0) {
            break;
        }

        if (ready > 0) {
            int received = receive_packet(socket_fd, &ack);

            if (received < 0) {
                break;
            }

            if (received == 1) {
                event = &ack;
            }
        }
    }

    free(bytes);
    return result;
}

static int receive_file(int socket_fd, const Options *options)
{
    FILE *file = fopen(options->file, "wb");

    if (file == NULL) {
        perror(options->file);
        return 2;
    }

    Receiver receiver;
    receiver_init(&receiver, now_ms());

    int result = 2;

    for (;;) {
        int status = receiver_status(&receiver, now_ms());

        if (status == 1) {
            result = 0;
            break;
        }

        if (status == 2) {
            fprintf(stderr, "Receiver idle for 30 seconds\n");
            break;
        }

        int ready = wait_for_packet(socket_fd, receiver.deadline);

        if (ready < 0) {
            break;
        }

        if (ready == 0) {
            continue;
        }

        Packet packet;
        int received = receive_packet(socket_fd, &packet);

        if (received < 0) {
            break;
        }

        if (received == 0) {
            continue;
        }

        ReceiveAction action =
            receiver_step(&receiver, &packet, now_ms());

        if (action.deliver) {
            size_t written =
                fwrite(action.payload, 1, action.length, file);

            if (written != action.length) {
                fprintf(stderr, "Could not write output file\n");
                break;
            }
        }

        if (action.close_file) {
            int error = fclose(file);
            file = NULL;

            if (error != 0) {
                perror("close output");
                break;
            }
        }

        if (action.send_ack) {
            if (!send_packet(socket_fd, &action.ack)) {
                break;
            }
        }
    }

    if (file != NULL) {
        if (fclose(file) != 0) {
            perror("close output");
            result = 2;
        }
    }

    return result;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        usage();
        return 0;
    }

    Options options;

    if (!parse_options(argc, argv, &options)) {
        usage();
        return 1;
    }

    int socket_fd = open_relay(&options);

    if (socket_fd < 0) {
        return 2;
    }

    int result = 2;

        if (register_relay(socket_fd, &options)) {
        if (options.sending) {
            result = send_file(socket_fd, &options);
        } else {
            result = receive_file(socket_fd, &options);
        }
    }

    close(socket_fd);
    return result;
}

```

## Tests Files
### lab-test.c

```c

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

```

## Scripts Files
### measure.sh

```c

#!/usr/bin/env bash
set -euo pipefail

mkdir -p measurements
head -c 1048576 /dev/urandom > 1mib.bin

printf 'window,loss,run,seconds\n' > measurements/results.csv

receiver_pid=""

cleanup() {
    if [[ -n "$receiver_pid" ]]; then
        kill "$receiver_pid" 2>/dev/null || true
    fi
}

trap cleanup EXIT

for window in 1 16; do
    for loss in 0 0.05; do
        for run in 1 2 3; do
            session="andy-w${window}-l${loss//./}-r${run}"
            output="measurements/${session}.bin"
            timing="measurements/${session}-time.txt"

            echo "Running window=$window loss=$loss run=$run"

            rm -f "$output"

            ./build/release/myapp recv \
                -s "$session" 127.0.0.1 "$output" \
                > "measurements/${session}-recv.log" 2>&1 &

            receiver_pid=$!

            # The output file opens after receiver registration succeeds.
            for attempt in {1..50}; do
                if [[ -f "$output" ]]; then
                    break
                fi

                sleep 0.1
            done

            if [[ ! -f "$output" ]]; then
                cat "measurements/${session}-recv.log"
                echo "Receiver did not become ready."
                exit 1
            fi

            TIMEFORMAT='%3R'

            {
                time ./build/release/myapp send \
                    -s "$session" -w "$window" -l "$loss" \
                    127.0.0.1 1mib.bin \
                    > "measurements/${session}-send.log" 2>&1
            } 2> "$timing"

            wait "$receiver_pid"
            receiver_pid=""

            cmp 1mib.bin "$output"

            seconds=$(cat "$timing")

            printf '%s,%s,%s,%s\n' \
                "$window" "$loss" "$run" "$seconds" \
                >> measurements/results.csv

            echo "Passed: ${seconds} seconds; files identical."
        done
    done
done

echo "All twelve transfers passed."
cat measurements/results.csv
```

Report generated on 10/12/2026 at 05:05:09


---

## End of Report

SHA-256 Hash of the report: 35d9895a5e146cd4682418777128ea9e06bb3b314491c6adcc02d51453415dc0

Do not edit the generated report. Any changes will be reported as academic dishonesty

---
## GitHub Info
- GitHub repo name: yagurlandy/cs425-p2
- The repository visibility is public.
- The workflow was triggered by yagurlandy
