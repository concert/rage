# Realtime Audio Graph Engine (RAGE)

This library provides abstract audio processing capabilities. Units of audio
processing can be described by defining "elements" that are executed in a host
"audio engine". The engine routes audio between elements using a flow graph.

We use the term "element" rather than "plugin" because elements in the audio
graph can have a wider processing remit than the pure data transformation
typically associated with the term "plugin". For example, we may want an audio
source in our graph to be produced from a file, so we have to coordinate the
soft realtime constraints of disk IO alongside the hard realtime constraints of
filling the audio buffers.

This library is conceptually divided into two parts: elements and a host engine.
Elements do all the audio processing, while the host engine makes their job
easier.

Some high-level features of our architecture:
- Sample-accurate control data interpolation
- Soft realtime updates of control data
- Dynamic plugin loading


## Elements

In contrast to LV2 plugins, elements may have initialisation parameters from
which they can generate their runtime interfaces.

Elements have two "play time" threads: one that runs in a "soft" context (which
is allowed to do IO), and another that runs in a stricter realtime context
(and isn't allowed to do anything realtime-hazardous).

Both threads of an element are provided with identical sample-accurate
interpolated views of the element instance's parameter series. The
"soft" thread runs first in order to prepare a buffer of data for the realtime
thread. This preparation reports how many frames may be consumed before it is
called again (allowing for efficient, batched, interleaved scheduling).

This buffer may be arbitrarily long in session time. For instance, a [file
playing element](https://github.com/foolswood/rage/blob/master/elements/persistence/persistence.c)
may know it doesn't need to play anything for ten minutes, but can buffer up the
first second of audio it would need in ten minutes' time to minimise
context switches.

When a change in the series of control values is made, the engine has to
substitute the new data in a timely fashion without disrupting the audio
flow. To achieve this, the engine may instruct elements to rewind their prepared
state to a point in time before the new control data is to be applied.


## Host Engine

The host engine manages the scheduling of the element hooks efficiently and
exposes a high-level, asynchronous interface for managing elements and changing
contral data series. The host engine also contains a loader which allows
elements to be loaded dynamically; at some point the intention is to make a
loader that wraps LV2 in order to support existing plugins.


## Hopes, dreams and pet peeve avoidance

Wishlist (that I believe this architecture will allow):
- Different sections of the audio flow graph to run at different period sizes
  (e.g. prerecorded segments run in large periods whilst live audio processing
  runs with lower latency).
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
