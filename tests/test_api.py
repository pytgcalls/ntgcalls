import json
import os
import tempfile
import unittest

import ntgcalls

CHAT_ID = -1001234567890


class TestStatic(unittest.TestCase):
    def test_ping(self):
        self.assertEqual(ntgcalls.NTgCalls.ping(), 'pong')

    def test_protocol(self):
        protocol = ntgcalls.NTgCalls.get_protocol()
        self.assertEqual(protocol.library_versions, ['8.0.0', '9.0.0', '12.0.0', '13.0.0'])
        self.assertEqual((protocol.min_layer, protocol.max_layer), (92, 92))
        self.assertTrue(protocol.udp_p2p)
        self.assertTrue(protocol.udp_reflector)

    def test_media_devices(self):
        devices = ntgcalls.NTgCalls.get_media_devices()
        for kind in ('microphone', 'speaker', 'camera', 'screen'):
            for device in getattr(devices, kind):
                self.assertIsInstance(device.name, str)
                self.assertIsInstance(device.metadata, str)

    def test_description_defaults(self):
        audio = ntgcalls.AudioDescription(ntgcalls.MediaSource.FILE, 48000, 2, 'input.ogg', False)
        self.assertEqual((audio.sample_rate, audio.channel_count, audio.input), (48000, 2, 'input.ogg'))
        self.assertIsNone(audio.format)
        self.assertIsNone(audio.bitrate)
        self.assertIsNone(audio.max_delay_ms)
        video = ntgcalls.VideoDescription(ntgcalls.MediaSource.FILE, 1280, 720, 30, 'input.mkv', False, format=ntgcalls.Format.I420)
        self.assertEqual((video.width, video.height, video.fps), (1280, 720, 30))
        self.assertEqual(video.format, ntgcalls.Format.I420)


