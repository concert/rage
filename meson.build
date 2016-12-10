project('rage', 'c')

add_global_arguments('-std=c11', '-Werror', '-fms-extensions', language: 'c')

# Types

types_inc = include_directories('types/include')

sources = [
    'types/src/ports.c',
    'types/src/interpolation.c',
    'types/src/time.c',
    'types/src/time_series.c',
    'types/src/atoms.c'
]

rage_types = shared_library(
    'rage_types', sources, include_directories: types_inc)

test_errors = executable(
    'test_errors', 'types/test/test_errors.c', include_directories: types_inc,
    link_with: rage_types)
test_interpolation = executable(
    'test_interpolation', 'types/test/test_interpolation.c',
    include_directories: types_inc, link_with: rage_types)
test_time = executable(
    'test_time', 'types/test/test_time.c', include_directories: types_inc,
    link_with: rage_types)
test_ports = executable(
    'test_ports', src=['types/test/test_ports.c'], include_directories: types_inc,
    link_with: rage_types)
test('errors', test_errors)
test('interpolation', test_interpolation)
test('time', test_time)
test('ports', test_ports)

# Graph

cc = meson.get_compiler('c')
dyn_load = cc.find_library('dl')

jack = dependency('jack')

graph_inc = include_directories('graph/include')

# At some point this should become an executable:
rage_graph = static_library(
    'rage_graph',
    ['graph/src/loader.c', 'graph/src/harness.c'],
    include_directories: [graph_inc, types_inc],
    dependencies: [dyn_load],
    link_with: rage_types)

test_loader = executable(
    'test_elements', 'graph/test/main.c',
    include_directories: [graph_inc, types_inc],
    dependencies: [jack],
    link_with: rage_graph)
test('loader', test_loader)

# Elements

amp_sources = ['elements/amp/amp.c']
amp = shared_library(
    'amp', amp_sources, include_directories: types_inc, link_with: rage_types)

# Example

example_sources = ['example/main.c']
example = executable(
    'example', example_sources, include_directories: [types_inc, graph_inc],
    link_with: [rage_types, rage_graph], dependencies: [jack])
