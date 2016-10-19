project('rage', 'c')
inc = include_directories('include')
add_global_arguments('-std=c11', '-Werror', language: 'c')
src = ['src/loader.c']
librage = shared_library('rage', src, include_directories: inc)
test_loader = executable('test_elements', 'test/test_loader.c', include_directories: inc, link_with: librage)
test_errors = executable('test_errors', 'test/test_errors.c', include_directories: inc, link_with: librage)
test('elements', test_loader)
test('errors', test_errors)
