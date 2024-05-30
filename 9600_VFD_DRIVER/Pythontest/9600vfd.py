import asyncio
import vfd_interface


async def run():
    await vfd_interface.init()

    while(True):
        try: 
            await vfd_interface.write_reg(0x2000, 0)
        except:
            pass
        await asyncio.sleep(1)

        try: 
            await vfd_interface.write_reg(0x1000, 0)
        except:
            pass

        await asyncio.sleep(30)


asyncio.run(run())
