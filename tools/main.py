import asyncio
import json

from pyrogram import Client, idle
from pyrogram.raw.functions.channels import GetFullChannel
from pyrogram.raw.functions.messages import GetFullChat
from pyrogram.raw.functions.phone import JoinGroupCall
from pyrogram.raw.types import UpdateGroupCallConnection, Updates, DataJSON, InputChannel

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'

async def main():
    client = Client('test', api_id, api_hash)

    async with client:
        chat = await client.resolve_peer(-1001919448795)
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
        params = json.loads(input("Enter the LocalDescription: "))
        request_call = {
            'ufrag': params['ufrag'],
            'pwd': params['pwd'],
            'fingerprints': [{
                'hash': params['hash'],
                'setup': params['setup'],
                'fingerprint': params['fingerprint'],
            }],
            'ssrc': params['source'],
        }
        result: Updates = await client.invoke(
            JoinGroupCall(
                call=input_call,
                params=DataJSON(data=json.dumps(request_call)),
                muted=False,
                join_as=local_peer,
            ),
        )
        for update in result.updates:
            if isinstance(update, UpdateGroupCallConnection):
                print(f"\nCopy this remote description:\n{update.params.data}")
                # NEEDED A WAY TO INTERCOMMUNICATE WITH C++

        await idle()

asyncio.new_event_loop().run_until_complete(main())