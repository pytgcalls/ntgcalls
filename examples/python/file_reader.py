import asyncio

from pyrogram import Client
from pyrogram import idle
from utils import connect_call

from ntgcalls import AudioDescription
from ntgcalls import MediaDescription
from ntgcalls import MediaSource
from ntgcalls import NTgCalls
from ntgcalls import StreamMode
from ntgcalls import VideoDescription

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'
wrtc = NTgCalls()
chat_id = -1001919448795


async def main():
    client = Client('test', api_id, api_hash, sleep_threshold=1)

    async with client:
        call_params = await wrtc.create_call(
            chat_id,
        )
        await wrtc.set_stream_sources(
            chat_id,
            StreamMode.CAPTURE,
            MediaDescription(
                microphone=AudioDescription(
                    media_source=MediaSource.FILE,
                    input='output.pcm',
                    sample_rate=48000,
                    channel_count=2,
                ),
                camera=VideoDescription(
                    media_source=MediaSource.FILE,
                    input='output.i420',
                    width=1280,
                    height=720,
                    fps=30,
                ),
            ),
        )
        result = await connect_call(client, chat_id, call_params)
        await wrtc.connect(chat_id, result, False)
        print('Connected!')
        await idle()
        print('Closed!')


asyncio.run(main())
