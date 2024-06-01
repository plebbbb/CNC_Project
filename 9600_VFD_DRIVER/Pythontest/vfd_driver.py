#!/usr/bin/python3
from pymodbus.client import AsyncModbusSerialClient
import asyncio
from enum import IntEnum
import hal
import time

#Note only this file is correctly updated

vfd_9600 = AsyncModbusSerialClient(
    port = "/dev/ttyS0", 
    baudrate = 19200, 
    bytesize = 8, 
    parity = 'E', 
    stopbits = 1, 
    strict = True,
    timeout = 0.01
)

add = 1
delay_per = 0.1
async def init():
    await vfd_9600.connect()

async def close():
    await vfd_9600.close()

async def read_reg(address):
    await asyncio.sleep(delay_per)
    return (await vfd_9600.read_holding_registers(address, count= 1, slave= add)).registers[0]

async def write_reg(address, value):
    await asyncio.sleep(delay_per)
    #the 9600 doesn't seem to send proper return values? so we use try-catch here. 
    try:
        await vfd_9600.write_register(address, value=value, slave=add, timeout=0)
    except: 
        pass
    return



hz_max = 400
perc_scaler = 10000

class vfd_speedmode(IntEnum):
    Forward = 1
    Stop = 6

mode_states = [
    6,
    1
]

async def setmode(enable):
    await write_reg(0x2000, mode_states[int(enable)])

async def setspeed_rpm(rpm):
    freq = abs(rpm)/60
    await setspeed_hz(freq)

#integer units of 0.01%, 10000 = 100%
async def setspeed_hz(freq):
    perc = int(min(abs(freq), hz_max)/hz_max * perc_scaler)
    await write_reg(0x1000, perc)
async def readcurrent():
    current_cA = await read_reg(0x7004)
    return current_cA/100

#output wattage is in units of 100w
async def readpower():
    power_hW = await read_reg(0x7005) 
    return power_hW*100

async def readfault():
    fault = await read_reg(0x703E) 
    return fault

#output voltage is in units of 1v
async def readvoltage():
    voltage_V = await read_reg(0x7003) 
    return voltage_V

#output current is in units of 0.01hz
#first is rpm, second is hz
async def readspeed():
    speed_chz = await read_reg(0x7000)
    return [speed_chz * 60 / 100, speed_chz/100]


h = hal.component("vfd_driver")
h.newpin("cmd_speed_hz", hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("at_target_speed", hal.HAL_BIT, hal.HAL_OUT)
h.newpin("enable", hal.HAL_BIT, hal.HAL_IN)
h.newpin("actual_speed_hz_abs", hal.HAL_FLOAT, hal.HAL_OUT)
h.newpin("actual_speed_rpm_abs", hal.HAL_FLOAT, hal.HAL_OUT)
h.newpin("error_count", hal.HAL_S32, hal.HAL_OUT)
h.newpin("fault_code", hal.HAL_S32, hal.HAL_OUT)

h.newpin("output_power_w", hal.HAL_FLOAT, hal.HAL_OUT)
h.newpin("output_voltage_v", hal.HAL_FLOAT, hal.HAL_OUT)
h.newpin("output_current_a", hal.HAL_FLOAT, hal.HAL_OUT)
h.ready()

time.sleep(10)
at_speed_threshold = 5

async def run():
    await init()
    try:
        while(True):
            try:
                await setmode(h.enable)
                tgt = h.cmd_speed_hz
                await setspeed_hz(tgt)
                h.output_current_a = await readcurrent()
                h.output_power_w = await readpower()
                h.output_voltage_v = await readvoltage()
                h.fault_code = await readfault()
                tmp = await readspeed()
                h.actual_speed_rpm_abs = tmp[0]
                h.actual_speed_hz_abs = tmp[1]
                h.at_target_speed = ((abs(tgt-tmp[1]) <= at_speed_threshold))
                await asyncio.sleep(2)
            except Exception as e:
                print(e)
                h.error_count+=1
                pass
    except KeyboardInterrupt:
        await close()
        raise SystemExit
asyncio.run(run())
