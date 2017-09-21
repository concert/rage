# Realtime Audio Graph Engine (RAGE)

[![Build Status](https://travis-ci.org/concert/rage.svg?branch=master)](
    https://travis-ci.org/concert/rage)

RAGE provides an audio processing graph implementation tailored towards
distributed audio workstations.
The audio graph within RAGE is composed of processing _elements_ and connections
between elements which are managed by the RAGE _host_.

Rage processing elements can be both traditional data transforms (effects,
mixers, etc) as well as data producers (samplers, synthesizers, etc) and data
consumers (recorders, analysis plugins). The RAGE host manages the scheduling
of both the low-latency audio commutations and the higher latency subtasks such
as loading a file from disk for sample replay. Additionally, RAGE provides
session time interpolated parameters, freeing individual elements from this
responsibility.


Project goals:
- Easy to synchronise serialisable graph representation.
- Managed low overhead worker threads per-element.
- Sample-accurate control data interpolation.
- Dynamic plugin loading.
- Simple interface to audio plugins.

What RAGE will not do:
- Serialize elements and the element graph.
    RAGE will provide the needed information to implement this functionality, but a
    higher level component built upon RAGE's interface is planned to perform
    this task.
- Provide traditional "local UI" support for elements.
    Traditional in-process LV2/VST/etc GUIs work well for purely local DAWs,
    however they introduce significant complexity and don't immediately suit the
    goals of a cross-platform distributed audio workstation.

## RAGE Elements

The RAGE audio graph is built out of a collection of audio processing _elements_.
Each element provides a series of callbacks which at runtime specify:

- A description of a tuple of type specialisation parameters
- A list of input and output ports
- A description of a tuple of instance time series
- A way to allocate and free element instance state
- An audio callback
- Optional worker thread callbacks

Elements have two "play time" threads: one that runs in a soft-realtime context
(which is allowed to do IO), and another that runs in a stricter, hard-realtime,
context (and isn't allowed to do anything realtime-hazardous).

Both threads of an element are provided with identical sample-accurate
interpolated views of the element instance's parameter series. The "soft"
thread runs first in order to prepare a buffer of data for the realtime thread.
The preparation routine reports how many frames may be consumed before it needs
to be called again (allowing for efficient, batched, interleaved scheduling).

The buffer presented to both threads may be arbitrarily long in session time.
For instance, a [file playing element](https://github.com/foolswood/rage/blob/master/elements/persistence/persistence.c)
may know it doesn't need to play anything for ten minutes, but can buffer up the
first second of audio it will need ten minutes from now and specify that it does
not need to be run until that second of data is in use.
Filling the audio buffer early makes it possible to leave the file player's
"soft" thread inactive, or offline for a prolonged period of time which helps in
minimising context switches.

When playing back a pre-prepared sequence, computing data in advance is an
efficient option. However should this sequence be modified some stale data may
have been created. RAGE is built to make it possible to easily apply these
sorts of control value changes in a timely manner. An (incompletely
implemented) feature of RAGE is the ability to rewind elements to insert
parameter changes if they have pre-computed outputs after the change.

Cleanup operations on this sequence will see what the realtime processing
callback was presented with regardless of any subsequent changes. This helps
with clean up as any buffers filled by the realtime (or preparatory) operations
will be seen in the context of the control sequence that created them.

## RAGE Engine

The RAGE host manages the scheduling of the elements efficiently and exposes a
high-level, asynchronous interface for managing elements and changing control
data series. The host also contains a loader which allows elements to be loaded
dynamically; currently the loader only loads RAGE elements, but wrapping LV2
plugins into RAGE elements is planned for a future release.


## Wishlist

- Different sections of the audio flow graph to run at different period sizes
  (e.g. prerecorded segments run in large periods whilst live audio processing
  runs with lower latency).
- Automatic static analysis and testing (especially of elements), since the
  functions that should be RT safe are explicitly known and the parameters are
  introspectable and some sane output invariants can be chosen allowing some
  basic "it doesn't crash or pop" type tests can be generated.
