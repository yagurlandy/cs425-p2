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
