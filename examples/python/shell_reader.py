import asyncio

from ntgcalls import NTgCalls, MediaDescription, AudioDescription, VideoDescription, MediaSource, StreamMode
from pyrogram import Client, idle

from utils import connect_call

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'
wrtc = NTgCalls()
chat_id = -1001919448795


async def main():
    client = Client('test', api_id, api_hash, sleep_threshold=1)

    link = 'https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4'

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
                    input=f"ffmpeg -i {link} -f s16le -ac 2 -ar 96k pipe:1",
                    sample_rate=96000,
                    channel_count=2,
                ),
                camera=VideoDescription(
                    media_source=MediaSource.SHELL,
                    input=f"ffmpeg -i {link} -f rawvideo -r 30 -pix_fmt yuv420p -vf scale=1280:720 pipe:1",
                    width=1280,
                    height=720,
                    fps=30,
                ),
            ),
        )
        result = await connect_call(client, chat_id, call_params)
        await wrtc.connect(chat_id, result, True)
        print("Connected!")
        await idle()
        print("Closed!")


asyncio.run(main())
