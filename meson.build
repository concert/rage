project('rage', 'c')

add_global_arguments('-std=gnu11', '-Werror', '-fms-extensions', language: 'c')

# Language Extensions

langext_inc = include_directories('langext/include')

langext_sources = [
    'langext/src/countdown.c',
    'langext/src/finaliser.c'
]

sem = dependency('threads')

langext = shared_library(
    'rage_langext', langext_sources, include_directories: langext_inc,
    dependencies: [sem])

test_errors = executable(
    'test_errors', 'langext/test/test_errors.c', include_directories: [langext_inc],
    link_with: langext)
test('errors', test_errors)
test_langext = executable(
    'test_langext', 'langext/test/main.c',
    include_directories: [langext_inc], link_with: langext, dependencies: [sem])
test('langext', test_langext)

# Types

types_inc = include_directories('types/include')

sources = [
    'types/src/ports.c',
    'types/src/interpolation.c',
    'types/src/time.c',
    'types/src/test_factories.c',
    'types/src/atoms.c'
]

rage_types = shared_library(
    'rage_types', sources, include_directories: [types_inc, langext_inc],
    link_with: langext, dependencies: [sem])

# FIXME: this proliferation of test executables is getting silly
test_interpolation = executable(
    'test_interpolation', 'types/test/test_interpolation.c',
    include_directories: [types_inc, langext_inc], link_with: [rage_types, langext])
test_time = executable(
    'test_time', 'types/test/test_time.c', include_directories: [types_inc, langext_inc],
    link_with: [rage_types, langext])
test_ports = executable(
    'test_ports', ['types/test/test_ports.c'], include_directories: [types_inc, langext_inc],
    link_with: [rage_types, langext])
test_factories = executable(
    'test_factories', ['types/test/test_test_factories.c'], include_directories: [types_inc, langext_inc],
    link_with: [rage_types, langext])
test('interpolation', test_interpolation)
test('time', test_time)
test('ports', test_ports)
test('factories', test_factories)

# Graph

cc = meson.get_compiler('c')
dyn_load = cc.find_library('dl')

jack = dependency('jack')

graph_inc = include_directories('graph/include')

rage_graph = shared_library(
    'rage_graph',
    [
        'graph/src/loader.c', 'graph/src/jack_bindings.c', 'graph/src/srt.c',
        'graph/src/proc_block.c', 'graph/src/graph.c'
    ],
    include_directories: [graph_inc, types_inc, langext_inc],
    dependencies: [dyn_load, jack],
    link_with: [langext, rage_types])

test_graph = executable(
    'test_graph', 'graph/test/main.c',
    include_directories: [graph_inc, types_inc, langext_inc],
    dependencies: [sem],
    link_with: [langext, rage_types, rage_graph])
test('graph', test_graph)

# Elements

element_incs = [langext_inc, types_inc]

amp_sources = ['elements/amp/amp.c']
amp = shared_library(
    'amp', amp_sources, include_directories: element_incs, link_with: rage_types)

sndfile = dependency('sndfile')

persist_sources = ['elements/persistence/persistence.c']
persist = shared_library(
    'persist', persist_sources, include_directories: element_incs,
    link_with: rage_types, dependencies: [jack, sndfile])

# Example

example_sources = ['example/main.c']
example = executable(
    'example', example_sources, include_directories: [langext_inc, types_inc, graph_inc],
    link_with: [langext, rage_types, rage_graph], dependencies: [jack])
