project('rage', 'c')
inc = include_directories('include')
add_global_arguments('-std=c11', '-Werror', language: 'c')

src = ['src/loader.c', 'src/interpolation.c', 'src/time.c']
librage = static_library('rage', src, include_directories: inc)

elem_utils = shared_library('elem_utils', ['src/ports.c'], include_directories: inc)
test_eu = executable('test_eu', src=['test/test_ports.c'], include_directories: inc, link_with: elem_utils)
test('eu', test_eu)

test_loader = executable('test_elements', 'test/test_loader.c', include_directories: inc, link_with: librage)
test_errors = executable('test_errors', 'test/test_errors.c', include_directories: inc, link_with: librage)
test_interpolation = executable('test_interpolation', 'test/test_interpolation.c', include_directories: inc, link_with: librage)
test_time = executable('test_time', 'test/test_time.c', include_directories: inc, link_with: librage)
test('elements', test_loader)
test('errors', test_errors)
test('interpolation', test_interpolation)
test('time', test_time)

amp_sources = ['elements/amp/amp.c']
amp = shared_library('amp', amp_sources, include_directories: inc, link_with: elem_utils)
