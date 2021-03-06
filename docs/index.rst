Welcome to RTCBot's documentation!
====================================

RTCBot's purpose is to provide a set of tutorials and simple modules that help in developing remote-controlled robots in Python, with a focus on the Raspberry Pi.

The tutorials start from a basic connection between a Raspberry Pi and Browser, and encompass
creating a video-streaming robot controlled entirely over a 4G mobile connection,
all the way to a powerful system that offloads complex computation to a desktop PC in real-time.

All communication happens through `WebRTC <https://en.wikipedia.org/wiki/WebRTC>`_, 
using Python 3's asyncio and the wonderful `aiortc <https://aiortc.readthedocs.io/en/latest/index.html>`_ library,
meaning that your robot can be controlled with low latency both from the browser and through Python,
even when it is not connected to your local network.

.. toctree::
   :maxdepth: 2

   examples/index
   API

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
