from aiohttp import web
from rtcbot import getRTCBotJS, RTCConnection, CVCamera, Microphone, DelayedSubscription

routes = web.RouteTableDef()
import logging

# logging.basicConfig(level=logging.DEBUG)


@routes.get("/rtcbot.js")
async def rtcbotJS(request):
    return web.Response(content_type="text/javascript", text=getRTCBotJS())


@routes.get("/")
async def index(request):
    with open("camera.html", "r") as f:
        return web.Response(content_type="text/html", text=f.read())


@routes.post("/setupRTC")
async def setupRTC(request):
    clientOffer = await request.json()
    conn = RTCConnection()

    conn.addAudio(DelayedSubscription(mic))
    conn.addVideo(cam.subscribe())

    response = await conn.getLocalDescription(clientOffer)
    return web.json_response(response)


cam = CVCamera()
mic = Microphone()
app = web.Application()
app.add_routes(routes)
web.run_app(app, port=8000)
