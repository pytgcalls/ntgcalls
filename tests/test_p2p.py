import asyncio
import math
import os
import struct
import tempfile
import unittest

import ntgcalls

USER_ID = 1
VERSION = '8.0.0'
SAMPLE_RATE = 48000
CHANNELS = 2
TIMEOUT = 20
RFC3526_PRIME = bytes.fromhex(
    'FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74'
    '020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F1437'
    '4FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED'
    'EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF05'
    '98DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB'
    '9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B'
    'E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718'
    '3995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF'
)


def write_tone(path: str, seconds: float):
    samples = int(SAMPLE_RATE * seconds)
    with open(path, 'wb') as f:
        for i in range(samples):
            value = int(12000 * math.sin(2 * math.pi * 440 * i / SAMPLE_RATE))
            f.write(struct.pack('<hh', value, value))


def rms(data: bytes) -> float:
    count = len(data) // 2
    if not count:
        return 0
    values = struct.unpack(f'<{count}h', data[:count * 2])
    return math.sqrt(sum(v * v for v in values) / count)


def ogg_packets(data: bytes) -> list[bytes]:
    packets = []
    partial = b''
    offset = 0
    while offset + 27 <= len(data) and data[offset:offset + 4] == b'OggS':
        segments = data[offset + 26]
        lacing = data[offset + 27:offset + 27 + segments]
        body = offset + 27 + segments
        for length in lacing:
            partial += data[body:body + length]
            body += length
            if length < 255:
                packets.append(partial)
                partial = b''
        offset = body
    return packets


def opus_duration_ms(packet: bytes) -> float:
    config = packet[0] >> 3
    if config < 12:
        frame = (10, 20, 40, 60)[config % 4]
    elif config < 16:
        frame = (10, 20)[config % 2]
    else:
        frame = (2.5, 5, 10, 20)[config % 4]
    code = packet[0] & 0x03
    frames = 1 if code == 0 else 2 if code < 3 else packet[1] & 0x3F
    return frame * frames


def pcm_input(path: str) -> ntgcalls.MediaDescription:
    return ntgcalls.MediaDescription(
        microphone=ntgcalls.AudioDescription(ntgcalls.MediaSource.FILE, SAMPLE_RATE, CHANNELS, path, False, format=ntgcalls.Format.PCM),
    )


class LocalCall:
    def __init__(self, loop: asyncio.AbstractEventLoop):
        self.loop = loop
        self.caller = ntgcalls.NTgCalls()
        self.callee = ntgcalls.NTgCalls()
        self.connected = {self.caller: asyncio.Event(), self.callee: asyncio.Event()}
        self.emojis = {}
        self.caller.on_signaling_data(self.forward(self.callee))
        self.callee.on_signaling_data(self.forward(self.caller))
        for calls in (self.caller, self.callee):
            calls.on_connection_change(self.connection_listener(calls))

    def forward(self, target: ntgcalls.NTgCalls):
        return lambda user_id, data: self.loop.call_soon_threadsafe(self.deliver, target, bytes(data))

    def deliver(self, target: ntgcalls.NTgCalls, data: bytes):
        asyncio.ensure_future(target.send_signaling_data(USER_ID, data))

    def connection_listener(self, calls: ntgcalls.NTgCalls):
        return lambda chat_id, info: info.state == ntgcalls.ConnectionState.CONNECTED and self.loop.call_soon_threadsafe(self.connected[calls].set)

    async def exchange_keys(self):
        dh_config = ntgcalls.DhConfig(3, RFC3526_PRIME, os.urandom(256))
        g_a_hash = await self.caller.init_exchange(USER_ID, dh_config, None)
        g_b = await self.callee.init_exchange(USER_ID, dh_config, g_a_hash)
        caller_auth = await self.caller.exchange_keys(USER_ID, g_b, 0)
        callee_auth = await self.callee.exchange_keys(USER_ID, caller_auth.g_a_or_b, caller_auth.key_fingerprint)
        return caller_auth, callee_auth

    async def connect(self, callee_media: ntgcalls.MediaDescription | None = None, caller_playback: ntgcalls.MediaDescription | None = None):
        for calls in (self.caller, self.callee):
            await calls.create_p2p_call(USER_ID)
        if callee_media is not None:
            await self.callee.set_stream_sources(USER_ID, ntgcalls.StreamMode.CAPTURE, callee_media)
        if caller_playback is not None:
            await self.caller.set_stream_sources(USER_ID, ntgcalls.StreamMode.PLAYBACK, caller_playback)
        await self.exchange_keys()
        await self.callee.connect_p2p(USER_ID, [], [VERSION], True, None)
        await self.caller.connect_p2p(USER_ID, [], [VERSION], True, None)
        for event in self.connected.values():
            await asyncio.wait_for(event.wait(), TIMEOUT)

    async def stop(self):
        for calls in (self.caller, self.callee):
            await calls.stop(USER_ID)


