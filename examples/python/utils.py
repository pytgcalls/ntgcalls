import asyncio

from asyncio import AbstractEventLoop
from typing import Any
from typing import Callable

from pyrogram import Client
from pyrogram.raw.functions.channels import GetFullChannel
from pyrogram.raw.functions.phone import JoinGroupCall
from pyrogram.raw.types import UpdateGroupCallConnection, Updates, DataJSON, InputChannel


async def connect_call(client: Client, chat_id: int, call_params: str) -> str:
    chat = await client.resolve_peer(chat_id)
    local_peer = await client.resolve_peer((await client.get_me()).id)
    input_call = (
        await client.invoke(
            GetFullChannel(
                channel=InputChannel(
                    channel_id=chat.channel_id,
                    access_hash=chat.access_hash,
                ),
            ),
        )
    ).full_chat.call
    result: Updates = await client.invoke(
        JoinGroupCall(
            call=input_call,
            params=DataJSON(data=call_params),
            video_stopped=False,
            muted=False,
            join_as=local_peer,
        ), sleep_threshold=0, retries=0
    )
    for update in result.updates:
        if isinstance(update, UpdateGroupCallConnection):
            return update.params.data


async def get_youtube_stream(link: str) -> (str, str):
    proc = await asyncio.create_subprocess_exec(
        'yt-dlp',
        '-g',
        '-f',
        'bestvideo+bestaudio/best',
        link,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    stdout, stderr = await proc.communicate()
    links = stdout.decode().split('\n')
    return links[1], links[0]
