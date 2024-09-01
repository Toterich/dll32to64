#!/bin/python
import argparse
import glob
import os
import shutil
import subprocess

import build_params as bp

CWD = os.path.dirname(os.path.realpath(__file__))
SRC = os.path.join(CWD, 'src')

def main(comp64: str, comp32: str, dll: str, include: str, output: str, debug: bool):
    # Shared compiler flags for both targets
    compiler_flags = f'-Wall -Wextra -Werror -Wfatal-errors -static -funsigned-char '

    if debug:
        debug_flags = '-g -Og -DDEBUG=1'
    else:
        debug_flags = '-O3 -DDEBUG=0'
    
    compiler_flags += debug_flags
    compiler_flags = compiler_flags.split()

    os.makedirs(output, exist_ok=True)

    print("Building bridge.dll")
    subprocess.run([comp64,
        os.path.join(SRC, 'bridge', 'bridge.cpp'),
        os.path.join(SRC, 'common', 'msg_protocol.cpp'),
        os.path.join(SRC, 'common', 'socket.cpp'),
        '-shared',
        '-I' + include,
        '-I' + os.path.join(SRC),
        '-I' + os.path.join(CWD, 'vendor', 'plog'),
        '-lws2_32',
        '-o' + os.path.join(output, 'bridge.dll')] +
        compiler_flags 
    )

    dll_dir, dll_name = os.path.split(dll)
    dll_name = dll_name.rsplit('.', 1)[0]

    print("Building wrapper.exe")
    subprocess.run([comp32,
        os.path.join(SRC, 'wrapper', 'wrapper.cpp'),
        os.path.join(SRC, 'common', 'msg_protocol.cpp'),
        os.path.join(SRC, 'common', 'socket.cpp'),
        '-static', '-static-libgcc', '-static-libstdc++',
        '-I' + include,
        '-I' + os.path.join(SRC),
        '-I' + os.path.join(CWD, 'vendor', 'plog'),
        '-lws2_32',
        '-L' + dll_dir,
        '-Wl,-Bdynamic',
        '-l' + dll_name,
        '-Wl,-Bstatic',
        '-o' + os.path.join(output, 'wrapper.exe')] +
        compiler_flags
    )

    print("Copy DLLs to build dir")
    for f in glob.glob(os.path.join(dll_dir, '*.dll')):
        shutil.copy(f, output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Generate a bridge dLL and wrapper exe around a given library.")
    parser.add_argument('--compiler64', type=str, default=bp.COMPILER64, help='64bit GCC compiler to be used.')
    parser.add_argument('--compiler32', type=str, default=bp.COMPILER32, help='32bit GCC compiler to be used.')
    parser.add_argument('--dll', type=str, default=bp.WRAPPED_DLL, help='Path to the library to be wrapped.')
    parser.add_argument('--include', type=str, default=bp.WRAPPED_DLL_INCLUDE, help="Path to the header(s) declaring the DLL's exported symbols.")
    parser.add_argument('--output', type=str, default=bp.OUTPUT_DIR, help='Directory where the generated binaries should be stored.')
    parser.add_argument('--debug', action='store_true', help='Build debug binaries.')
    args = parser.parse_args()

    main(comp64=args.compiler64, comp32=args.compiler32, dll=args.dll, include=args.include, output=args.output, debug=args.debug)