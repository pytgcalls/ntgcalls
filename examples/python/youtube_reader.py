import asyncio

from ntgcalls import NTgCalls, MediaDescription, AudioDescription, VideoDescription, InputMode
from pyrogram import Client, idle

from utils import connect_call, get_youtube_stream

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'
wrtc = NTgCalls()
chat_id = -1001919448795


async def main():
    client = Client('test', api_id, api_hash, sleep_threshold=1)

    audio, video = await get_youtube_stream("https://www.youtube.com/watch?v=u__gKd2mCVA")
    async with client:
        call_params = await wrtc.create_call(
            chat_id,
            MediaDescription(
                audio=AudioDescription(
                    input_mode=InputMode.SHELL,
                    input=f"ffmpeg -i {audio} -loglevel panic -f s16le -ac 2 -ar 96k pipe:1",
                    sample_rate=96000,
                    bits_per_sample=16,
                    channel_count=2,
                ),
                video=VideoDescription(
                    input_mode=InputMode.SHELL,
                    input=f"ffmpeg -i {video} -loglevel panic -f rawvideo -r 60 -pix_fmt yuv420p -vf scale=1920:1080 pipe:1",
                    width=1920,
                    height=1080,
                    fps=60,
                ),
            ),
        )
        result = await connect_call(client, chat_id, call_params)
        await wrtc.connect(chat_id, result)
        print("Connected!")
        await idle()
        print("Closed!")


asyncio.new_event_loop().run_until_complete(main())
