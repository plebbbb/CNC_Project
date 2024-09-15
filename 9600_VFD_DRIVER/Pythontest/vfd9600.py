import asyncio
import vfd_interface
from enum import IntEnum

hz_max = 400
perc_scaler = 10000

class vfd_speedmode(IntEnum):
    Forward = 1
    Stop = 6

mode_states = [
    6,
    1
]

async def setmode(enable: bool):
    await vfd_interface.write_reg(0x2000, mode_states[int(enable)])

async def setspeed_rpm(rpm):
    freq = abs(rpm)/60
    await setspeed_hz(freq)

#integer units of 0.01%, 10000 = 100%
async def setspeed_hz(freq):
    perc = int(min(abs(freq), hz_max)/hz_max * perc_scaler)
    await vfd_interface.write_reg(0x1000, perc)
async def readcurrent():
    current_cA = await vfd_interface.read_reg(0x7004)
    return current_cA/100

#output wattage is in units of 100w
async def readpower():
    power_hW = await vfd_interface.read_reg(0x7005) 
    return power_hW*100
#output current is in units of 0.01hz
#first is rpm, second is hz
async def readspeed():
    speed_chz = await vfd_interface.read_reg(0x7000)
    return [speed_chz * 60 / 100, speed_chz/100]

async def init():
    await vfd_interface.init()

async def close():
    await vfd_interface.close()