from pymodbus.client import AsyncModbusSerialClient
import asyncio


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