import asyncio
import vfd9600
import hal

h = hal.component("vfd")
h.newpin("cmd_speed_hz", hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("at_target_speed", hal.HAL_BIT, hal.HAL_OUT)
h.newpin("enable", hal.HAL_BIT, hal.HAL_IN)
h.newpin("actual_speed_hz_abs", hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("actual_speed_rpm_abs", hal.HAL_FLOAT, hal.HAL_IN)

h.newpin("output_power_w", hal.HAL_FLOAT, hal.HAL_OUT)
h.newpin("output_current_a", hal.HAL_FLOAT, hal.HAL_OUT)
h.ready()

at_speed_threshold = 0.05

async def run():
    await vfd9600.init()
    try:
        while(True):
            await vfd9600.setmode(h.enable.get())
            tgt = await vfd9600.readcurrent()
            await vfd9600.setspeed_hz(h.cmd_speed_hz.get())
            h.output_current_a.set(tgt)
            h.output_power_w.set(await vfd9600.readpower())
            tmp = await vfd9600.readspeed()
            h.actual_speed_rpm_abs.set(tmp[0])
            h.actual_speed_hz_abs.set(tmp[1])
            h.at_target_speed.set((abs(tgt-tmp[1])/tgt <= at_speed_threshold))
            await asyncio.sleep(1)

    except KeyboardInterrupt:
        await vfd9600.close()
        raise SystemExit
asyncio.run(run())
