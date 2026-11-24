import argparse
import asyncio
import time

import psutil
from pyrogram import Client
from pytgcalls import PyTgCalls
from pytgcalls.types import MediaStream
from pytgcalls.types.raw import AudioParameters


def cpu_seconds(processes):
    total = 0.0
    for process in processes:
        try:
            times = process.cpu_times()
            total += times.user + times.system
        except psutil.NoSuchProcess:
            pass
    return total


async def sample(process, seconds):
    start_cpu = cpu_seconds([process])
    start = time.monotonic()
    peak = 0
    for _ in range(seconds):
        await asyncio.sleep(1)
        peak = max(peak, process.memory_info().rss)
    return (cpu_seconds([process]) - start_cpu) / (time.monotonic() - start) * 100, peak


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('chats', type=int, nargs='+')
    parser.add_argument('--session', default='bench')
    parser.add_argument('--workdir', default='.')
    parser.add_argument('--seconds', type=int, default=40)
    args = parser.parse_args()

    calls = PyTgCalls(Client(args.session, workdir=args.workdir))
    await calls.start()
    process = psutil.Process()
    await asyncio.sleep(3)
    base_cpu, base_rss = await sample(process, 15)

    for chat in args.chats:
        await calls.play(chat, MediaStream(args.source, audio_parameters=AudioParameters(48000, 2), video_flags=MediaStream.Flags.IGNORE))
        await asyncio.sleep(4)
    await asyncio.sleep(8)

    children = process.children(recursive=True)
    children_start = cpu_seconds(children)
    start = time.monotonic()
    call_cpu, call_rss = await sample(process, args.seconds)
    ffmpeg_cpu = (cpu_seconds(children) - children_start) / (time.monotonic() - start) * 100

    print(f'calls={len(args.chats)} cpu={call_cpu:.2f}% ffmpeg_cpu={ffmpeg_cpu:.2f}% idle_cpu={base_cpu:.2f}% rss_delta={(call_rss - base_rss) / 1048576:.1f}MiB', flush=True)

    for chat in args.chats:
        await calls.leave_call(chat)
    await asyncio.sleep(2)


asyncio.run(main())
