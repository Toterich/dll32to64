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
COMPILER_FLAGS = COMPILER_FLAGS.split()

CWD = os.path.dirname(os.path.realpath(__file__))
SRC = os.path.join(CWD, 'src')

DLL_PATH = os.path.join(CWD, bp.WRAPPED_DLL)
DLL_DIR, DLL_NAME = os.path.split(DLL_PATH)
DLL_NAME = DLL_NAME.split('.')[0]

DLL_INCLUDE_PATH = os.path.join(CWD, bp.WRAPPED_DLL_INCLUDE)

BUILD_DIR = os.path.join(CWD, 'build')
os.makedirs(BUILD_DIR, exist_ok=True)

def main():
    print("Building bridge.dll")
    subprocess.run([bp.COMPILER64,
        os.path.join(SRC, 'bridge', 'bridge.cpp'),
        os.path.join(SRC, 'common', 'msg_protocol.cpp'),
        os.path.join(SRC, 'common', 'socket.cpp'),
        '-shared',
        '-I' + DLL_INCLUDE_PATH,
        '-I' + os.path.join(SRC),
        '-I' + os.path.join(CWD, 'vendor', 'plog'),
        '-lws2_32',
        '-o' + os.path.join(BUILD_DIR, 'bridge.dll')] +
        COMPILER_FLAGS
    )

    print("Building wrapper.exe")
    subprocess.run([bp.COMPILER64,
        os.path.join(SRC, 'wrapper', 'wrapper.cpp'),
        os.path.join(SRC, 'common', 'msg_protocol.cpp'),
        os.path.join(SRC, 'common', 'socket.cpp')] +
        COMPILER_FLAGS +
        ['-static', '-static-libgcc', '-static-libstdc++',
        '-I' + DLL_INCLUDE_PATH,
        '-I' + os.path.join(SRC),
        '-I' + os.path.join(CWD, 'vendor', 'plog'),
        '-lws2_32',
        '-L' + DLL_DIR,
        '-Wl,-Bdynamic',
        '-l' + DLL_NAME,
        '-Wl,-Bstatic',
        '-o' + os.path.join(BUILD_DIR, 'wrapper.exe')]
    )


    print("Copy DLLs to build dir")
    for f in glob.glob(os.path.join(DLL_DIR, '*.dll')):
        shutil.copy(f, BUILD_DIR)


if __name__ == '__main__':
    main()