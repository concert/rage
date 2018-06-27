#!/usr/bin/env python

from sys import argv
from os import listdir, environ
from os.path import join
from subprocess import check_output
from io import BytesIO
import re

pkg_dirs = ['langext', 'types', 'graph']
base_dir = '.'

include_dirs = [join(base_dir, p, 'include') for p in pkg_dirs]
target_header = argv[1]

included_re = re.compile('#include "(.*)"')


def in_pkg_includes(hdr_path):
    with open(hdr_path) as f:
        for l in f.readlines():
            m = included_re.match(l)
            if m:
                yield m.groups()[0]


pkg_headers = {}
for include_dir in include_dirs:
    header_names = listdir(include_dir)
    for header_name in header_names:
        pkg_headers[header_name] = join(include_dir, header_name)


all_included_headers = [target_header]
headers_todo = {target_header}
while headers_todo:
    this_round_headers = set()
    for hdr in headers_todo:
        this_round_headers |= set(in_pkg_includes(pkg_headers[hdr]))
    headers_todo = this_round_headers - set(all_included_headers)
    all_included_headers.extend(this_round_headers)

for h in tuple(all_included_headers):
    if all_included_headers.count(h) > 1:
        all_included_headers.remove(h)

all_included_headers.reverse()

filtered = re.compile('#include.*|#pragma once')
sys_include_re = re.compile('#include <(.*)>')
unified_header = [] 
system_includes = []
for h in all_included_headers:
    with open(pkg_headers[h]) as f:
        for l in f.readlines():
            if filtered.match(l):
                m = sys_include_re.match(l)
                if m:
                    system_includes.append(m.groups()[0])
                continue
            unified_header.append(l)
unified_header = '\n'.join(unified_header)

print(check_output(('cc', '-E', '-P', '-'), input=unified_header.encode('utf8')).decode('utf8'))
