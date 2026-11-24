import argparse
import asyncio
import os
import time

import psutil
from pytgcalls import GroupCallFactory
from telethon import TelegramClient
from telethon.sessions import StringSession


async def sample(process, seconds):
    start_times = process.cpu_times()
    start = time.monotonic()
    peak = 0
    for _ in range(seconds):
        await asyncio.sleep(1)
        peak = max(peak, process.memory_info().rss)
    end_times = process.cpu_times()
    spent = end_times.user + end_times.system - start_times.user - start_times.system
    return spent / (time.monotonic() - start) * 100, peak


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('chats', type=int, nargs='+')
    parser.add_argument('--seconds', type=int, default=40)
    args = parser.parse_args()

    client = TelegramClient(StringSession(os.environ['TG_SESSION']), int(os.environ['TG_API_ID']), os.environ['TG_API_HASH'])
    await client.connect()
    try:
        await client.get_dialogs(limit=30)
    except IndexError:
        pass
    process = psutil.Process()
    await asyncio.sleep(3)
    base_cpu, base_rss = await sample(process, 15)

    factory = GroupCallFactory(client, GroupCallFactory.MTPROTO_CLIENT_TYPE.TELETHON)
    calls = []
    for chat in args.chats:
        call = factory.get_file_group_call(args.source)
        await call.start(chat)
        calls.append(call)
        await asyncio.sleep(4)
    for _ in range(60):
        if all(call.is_connected for call in calls):
            break
        await asyncio.sleep(0.5)
    await asyncio.sleep(8)

    call_cpu, call_rss = await sample(process, args.seconds)
    connected = sum(call.is_connected for call in calls)
    print(f'calls={len(calls)} connected={connected} cpu={call_cpu:.2f}% idle_cpu={base_cpu:.2f}% rss_delta={(call_rss - base_rss) / 1048576:.1f}MiB', flush=True)

    for call in calls:
        await call.stop()
    await client.disconnect()


asyncio.run(main())
