Realtime Audio Graph Engine

Roughly this library is divided into 2 parts, "elements" and a host.

Elements have parameterisable initialisation and expose their interfaces at
runtime via structs.

Elements have 2 "play time" threads, one that runs in a "soft" context (is
allowed to do IO), and another that runs in a stricter real time context (and
isn't allowed to do anything RT hazardous). Communication between these is left
to the element author to determine.

Both threads of an element are provided with an identical sample accurate
interpolated view of the elements time series of parameters. The "soft" thread
runs first in order to prepare a buffer of data for the RT thread. This
preparation reports how many frames may be consumed before it is called again
(allowing for efficient, batched, interleaved scheduling).

This buffer may be arbitrarily long in session time. For instance a file
playing (persistance) element may know it doesn't need to play anything for 10
mins, but buffers up the first second of audio it would need at that point in
the future to minimise context switches. Therefore in order for parameter
changes to be applied in a somewhat timely manner, without leaving the RT
thread short a mechanism to (potentially partially) rewind this buffer is
used.

This makes the job of the host to manage the scheduling of the element hooks
efficiently and to expose out a simpler, albeit asynchronous, interface for
changing timeseries. The host also contains a loader which allows elements to
be loaded dynamically, at some point the intention is to wrap over LV2 in order
to support existing plugins.

Wishlist (that I believe this architecture will allow):
- Different sections of the audio flow graph to run at different period sizes
  (e.g. prerecorded segments run in large periods).
- Arbitrarily placed bounces (since a persistance element can be put anywhere,
  even half way through what would traditionally be a "plugin rack".
- Automatic static analysis and testing (especially of elements), since the
  functions that should be RT safe are explicitly known and the parameters are
  introspectable and therefore test configurations can be generated.

Things this isn't:
- Traditional "saving" plugin host. All state is ephemeral within the library,
  a persisting mechanism should be a layer above.
- Supporting traditional "local UI" for elements, I think this will end up
  being more trouble than it is worth as non-local control surfaces become more
  prevalent.
