import asyncio

from ntgcalls import NTgCalls, MediaDescription, AudioDescription, VideoDescription, InputMode
from pyrogram import Client, idle

from utils import connect_call, ToAsync, get_youtube_stream

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'
wrtc = NTgCalls()
chat_id = -1001919448795


async def main():
    client = Client('test', api_id, api_hash, sleep_threshold=1)

    link = await get_youtube_stream("https://www.youtube.com/watch?v=u__gKd2mCVA")

    async with client:
        call_params = await ToAsync(wrtc.create_call, chat_id, MediaDescription(
            audio=AudioDescription(
                input_mode=InputMode.Shell,
                input=f"ffmpeg -i {link} -loglevel panic -f s16le -ac 2 -ar 48k pipe:1",
                sample_rate=48000,
                bits_per_sample=16,
                channel_count=2,
            ),
            video=VideoDescription(
                input_mode=InputMode.Shell,
                input=f"ffmpeg -i {link} -loglevel panic -f rawvideo -r 30 -pix_fmt yuv420p -vf scale=1280:720 pipe:1",
                width=1280,
                height=720,
                fps=30,
            ),
        ))
        result = await connect_call(client, chat_id, call_params)
        await ToAsync(wrtc.connect, chat_id, result)
        print("Connected!")
        await idle()
        print("Closed!")


asyncio.new_event_loop().run_until_complete(main())
