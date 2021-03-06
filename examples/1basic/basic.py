from aiohttp import web
from rtcbot import RTCConnection

routes = web.RouteTableDef()


@routes.get("/")
async def index(request):
    with open("basic.html", "r") as f:
        return web.Response(content_type="text/html", text=f.read())


@routes.post("/setupRTC")
async def setupRTC(request):
    clientOffer = await request.json()
    conn = RTCConnection()

    @conn.onMessage
    def onMsg(c, m):
        print("Message:", m, "from", c.label)
        c.send(m)

    response = await conn.getLocalDescription(clientOffer)
    return web.json_response(response)


routes.static("/rtcbot/", path="./rtcbot")
app = web.Application()
app.add_routes(routes)
web.run_app(app, port=8000)
