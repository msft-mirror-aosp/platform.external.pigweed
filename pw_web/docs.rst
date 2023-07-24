.. _module-pw_web:

---------
pw_web
---------

Pigweed provides an NPM package with modules to build web apps for Pigweed
devices.

Also included is a basic React app that demonstrates using the npm package.

Getting Started
===============

Installation
-------------
If you have a bundler set up, you can install ``pigweedjs`` in your web application by:

.. code:: bash

   $ npm install --save pigweedjs


After installing, you can import modules from ``pigweedjs`` in this way:

.. code:: javascript

   import { pw_rpc, pw_tokenizer, Device, WebSerial } from 'pigweedjs';

Import Directly in HTML
^^^^^^^^^^^^^^^^^^^^^^^

If you don't want to set up a bundler, you can also load Pigweed directly in
your HTML page by:

.. code:: html

   <script src="https://unpkg.com/pigweedjs/dist/index.umd.js"></script>
   <script>
     const { pw_rpc, pw_hdlc, Device, WebSerial } from Pigweed;
   </script>

Getting Started
---------------
Easiest way to get started is to build pw_system demo and run it on a STM32F429I
Discovery board. Discovery board is Pigweed's primary target for development.
Refer to :ref:`target documentation<target-stm32f429i-disc1-stm32cube>` for
instructions on how to build the demo and try things out.

``pigweedjs`` provides a ``Device`` API which simplifies common tasks. Here is
an example to connect to device and call ``EchoService.Echo`` RPC service.

.. code:: html

   <h1>Hello Pigweed</h1>
   <button onclick="connect()">Connect</button>
   <button onclick="echo()">Echo RPC</button>
   <br /><br />
   <code></code>
   <script src="https://unpkg.com/pigweedjs/dist/index.umd.js"></script>
   <script src="https://unpkg.com/pigweedjs/dist/protos/collection.umd.js"></script>
   <script>
     const { Device } = Pigweed;
     const { ProtoCollection } = PigweedProtoCollection;

     const device = new Device(new ProtoCollection());

     async function connect(){
       await device.connect();
     }

     async function echo(){
       const [status, response] = await device.rpcs.pw.rpc.EchoService.Echo("Hello");
       document.querySelector('code').innerText = "Response: " + response;
     }
   </script>

pw_system demo uses ``pw_log_rpc``; an RPC-based logging solution. pw_system
also uses pw_tokenizer to tokenize strings and save device space. Below is an
example that streams logs using the ``Device`` API.

.. code:: html

   <h1>Hello Pigweed</h1>
   <button onclick="connect()">Connect</button>
   <br /><br />
   <code></code>
   <script src="https://unpkg.com/pigweedjs/dist/index.umd.js"></script>
   <script src="https://unpkg.com/pigweedjs/dist/protos/collection.umd.js"></script>
   <script>
     const { Device, pw_tokenizer } = Pigweed;
     const { ProtoCollection } = PigweedProtoCollection;
     const tokenDBCsv = `...` // Load token database here

     const device = new Device(new ProtoCollection());
     const detokenizer = new pw_tokenizer.Detokenizer(tokenDBCsv);

     async function connect(){
       await device.connect();
       const call = device.rpcs.pw.log.Logs.Listen((msg) => {
         msg.getEntriesList().forEach((entry) => {
           const frame = entry.getMessage();
           const detokenized = detokenizer.detokenizeUint8Array(frame);
           document.querySelector('code').innerHTML += detokenized + "<br/>";
         });
       })
     }
   </script>

The above example requires a token database in CSV format. You can generate one
from the pw_system's ``.elf`` file by running:

.. code:: bash

   $ pw_tokenizer/py/pw_tokenizer/database.py create \
   --database db.csv out/stm32f429i_disc1_stm32cube.size_optimized/obj/pw_system/bin/system_example.elf

You can then load this CSV in JavaScript using ``fetch()`` or by just copying
the contents into the ``tokenDBCsv`` variable in the above example.

Modules
=======

Device
------
Device class is a helper API to connect to a device over serial and call RPCs
easily.

To initialize device, it needs a ``ProtoCollection`` instance. ``pigweedjs``
includes a default one which you can use to get started, you can also generate
one from your own ``.proto`` files using ``pw_proto_compiler``.

``Device`` goes through all RPC methods in the provided ProtoCollection. For
each RPC, it reads all the fields in ``Request`` proto and generates a
JavaScript function that accepts all the fields as it's arguments. It then makes
this function available under ``rpcs.*`` namespaced by its package name.

Device has following public API:

- ``constructor(ProtoCollection, WebSerialTransport <optional>, rpcAddress <optional>)``
- ``connect()`` - Shows browser's WebSerial connection dialog and let's user
  make device selection
