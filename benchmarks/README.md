# Benchmarks

These scripts measure the CPU and memory a process spends streaming audio into Telegram group calls. They produce the numbers in the [main README](../README.md#benchmarks).

## What is measured
Each script starts its client, measures the idle process for 15 seconds, joins one group call per chat, waits 8 seconds and then samples the process for 40 seconds. It prints:

- `cpu`: the CPU used while streaming, as a share of one core.
- `idle_cpu`: the CPU used before joining.
- `rss_delta`: the memory added by the calls.
- `ffmpeg_cpu` (NTgCalls only): the CPU used by the FFmpeg processes that read the source.

## Setup
- Two or more groups with an active video chat, where the account can speak. Telegram accepts about six joins in a short time from one account, so keep five calls or fewer per run and wait a few minutes between runs.
- A source file, for example a lossless track converted to the formats below:

```bash
ffmpeg -i track.flac -t 75 -f s16le -ac 2 -ar 48000 input.raw
ffmpeg -f s16le -ar 48000 -ac 2 -i input.raw input.wav
ffmpeg -i input.wav -c:a libopus -b:a 128k input.ogg
```

## NTgCalls
Requires Py-TgCalls 4.0 and `psutil`. The session is a Pyrogram session in the working directory.

```bash
pip install py-tgcalls psutil
python ntgcalls_bench.py input.ogg -1001111111111 -1002222222222 --session bench
python ntgcalls_bench.py input.wav -1001111111111 -1002222222222 --session bench
```

`input.ogg` is sent without re-encoding; `input.wav` is encoded to Opus like any PCM source.

## Official tgcalls
The official engine is driven through its Python binding, which only runs up to Python 3.9 and only reads raw PCM. A container keeps it apart from the rest of the system:

```bash
docker run --rm --network host -v "$PWD:/bench" \
    -e TG_SESSION -e TG_API_ID -e TG_API_HASH \
    python:3.9-slim sh -c "pip install -q 'pytgcalls[telethon]==2.1.0' 'telethon==1.24.0' psutil && \
    python /bench/tgcalls_bench.py /bench/input.raw -1001111111111 -1002222222222"
```

`TG_SESSION` is a Telethon string session of the same account, and `TG_API_ID` and `TG_API_HASH` are its API credentials.

To compare an Opus source, add the CPU FFmpeg needs to decode one stream in real time, once per call:

```bash
/usr/bin/time -f "%U %S %e" ffmpeg -re -i input.ogg -t 40 -f s16le -ac 2 -ar 48000 - > /dev/null
```

The decoding share is `(user + system) / elapsed`.
