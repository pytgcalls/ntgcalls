import asyncio
from asyncio import AbstractEventLoop
from typing import Any

from ntgcalls import NTgCalls, StreamType, MediaState, MediaDescription, AudioDescription, VideoDescription, \
    FFmpegOptions
from pyrogram import Client, idle
from pyrogram.raw.functions.channels import GetFullChannel
from pyrogram.raw.functions.phone import JoinGroupCall
from pyrogram.raw.types import UpdateGroupCallConnection, Updates, DataJSON, InputChannel

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'

wrtc = NTgCalls()

chat_id = -1001919448795


class ToAsync:
    def __init__(self, function: callable, *args):
        self._loop: AbstractEventLoop = asyncio.get_running_loop()
        self._function: callable = function
        self._function_args: tuple = args

    async def _run(self):
        result: Any = await self._loop.run_in_executor(
            None,
            self._function,
            *self._function_args
        )

        return result

    def __await__(self):
        return self._run().__await__()


async def main():
    client = Client('test', api_id, api_hash, sleep_threshold=1)

    def stream_end(test: StreamType):
        print("Python Callback:", test.name)

    wrtc.onStreamEnd(stream_end)

    def stream_upgrade(test: MediaState):
        print("Python Callback:", test)

    wrtc.onUpgrade(stream_upgrade)

    file_audio = "output.pcm"
    file_video = "output.i420"
    async with client:
        call_params = await ToAsync(wrtc.createCall, chat_id, MediaDescription(
            encoder="raw",
            audio=AudioDescription(
                sampleRate=48000,
                bitsPerSample=16,
                channelCount=2,
                path=file_audio,
                options=FFmpegOptions()
            ),
            video=VideoDescription(
                width=1920,
                height=1080,
                fps=30,
                path=file_video,
                options=FFmpegOptions()
            ),
        ))
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
                await ToAsync(wrtc.connect, chat_id, update.params.data)
        print("Connected!")
        await idle()
        print("Closed!")


asyncio.new_event_loop().run_until_complete(main())