- ``rpcs.*`` - Device API enumerates all RPC services and methods present in the
  provided proto collection and makes them available as callable functions under
  ``rpcs``. Example: If provided proto collection includes Pigweed's Echo
  service ie. ``pw.rpc.EchoService.Echo``, it can be triggered by calling
  ``device.rpcs.pw.rpc.EchoService.Echo("some message")``. The functions return
  a ``Promise`` that resolves an array with status and response.

WebSerialTransport
------------------

To help with connecting to WebSerial and listening for serial data, a helper
class is also included under ``WebSerial.WebSerialTransport``. Here is an
example usage:

.. code:: javascript

   import { WebSerial, pw_hdlc } from 'pigweedjs';

   const transport = new WebSerial.WebSerialTransport();
   const decoder = new pw_hdlc.Decoder();

   // Present device selection prompt to user
   await transport.connect();

   // Or connect to an existing `SerialPort`
   // await transport.connectPort(port);

   // Listen and decode HDLC frames
   transport.chunks.subscribe((item) => {
     const decoded = decoder.process(item);
     for (const frame of decoded) {
       if (frame.address === 1) {
         const decodedLine = new TextDecoder().decode(frame.data);
         console.log(decodedLine);
       }
     }
   });

   // Later, close all streams and close the port.
   transport.disconnect();

Individual Modules
==================
Following Pigweed modules are included in the NPM package:

- `pw_hdlc <https://pigweed.dev/pw_hdlc/#typescript>`_
- `pw_rpc <https://pigweed.dev/pw_rpc/ts/>`_
- `pw_tokenizer <https://pigweed.dev/pw_tokenizer/#typescript>`_
- `pw_transfer <https://pigweed.dev/pw_transfer/#typescript>`_

Web Console
===========

Pigweed includes a web console that demonstrates `pigweedjs` usage in a
React-based web app. Web console includes a log viewer and a REPL that supports
autocomplete. Here's how to run the web console locally:

.. code:: bash

   $ cd pw_web/webconsole
   $ npm install
   $ npm run dev

Log viewer component
====================

The NPM package also includes a log viewer component that can be embedded in any
webapp. The component works with Pigweed's RPC stack out-of-the-box but also
supports defining your own log source.

The component is composed of the component itself and a log source. Here is a
simple example app that uses a mock log source:

.. code:: html

   <div id="log-viewer-container"></div>
   <script src="https://unpkg.com/pigweedjs/dist/logging.umd.js"></script>
   <script>

     const { createLogViewer, MockLogSource } = PigweedLogging;
     const logSource = new MockLogSource();
     const containerEl = document.querySelector(
       '#log-viewer-container'
     );

     let unsubscribe = createLogViewer(logSource, containerEl);
     logSource.start(); // Start producing mock logs

   </script>

The code above will render a working log viewer that just streams mock
log entries.

It also comes with an RPC log source with support for detokenization. Here is an
example app using that:

.. code:: html

   <div id="log-viewer-container"></div>
   <script src="https://unpkg.com/pigweedjs/dist/index.umd.js"></script>
   <script src="https://unpkg.com/pigweedjs/dist/protos/collection.umd.js"></script>
   <script src="https://unpkg.com/pigweedjs/dist/logging.umd.js"></script>
   <script>

     const { Device, pw_tokenizer } = Pigweed;
     const { ProtoCollection } = PigweedProtoCollection;
     const { createLogViewer, PigweedRPCLogSource } = PigweedLogging;

     const device = new Device(new ProtoCollection());
     const logSource = new PigweedRPCLogSource(device, "CSV TOKEN DB HERE");
     const containerEl = document.querySelector(
       '#log-viewer-container'
     );

     let unsubscribe = createLogViewer(logSource, containerEl);

   </script>

Custom Log Source
-----------------

You can define a custom log source that works with the log viewer component by
just extending the abstract `LogSource` class and emitting the `logEntry` events
like this:

.. code:: typescript

  import { LogSource, LogEntry, Severity } from 'pigweedjs/logging';

  export class MockLogSource extends LogSource {
    constructor(){
      super();
      // Do any initializations here
      // ...
      // Then emit logs
      const log1: LogEntry = {

      }
      this.emitEvent('logEntry', {
        severity: Severity.INFO,
        timestamp: new Date(),
        fields: [
          { key: 'severity', value: severity }
          { key: 'timestamp', value: new Date().toISOString() },
          { key: 'source', value: "LEFT SHOE" },
          { key: 'message', value: "Running mode activated." }
        ]
      });
    }
  }

After this, you just need to pass your custom log source object
to `createLogViewer()`. See implementation of
`PigweedRPCLogSource <https://cs.opensource.google/pigweed/pigweed/+/main:ts/logging_source_rpc.ts>`_
for reference.
