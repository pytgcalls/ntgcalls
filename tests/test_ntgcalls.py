import unittest
from ntgcalls import NTgCalls, ConnectionNotFound


class TestNTgCalls(unittest.IsolatedAsyncioTestCase):
    async def test_ping(self):
        result = NTgCalls.ping()
        self.assertEqual(result, "pong")

    async def test_get_protocol(self):
        protocol = NTgCalls.get_protocol()
        self.assertIsNotNone(protocol)
        self.assertTrue(hasattr(protocol, "min_layer"))
        self.assertTrue(hasattr(protocol, "max_layer"))

    async def test_get_media_devices(self):
        devices = NTgCalls.get_media_devices()
        self.assertIsNotNone(devices)

    async def test_instance_methods(self):
        client = NTgCalls()
        usage = await client.cpu_usage()
        self.assertIsInstance(usage, float)
        active_calls = await client.calls()
        self.assertIsInstance(active_calls, dict)

    async def test_invalid_chat_exception(self):
        client = NTgCalls()
        with self.assertRaises(ConnectionNotFound):
            await client.get_state(999999)


if __name__ == "__main__":
    unittest.main()