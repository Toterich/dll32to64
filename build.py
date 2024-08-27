#!/bin/python
import glob
import os
import shutil
import subprocess

import build_params as bp

if bp.DEBUG:
    OPT_FLAGS = '-g -Og'
else:
    OPT_FLAGS = '-O3'

# Shared compiler flags for both targets
COMPILER_FLAGS = f'{OPT_FLAGS} -DDEBUG={bp.DEBUG} -Wall -Wextra -Werror -Wfatal-errors -static -funsigned-char'

cwd = os.path.dirname(os.path.realpath(__file__))
src = os.path.join(cwd, 'src')

dll_path = os.path.join(cwd, bp.WRAPPED_DLL)
dll_dir, dll_name = os.path.split(dll_path)
dll_name = dll_name.split('.')[0]

dll_include_path = os.path.join(cwd, bp.WRAPPED_DLL_INCLUDE)

build_dir = os.path.join(cwd, 'build')

print("Build bridge.dll")
subprocess.run([bp.COMPILER64,
    os.path.join(src, 'bridge', 'bridge.cpp'),
    os.path.join(src, 'common', 'msg_protocol.cpp'),
    os.path.join(src, 'common', 'socket.cpp'),
    COMPILER_FLAGS,
    '-shared',
    '-I' + dll_include_path,
    '-I' + os.path.join(cwd, 'vendor', 'plog'),
    '-lws2_32',
    '-o' + os.path.join(build_dir, 'bridge.dll')
])

print("Build wrapper.exe")
subprocess.run([bp.COMPILER32,
    os.path.join(src, 'wrapper', 'wrapper.cpp'),
    os.path.join(cwd, 'common', 'msg_protocol.cpp'),
    os.path.join(cwd, 'common', 'socket.cpp'),
    COMPILER_FLAGS,
    '-static', '-static-libgcc', '-static-libstdc++'
    '-I' + dll_include_path,
    '-I' + os.path.join(cwd, 'vendor', 'plog'),
    '-lws2_32',
    '-L' + dll_dir,
    '-Wl,-Bdynamic',
    '-l' + dll_name,
    '-Wl,-Bstatic',
    '-o' + os.path.join(build_dir, 'wrapper.exe')
])

print("Copy DLLs to build dir")
for f in glob.glob(os.path.join(dll_dir, '*.dll')):
    shutil.copy(f, build_dir)