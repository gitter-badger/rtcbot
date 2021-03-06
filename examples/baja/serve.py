from aiohttp import web
from rtcbot import (
    RTCConnection,
    CVCamera,
    PiCamera,
    Microphone,
    Speaker,
    SerialConnection,
)
import asyncio
import logging

# logging.basicConfig(level=logging.DEBUG)

routes = web.RouteTableDef()

try:
    import picamera

    cam = PiCamera()
except:
    cam = CVCamera()
mic = Microphone()
# s = Speaker()

conn = RTCConnection()
conn.video.putSubscription(cam)
conn.audio.putSubscription(mic)
# conn.audio.subscribe(s)

arduino = SerialConnection(writeFormat="<hhh", writeKeys=["gas", "turn", "rot"])

arduino.putSubscription(conn)

"""
@conn.subscribe
def oncMsg(msg):
    try:
        print(msg)
        print(msg["gas"])
    except Exception as e:
        print(e)
    arduino.put_nowait(msg)
"""


@arduino.subscribe
def onMessage(msg):
    print(msg)


@routes.post("/setupRTC")
async def setupRTC(request):
    clientOffer = await request.json()
    print("Got client offer")
    print(clientOffer)
    response = await conn.getLocalDescription(clientOffer)
    print("RESPONSE")
    print(response)
    return web.json_response(response)


async def closer(app):
    print("Running closer")
    mic.close()
    cam.close()
    await conn.close()
    print("Closed")


app = web.Application()
app.add_routes(routes)
app.on_shutdown.append(closer)
web.run_app(app, port=8000)
