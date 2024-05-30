from pymodbus.client import AsyncModbusSerialClient
import time


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

async def init():
    await vfd_9600.connect()

async def close():
    await vfd_9600.close()

async def read_reg(address):
    return (await vfd_9600.read_holding_registers(address, count= 1, slave= add)).registers

async def write_reg(address, value):
    #does not 
    await vfd_9600.write_register(address, value=value, slave=add, timeout=0)
    return #await read_reg(address)