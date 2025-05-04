import asyncio
import struct
import serial
from enum import IntEnum
import hal
import time

obj = serial.Serial(port= "/dev/ttyS1", baudrate=4800, timeout=None)

class coolcommand:
    pumpenabled = False
    report_enabled = True
    aggressive = False

    def get_byteoutput(self):
        return int(0x3C * self.aggressive + 0x1 * self.pumpenabled + 0x2 * self.report_enabled)
    
    def conduct_byte_update(self):
        obj.read_all()
        nv = self.get_byteoutput()
        obj.write(nv.to_bytes(length=1, byteorder="big"))
        time.sleep(1)
        k = int.from_bytes(obj.read(1), "big", signed= False)

        print (k)

        if (k == nv): return True
        else: return False
    
    def read_temp(self):
        temp = self.r


h = hal.component("cooling_driver")
h.newpin("coolant_temp", hal.HAL_FLOAT, hal.HAL_OUT)
h.newpin("feedforward", hal.HAL_BIT, hal.HAL_IN)
h.newpin("aggressive", hal.HAL_BIT, hal.HAL_IN)
h.newpin("safe", hal.HAL_BIT, hal.HAL_OUT)


mod = coolcommand()

async def init():
    h.safe = True
    mod.pumpenabled = True
    mod.conduct_byte_update()
    await asyncio.sleep(0.75)
    mod.pumpenabled = True
    mod.conduct_byte_update()


async def close():
    obj.close()



h.ready()
time.sleep(2.5)
async def run():
    await init()
    try:
        while(True):
            try:
                #temp = struct.unpack(">f", obj.read(4))[0]
                '''   if (mod.pumpenabled != h.feedforward or mod.aggressive  != h.aggressive): 
                    mod.pumpenabled = h.feedforward
                    mod.aggressive = h.aggressive
                    mod.conduct_byte_update()
                    continue
                h.safe = True
             #   if (temp > 40):
            #        h.safe = False
           #     else: h.safe = True
              #  h.coolant_temp = temp
              #  print(temp)*/'''
                await asyncio.sleep(0.25)
            except Exception as e:
                #print(e)
                pass
    except KeyboardInterrupt:
        await close()
        raise SystemExit
    

asyncio.run(run())
