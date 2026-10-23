"""Protocol tests, including genuine Lupa tables when installed. No hardware I/O."""
import struct
import threading
import unittest
from enum import IntEnum
from hal.hal_power import HalPowerControl

encode_en_sequence = HalPowerControl._encode_en_sequence
normalize_en_sequence = HalPowerControl._normalize_en_sequence

def run_en_sequence(power, events, status_type):
    return HalPowerControl.set_en_sequence(power, events)

class Status(IntEnum):
    OK = 0
    FAIL = 1
    TIMEOUT = 2

class FakePower:
    _encode_en_sequence = staticmethod(encode_en_sequence)
    def __init__(self, status=Status.OK, response=None):
        self._hw_lock = threading.RLock()
        self.power_en = [0] * 8
        self.status = status
        self.response = response or [0xA0, 0x2B, 0, 0, 2, 7, 1, 0, 0, 0, 0]
        self.sent = None
    def _spi_exchange64(self, header, payload):
        self.sent = header + payload
        return self.status, self.response

class SequenceTests(unittest.TestCase):
    rows = [[0,1,1,0,0,0,0,0,0], [50,1,1,1,0,0,0,0,0]]
    def test_wire_format(self):
        payload, count = encode_en_sequence(self.rows)
        self.assertEqual(count, 2)
        self.assertEqual(bytes(payload), b'\x01\x02' + struct.pack('<IBIB', 0,3,50,7))
    def test_bounds(self):
        for rows in [[], self.rows * 5, [[1]+[0]*8], [[0]+[0]*7],
                     [[0]+[0]*8,[49]+[1]*8], [[0]+[0]*8,[50.5]+[1]*8],
                     [[0]+[0]*8,[1000001]+[1]*8], [[0,2]+[0]*7],
                     [[float('nan')]+[0]*8], [[True]+[0]*8]]:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                encode_en_sequence(rows)
        self.assertEqual(encode_en_sequence([[0]+[0]*8,[1000000]+[1]*8])[1], 2)
    def test_success_and_partial_execution(self):
        p = FakePower()
        self.assertEqual(run_en_sequence(p, self.rows, Status), Status.OK)
        self.assertEqual(p.power_en, [1,1,1,0,0,0,0,0])
        p = FakePower(Status.FAIL, [0xA0,0x2B,1,6,1,3,1,7,0,0,0])
        self.assertEqual(run_en_sequence(p, self.rows, Status), Status.FAIL)
        self.assertEqual(p.power_en, [1,1,0,0,0,0,0,0])
        self.assertEqual(p.last_en_sequence['max_lateness_us'], 7)
    def test_incomplete_and_stale_responses(self):
        for response in [[0xA0,0x14]+[0]*62, [0xA0,0x2B,0,0,1,3,1,0,0,0,0]]:
            p = FakePower(response=response)
            self.assertEqual(run_en_sequence(p, self.rows, Status), Status.FAIL)
        p = FakePower(Status.TIMEOUT)
        p.response = []
        self.assertEqual(run_en_sequence(p, self.rows, Status), Status.TIMEOUT)
        self.assertEqual(p.power_en, [None]*8)
    def test_lua_tables(self):
        try:
            from lupa import LuaRuntime
        except ImportError:
            self.skipTest('Lupa not installed')
        lua = LuaRuntime(unpack_returned_tuples=True)
        table = lua.eval('{{0,1,1,0,0,0,0,0,0},{50,1,1,1,0,0,0,0,0}}')
        self.assertEqual(normalize_en_sequence(table), self.rows)
        with self.assertRaises(ValueError):
            normalize_en_sequence(lua.eval('{[1]={0,0,0,0,0,0,0,0,0},[3]={50,1,1,1,1,1,1,1,1}}'))
        p = FakePower()
        lua.globals().run = lambda events: int(run_en_sequence(p, events, Status))
        lua.execute('assert(run({{0,1,1,0,0,0,0,0,0},{50,1,1,1,0,0,0,0,0}})==0)')

if __name__ == '__main__':
    unittest.main()
