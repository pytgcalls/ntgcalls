import asyncio

from pyrogram import Client
from pyrogram import idle
from utils import connect_call
from utils import get_youtube_stream

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

    audio, video = await get_youtube_stream(
        'https://www.youtube.com/watch?v=u__gKd2mCVA'
    )
    async with client:
        call_params = await wrtc.create_call(
            chat_id,
        )
        await wrtc.set_stream_sources(
            chat_id,
            StreamMode.CAPTURE,
            MediaDescription(
                microphone=AudioDescription(
                    media_source=MediaSource.SHELL,
                    input=(
                        f'ffmpeg -i {audio} -loglevel panic '
                        f'-f s16le -ac 2 -ar 96k pipe:1'
                    ),
                    sample_rate=96000,
                    channel_count=2,
                ),
                camera=VideoDescription(
                    media_source=MediaSource.SHELL,
                    input=(
                        f'ffmpeg -i {video} -loglevel panic -f rawvideo '
                        f'-r 60 -pix_fmt yuv420p -vf scale=1920:1080 pipe:1'
                    ),
                    width=1920,
                    height=1080,
                    fps=60,
                ),
            ),
        )
        result = await connect_call(client, chat_id, call_params)
        await wrtc.connect(chat_id, result, False)
        print('Connected!')
        await idle()
        print('Closed!')


asyncio.run(main())