class TestKeyExchange(unittest.IsolatedAsyncioTestCase):
    async def test_both_sides_derive_the_same_key(self):
        call = LocalCall(asyncio.get_running_loop())
        for calls in (call.caller, call.callee):
            await calls.create_p2p_call(USER_ID)
        caller_auth, callee_auth = await call.exchange_keys()
        self.assertEqual(caller_auth.key_fingerprint, callee_auth.key_fingerprint)
        self.assertGreaterEqual(len(caller_auth.g_a_or_b), 248)
        self.assertLessEqual(len(caller_auth.g_a_or_b), 256)
        caller_emojis = await call.caller.get_emojis_fingerprint(USER_ID)
        self.assertTrue(caller_emojis)
        self.assertEqual(caller_emojis, await call.callee.get_emojis_fingerprint(USER_ID))
        await call.stop()

    async def test_tampered_key_is_rejected(self):
        call = LocalCall(asyncio.get_running_loop())
        for calls in (call.caller, call.callee):
            await calls.create_p2p_call(USER_ID)
        dh_config = ntgcalls.DhConfig(3, RFC3526_PRIME, os.urandom(256))
        g_a_hash = await call.caller.init_exchange(USER_ID, dh_config, None)
        g_b = await call.callee.init_exchange(USER_ID, dh_config, g_a_hash)
        caller_auth = await call.caller.exchange_keys(USER_ID, g_b, 0)
        with self.assertRaises(ntgcalls.CryptoError):
            await call.callee.exchange_keys(USER_ID, caller_auth.g_a_or_b, caller_auth.key_fingerprint ^ 1)
        await call.stop()


class TestLocalCall(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.loop = asyncio.get_running_loop()
        self.folder = tempfile.mkdtemp()
        self.tone = os.path.join(self.folder, 'tone.raw')
        write_tone(self.tone, 3)
        self.call = LocalCall(self.loop)

    async def asyncTearDown(self):
        await self.call.stop()

    async def test_connects(self):
        await self.call.connect()
        self.assertEqual(await self.call.caller.get_connection_mode(USER_ID), ntgcalls.ConnectionMode.RTC)

    async def test_audio_reaches_the_other_side(self):
        received = bytearray()
        enough = asyncio.Event()

        def on_frames(chat_id, mode, device, frames):
            if mode == ntgcalls.StreamMode.PLAYBACK and device == ntgcalls.StreamDevice.MICROPHONE:
                for frame in frames:
                    received.extend(frame.data)
                if len(received) > SAMPLE_RATE * CHANNELS * 2:
                    self.loop.call_soon_threadsafe(enough.set)

        self.call.caller.on_frames(on_frames)
        await self.call.connect(callee_media=pcm_input(self.tone))
        await self.call.caller.set_stream_sources(USER_ID, ntgcalls.StreamMode.PLAYBACK, ntgcalls.MediaDescription(
            microphone=ntgcalls.AudioDescription(ntgcalls.MediaSource.EXTERNAL, SAMPLE_RATE, CHANNELS, '', False, format=ntgcalls.Format.PCM),
        ))
        await asyncio.wait_for(enough.wait(), TIMEOUT)
        self.assertGreater(rms(bytes(received[-SAMPLE_RATE * CHANNELS:])), 1000)

    async def test_remote_mute(self):
        states = asyncio.Queue()
        self.call.caller.on_remote_source_change(lambda chat_id, source: source.device == ntgcalls.StreamDevice.MICROPHONE and self.loop.call_soon_threadsafe(states.put_nowait, source.state))
        await self.call.connect(callee_media=pcm_input(self.tone))
        for action, expected in ((self.call.callee.mute, ntgcalls.StreamStatus.IDLING), (self.call.callee.unmute, ntgcalls.StreamStatus.ACTIVE)):
            await action(USER_ID)
            while (state := await asyncio.wait_for(states.get(), TIMEOUT)) != expected:
                pass
            self.assertEqual(state, expected)

    async def test_stream_end(self):
        ended = asyncio.Event()
        self.call.callee.on_stream_end(lambda chat_id, stream_type, device: device == ntgcalls.StreamDevice.MICROPHONE and self.loop.call_soon_threadsafe(ended.set))
        await self.call.connect(callee_media=pcm_input(self.tone))
        await asyncio.wait_for(ended.wait(), TIMEOUT)

    async def test_records_opus_without_reencoding(self):
        output = os.path.join(self.folder, 'record.ogg')
        ended = asyncio.Event()
        self.call.callee.on_stream_end(lambda chat_id, stream_type, device: self.loop.call_soon_threadsafe(ended.set))
        await self.call.connect(callee_media=pcm_input(self.tone), caller_playback=ntgcalls.MediaDescription(
            microphone=ntgcalls.AudioDescription(ntgcalls.MediaSource.FILE, SAMPLE_RATE, CHANNELS, output, False, format=ntgcalls.Format.OGG),
        ))
        await asyncio.wait_for(ended.wait(), TIMEOUT)
        await asyncio.sleep(1)
        await self.call.caller.stop(USER_ID)
        with open(output, 'rb') as f:
            data = f.read()
        packets = ogg_packets(data)
        self.assertTrue(packets[0].startswith(b'OpusHead'))
        self.assertTrue(packets[1].startswith(b'OpusTags'))
        self.assertGreater(sum(opus_duration_ms(packet) for packet in packets[2:]), 2500)


if __name__ == '__main__':
    unittest.main()
