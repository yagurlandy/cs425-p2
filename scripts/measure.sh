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