project('rage', 'c')

add_global_arguments('-std=gnu11', '-Werror', '-fms-extensions', language: 'c')

# Language Extensions

langext_inc = include_directories('langext/include')

langext_sources = [
    'langext/src/countdown.c',
    'langext/src/finaliser.c'
]

sem = dependency('threads')

langext = static_library(
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
    'types/src/chronology.c',
    'types/src/test_factories.c',
    'types/src/atoms.c'
]

rage_types = library(
    'rage_types',
    sources,
    include_directories: [types_inc, langext_inc],
    link_with: langext,
    dependencies: [sem],
    install: true)

test_types = executable(
    'test_types', 'types/test/test_main.c',
    include_directories: [types_inc, langext_inc], link_with: [rage_types, langext])
test('types', test_types)

# Graph

cc = meson.get_compiler('c')
dyn_load = cc.find_library('dl')

jack = dependency('jack')

graph_inc = include_directories('graph/include')

# TODO: work out how to thin this down?
install_headers(
    'haskwrap/include/wrappers.h',
    'graph/include/graph.h', 'graph/include/loader.h',
    'types/include/atoms.h', 'types/include/element.h',
    'types/include/ports.h', 'types/include/interpolation.h',
    'types/include/chronology.h', 'types/include/transport_state.h',
    'types/include/time_series.h',
    'langext/include/macros.h', 'langext/include/error.h',
    'langext/include/finaliser.h',
    subdir: 'rage')

rage_graph = library(
    'rage_graph',
    [
        'graph/src/loader.c', 'graph/src/jack_bindings.c', 'graph/src/srt.c',
        'graph/src/proc_block.c', 'graph/src/graph.c'
    ],
    include_directories: [graph_inc, types_inc, langext_inc],
    dependencies: [dyn_load, jack, sem],
    link_with: [langext, rage_types],
    install: true)

graph_test_factories = library(
    'rage_graph_test_factories',
    ['graph/src/graph_test_factories.c'],
    include_directories: [graph_inc, types_inc, langext_inc],
    link_with: [langext, rage_types, rage_graph])

test_graph = executable(
    'test_graph', 'graph/test/main.c',
    include_directories: [graph_inc, types_inc, langext_inc],
    dependencies: [sem],
    link_with: [langext, rage_types, rage_graph, graph_test_factories])
test('graph', test_graph)

# Elements

element_incs = [langext_inc, types_inc]
element_install_dir = join_paths(get_option('datadir'), 'rage', 'elements')

amp_sources = ['elements/amp/amp.c']
amp = library(
    'amp', amp_sources, include_directories: element_incs,
    link_with: rage_types,
    install_dir: element_install_dir,
    install: true)

sndfile = dependency('sndfile')

persist_sources = ['elements/persistence/persistence.c']
persist = library(
    'persist',
    persist_sources,
    include_directories: element_incs,
    link_with: rage_types,
    dependencies: [jack, sndfile],
    install_dir: element_install_dir,
    install: true)

test_persist = executable(
    'test_persist',
    ['elements/persistence/test_persistence.c'],
    include_directories: [langext_inc, types_inc, graph_inc],
    link_with: [langext, rage_types, rage_graph, graph_test_factories])
test('persist', test_persist)

# Example

example_sources = ['example/main.c']
example = executable(
    'example',
    example_sources,
    include_directories: [langext_inc, types_inc, graph_inc],
    link_with: [langext, rage_types, rage_graph],
    dependencies: [jack])

# Haskell wrapper (because you can't return structs to GHC)

hsw_sources = ['haskwrap/src/wrappers.c']
hsw_inc = include_directories('haskwrap/include/')
hsw = library(
    'rage_hswrap',
    hsw_sources,
    include_directories: [graph_inc, types_inc, langext_inc, hsw_inc],
    link_with: [rage_graph],
    install: true)
