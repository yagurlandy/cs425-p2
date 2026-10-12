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