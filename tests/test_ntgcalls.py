import unittest
from ntgcalls import NTgCalls


class TestNTgCalls(unittest.IsolatedAsyncioTestCase):
    async def test_ping(self):
        result = NTgCalls().ping()
        self.assertEqual(result, "pong")
        self.assertIsNotNone(result)


if __name__ == "__main__":
    unittest.main()