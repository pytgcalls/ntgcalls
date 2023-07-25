from pyrogram import Client

api_id = 2799555
api_hash = '47d66bbf0939d0ddf32caf8bad590ed7'
client = Client(api_id, api_hash)

with client:
    client