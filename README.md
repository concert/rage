# Realtime Audio Graph Engine

Roughly this library is divided into 2 parts, "elements" and a host.
Elements do all the audio processing, the host tries to make their job easy.

## Elements

In contrast to LV2 plugins elements may have initialisation parameters from
which they can generate their runtime interfaces.

Elements have 2 "play time" threads, one that runs in a "soft" context (which
is allowed to do IO), and another that runs in a stricter real time context
(and isn't allowed to do anything RT hazardous).

Both threads of an element are provided with an identical sample accurate
interpolated view of the time series of the element instance's parameters. The
"soft" thread runs first in order to prepare a buffer of data for the RT
thread. This preparation reports how many frames may be consumed before it is
called again (allowing for efficient, batched, interleaved scheduling).

This buffer may be arbitrarily long in session time. For instance a file
playing (persistance) element may know it doesn't need to play anything for 10
minutes, but can buffers up the first second of audio it would need at that
point in the future to minimise context switches.

In order for parameter changes to be applied in a somewhat timely manner,
without leaving the RT thread short a mechanism to (potentially partially)
rewind the prepared data in this buffer is used.

## Host

The job of the host is to manage the scheduling of the element hooks
efficiently and to expose out a simpler, asynchronous, interface for managing
elements and changing timeseries. The host also contains a loader which allows
elements to be loaded dynamically, at some point the intention is to make a
loader that wraps LV2 in order to support existing plugins.

## Hopes, dreams and pet peeve avoidance

Wishlist (that I believe this architecture will allow):
- Different sections of the audio flow graph to run at different period sizes
  (e.g. prerecorded segments run in large periods whilst live audio processing
  runs with lower latency).
- Arbitrarily placed bounces (since a persistance element can be put anywhere,
  even half way through what would traditionally be a "plugin rack").
- Automatic static analysis and testing (especially of elements), since the
  functions that should be RT safe are explicitly known and the parameters are
  introspectable and some sane output invariants can be chosen allowing some
  basic "it doesn't crash or pop" type tests can be generated.

Things this isn't:
- Traditional "saving" plugin host. All state is ephemeral within the library,
  a persisting mechanism can be implemented in a layer above.
- Supporting traditional "local UI" for elements, I think this will end up
  being more trouble than it is worth as non-local (or just non keyboard/mouse)
  control surfaces become more prevalent.
