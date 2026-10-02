# Isolated diagnostics

This folder contains a bounded, local-only resource diagnostic. It reports process memory, system available memory, peak resident memory, context switches, open file descriptor count, kernel details, load average, and filesystem capacity.

It does not inspect files outside the process metrics needed for the report, read environment variables, collect credentials, monitor keyboard input, make network requests, persist itself, or send data anywhere.

## Run in a separate terminal

From the repository root:

```bash
./spyware\ test/run_diagnostics.sh
```

Arguments are allocation size in MB, interval in seconds, and sample count. They are capped at 512 MB, 60 seconds, and 60 samples:

```bash
./spyware\ test/run_diagnostics.sh 128 1 10
```