class TestGroupCall(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.calls = ntgcalls.NTgCalls()

    async def asyncTearDown(self):
        await self.calls.stop(CHAT_ID)

    async def test_join_params(self):
        params = json.loads(await self.calls.create_call(CHAT_ID))
        self.assertTrue(params['ufrag'])
        self.assertTrue(params['pwd'])
        self.assertEqual(params['fingerprints'][0]['hash'], 'sha-256')
        self.assertTrue(params['ssrc'])

    async def test_call_registry(self):
        await self.calls.create_call(CHAT_ID)
        self.assertIn(CHAT_ID, await self.calls.calls())
        self.assertEqual(await self.calls.get_call_type(CHAT_ID), ntgcalls.CallType.GROUP)
        self.assertEqual(await self.calls.get_connection_mode(CHAT_ID), ntgcalls.ConnectionMode.NONE)
        await self.calls.stop(CHAT_ID)
        self.assertNotIn(CHAT_ID, await self.calls.calls())

    async def test_duplicate_call(self):
        await self.calls.create_call(CHAT_ID)
        with self.assertRaises(ntgcalls.ConnectionError):
            await self.calls.create_call(CHAT_ID)

    async def test_unknown_call(self):
        with self.assertRaises(ntgcalls.ConnectionNotFound):
            await self.calls.mute(CHAT_ID)
        with self.assertRaises(ntgcalls.ConnectionNotFound):
            await self.calls.set_stream_sources(CHAT_ID, ntgcalls.StreamMode.CAPTURE, ntgcalls.MediaDescription())

    async def test_invalid_join_response(self):
        await self.calls.create_call(CHAT_ID)
        with self.assertRaises(ntgcalls.TransportParseException):
            await self.calls.connect(CHAT_ID, 'not json', False)
        with self.assertRaises(ntgcalls.TransportParseException):
            await self.calls.connect(CHAT_ID, '{}', False)

    async def test_invalid_formats(self):
        await self.calls.create_call(CHAT_ID)
        with self.assertRaises(ntgcalls.InvalidParams):
            await self.calls.set_stream_sources(CHAT_ID, ntgcalls.StreamMode.CAPTURE, ntgcalls.MediaDescription(
                microphone=ntgcalls.AudioDescription(ntgcalls.MediaSource.FILE, 48000, 2, 'input.mkv', False, format=ntgcalls.Format.MATROSKA),
            ))
        with self.assertRaises(ntgcalls.InvalidParams):
            await self.calls.set_stream_sources(CHAT_ID, ntgcalls.StreamMode.CAPTURE, ntgcalls.MediaDescription(
                camera=ntgcalls.VideoDescription(ntgcalls.MediaSource.FILE, 1280, 720, 30, 'input.raw', False, format=ntgcalls.Format.PCM),
            ))
        with self.assertRaises(ntgcalls.InvalidParams):
            await self.calls.set_stream_sources(CHAT_ID, ntgcalls.StreamMode.PLAYBACK, ntgcalls.MediaDescription(
                camera=ntgcalls.VideoDescription(ntgcalls.MediaSource.FILE, 0, 0, 0, 'output.mkv', False),
            ))

    async def test_missing_file(self):
        await self.calls.create_call(CHAT_ID)
        path = os.path.join(tempfile.gettempdir(), 'ntgcalls-missing-input.raw')
        with self.assertRaises(ntgcalls.FileError):
            await self.calls.set_stream_sources(CHAT_ID, ntgcalls.StreamMode.CAPTURE, ntgcalls.MediaDescription(
                microphone=ntgcalls.AudioDescription(ntgcalls.MediaSource.FILE, 48000, 2, path, False, format=ntgcalls.Format.PCM),
            ))

    async def test_controls_before_connecting(self):
        await self.calls.create_call(CHAT_ID)
        self.assertTrue(await self.calls.mute(CHAT_ID))
        self.assertTrue(await self.calls.unmute(CHAT_ID))
        self.assertEqual(await self.calls.time(CHAT_ID, ntgcalls.StreamMode.CAPTURE), 0)

    async def test_incoming_video_without_ssrcs(self):
        await self.calls.create_call(CHAT_ID)
        self.assertEqual(await self.calls.add_incoming_video(CHAT_ID, 1, 'empty', []), 0)
        self.assertEqual(await self.calls.add_incoming_video(CHAT_ID, 1, 'empty-groups', [ntgcalls.SsrcGroup('SIM', []), ntgcalls.SsrcGroup('FID', [])]), 0)
        self.assertEqual(await self.calls.add_incoming_video(CHAT_ID, 2, 'single', [ntgcalls.SsrcGroup('FID', [31, 32])]), 31)
        self.assertEqual(await self.calls.add_incoming_video(CHAT_ID, 3, 'simulcast', [ntgcalls.SsrcGroup('SIM', [41, 42, 43]), ntgcalls.SsrcGroup('FID', [41, 44])]), 41)


class TestPrivateCall(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.calls = ntgcalls.NTgCalls()
        await self.calls.create_p2p_call(1)

    async def asyncTearDown(self):
        await self.calls.stop(1)

    async def test_call_type(self):
        self.assertEqual(await self.calls.get_call_type(1), ntgcalls.CallType.P2P)

    async def test_connect_without_key(self):
        with self.assertRaises(ntgcalls.ConnectionNotFound):
            await self.calls.connect_p2p(1, [], ['8.0.0'], True, None)

    async def test_unsupported_version(self):
        await self.calls.skip_exchange(1, bytes(256), True)
        with self.assertRaises(ntgcalls.SignalingUnsupported):
            await self.calls.connect_p2p(1, [], ['1.0.0'], True, None)

    async def test_invalid_dh_config(self):
        for g, p in ((2, b'\x07'), (1, b'\xff' * 256), (9, b'\xff' * 256)):
            with self.assertRaises(ntgcalls.InvalidParams):
                await self.calls.init_exchange(1, ntgcalls.DhConfig(g, p, os.urandom(256)), None)


if __name__ == '__main__':
    unittest.main()
