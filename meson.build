project('rage', 'c')

add_project_arguments('-std=gnu11', '-Werror', '-fms-extensions', '-Wno-microsoft-anon-tag', language: 'c')

# Language Extensions

langext_inc = include_directories('langext/include')

langext_sources = [
    'langext/src/countdown.c',
    'langext/src/set.c',
    'langext/src/rtcrit.c'
]

sem = dependency('threads')

langext = static_library(
    'rage_langext', langext_sources, include_directories: langext_inc,
    dependencies: [sem])
langext_dep = declare_dependency(link_with: langext, include_directories: langext_inc)

test_errors = executable(
    'test_errors', 'langext/test/test_errors.c', dependencies: [langext_dep])
test('errors', test_errors)
test_langext = executable(
    'test_langext', 'langext/test/main.c',
    dependencies: [langext_dep])
test('langext', test_langext)

# Types

types_inc = include_directories('types/include')

sources = [
    'types/src/ports.c',
    'types/src/interpolation.c',
    'types/src/chronology.c',
    'types/src/test_factories.c',
    'types/src/atoms.c',
    'types/src/element_helpers.c',
    'types/src/event.c',
    'types/src/queue.c'
]

rage_types = library(
    'rage_types',
    sources,
    include_directories: [types_inc],
    dependencies: [sem, langext_dep],
    install: true)
types_dep = declare_dependency(link_with: rage_types, include_directories: types_inc)

test_types = executable(
    'test_types', 'types/test/test_main.c',
    dependencies: [types_dep, langext_dep])
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
    'graph/include/binding_interface.h', 'graph/include/con_trans.h',
    'types/include/atoms.h', 'types/include/element.h',
    'types/include/ports.h', 'types/include/interpolation.h',
    'types/include/chronology.h', 'types/include/transport_state.h',
    'types/include/time_series.h',
    'langext/include/macros.h', 'langext/include/error.h',
    subdir: 'rage')

rage_graph = library(
    'rage_graph',
    [
        'graph/src/loader.c', 'graph/src/jack_bindings.c', 'graph/src/srt.c',
        'graph/src/proc_block.c', 'graph/src/graph.c', 'graph/src/buffer_pile.c',
        'graph/src/binding_interface.c', 'graph/src/depmap.c',
        'graph/src/categorisation.c'
    ],
    include_directories: [graph_inc],
    dependencies: [dyn_load, jack, sem, types_dep, langext_dep],
    install: true)
graph_dep = declare_dependency(link_with: rage_graph, include_directories: graph_inc)

graph_test_factories = library(
    'rage_graph_test_factories',
    ['graph/src/graph_test_factories.c'],
    dependencies: [graph_dep, types_dep, langext_dep])
graph_test_factories_dep = declare_dependency(
    link_with: graph_test_factories, include_directories: graph_inc)

test_graph = executable(
    'test_graph', 'graph/test/main.c',
    dependencies: [
        sem, graph_dep, types_dep, langext_dep, graph_test_factories_dep])
test('graph', test_graph)

# Elements

element_deps = [types_dep, langext_dep]
element_install_dir = join_paths(get_option('datadir'), 'rage', 'elements')

amp_sources = ['elements/amp/amp.c']
amp = library(
    'amp', amp_sources,
    dependencies: element_deps,
    install: true)

sndfile = dependency('sndfile')

persist_sources = ['elements/persistence/persistence.c']
persist = library(
    'persist',
    persist_sources,
    dependencies: [jack, sndfile, element_deps],
    install_dir: element_install_dir,
    install: true)

test_persist = executable(
    'test_persist',
    ['elements/persistence/test_persistence.c'],
    dependencies: [graph_dep, graph_test_factories_dep, element_deps])
test('persist', test_persist)

# Example

example_sources = ['example/main.c']
example = executable(
    'example',
    example_sources,
    dependencies: [graph_dep, langext_dep, types_dep])

# Haskell wrapper (because you can't return structs to GHC)

hsw_sources = ['haskwrap/src/wrappers.c', 'haskwrap/src/refcounter.c']
hsw_inc = include_directories('haskwrap/include/')
hsw = library(
    'rage_hswrap',
    hsw_sources,
    include_directories: hsw_inc,
    dependencies: [graph_dep, types_dep, langext_dep],
    install: true)
rage_hsw_dep = declare_dependency(link_with: hsw, include_directories: hsw_inc)

test_hsw = executable(
    'test_haskwrap',
    ['haskwrap/test/test_refcounter.c'],
    dependencies: [rage_hsw_dep, types_dep, langext_dep])
test('haskwrap', test_hsw)
