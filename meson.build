project('rage', 'c')
inc = include_directories('include')
add_global_arguments('-std=c11', '-Werror', language: 'c')
src = ['src/elements.c']
librage = shared_library('rage', src, include_directories: inc)
test_elements = executable('test_elements', 'test/test_elements.c', include_directories: inc, link_with: librage)
test_errors = executable('test_errors', 'test/test_errors.c', include_directories: inc, link_with: librage)
test('elements', test_elements)
test('errors', test_errors)
