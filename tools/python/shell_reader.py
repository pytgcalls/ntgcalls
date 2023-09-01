import asyncio

from ntgcalls import NTgCalls, MediaDescription, ShellAudioDescription, ShellVideoDescription
from pyrogram import Client, idle

from utils import connect_call, ToAsync

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'
wrtc = NTgCalls()
chat_id = -1001919448795
async def main():
    client = Client('test', api_id, api_hash, sleep_threshold=1)

    async with client:
        call_params = await ToAsync(wrtc.createCall, chat_id, MediaDescription(
            audio=ShellAudioDescription(
                sampleRate=48000,
                bitsPerSample=16,
                channelCount=2,
                command="ffmpeg -i tools/test.mp4 -f s16le -ac 2 -ar 48k pipe:1",
            ),
            video=ShellVideoDescription(
                width=1280,
                height=720,
                fps=30,
                command="ffmpeg -i tools/test.mp4 -f rawvideo -pix_fmt yuv420p -vf scale=1280:720 pipe:1",
            ),
        ))
        result = await connect_call(client, chat_id, call_params)
        await ToAsync(wrtc.connect, chat_id, result)
        print("Connected!")
        await idle()
        print("Closed!")


asyncio.new_event_loop().run_until_complete(main())
