#!/bin/python
import argparse
import json
import re
import os
import subprocess

from cpp_gen import CppGen

try:
    import build_params as bp
    DEFAULT_PARAMS = bp.__dict__
except ModuleNotFoundError:
    DEFAULT_PARAMS = {}

CWD = os.path.dirname(os.path.realpath(__file__))
SRC = os.path.join(CWD, 'src')

# Group 1: Gen Tag
GEN_TAG = re.compile(r"^\s*// \[\[GEN\]\]\s*(.+)$")

def main(comp64 = None, comp32 = None, dll = None, include = None, output = None, cfg_path = None, debug = None):
    if comp64 is None:
        comp64 = DEFAULT_PARAMS.get('COMPILER64')
    if comp32 is None:
        comp32 = DEFAULT_PARAMS.get('COMPILER32')
    if dll is None:
        dll = DEFAULT_PARAMS.get('WRAPPED_DLL')
    if include is None:
        include = DEFAULT_PARAMS.get('WRAPPED_DLL_INCLUDE')
    if output is None:
        output = DEFAULT_PARAMS.get('OUTPUT_DIR')
    if cfg_path is None:
        cfg_path = DEFAULT_PARAMS.get("CONFIG")
    if debug is None:
        debug = DEFAULT_PARAMS.get('DEBUG')

    if not os.path.isabs(cfg_path):
        cfg_path = os.path.join(CWD, cfg_path)
    generator = CppGen(cfg_path)

    os.makedirs(output, exist_ok=True)
    
    print("Generate bridge.cpp")
    bridge_template = os.path.join(SRC, 'bridge', 'bridge.cpp.template')
    bridge_cpp = os.path.join(output, 'bridge.cpp')
    with open(bridge_cpp, 'w') as out_f, open(bridge_template, 'r') as in_f:
        print_out = lambda s: print(s, file=out_f, end='')
        for l in in_f:
            print_out(l)

            m = GEN_TAG.match(l)
            if not m:
                continue

            gen_tag = m.group(1)
            print_out(generator.code_gen(gen_tag))

    # Shared compiler flags for both targets
    compiler_flags = f'-Wall -Wextra -Werror -Wfatal-errors -static -funsigned-char '

    if debug:
        debug_flags = '-g -Og -DDEBUG=1'
    else:
        debug_flags = '-O3 -DDEBUG=0'
    
    compiler_flags += debug_flags
    compiler_flags = compiler_flags.split()

    print("Building bridge.dll")
    subprocess.check_output([comp64,
        os.path.join(output, 'bridge.cpp'),
        os.path.join(SRC, 'common', 'msg_protocol.cpp'),
        os.path.join(SRC, 'common', 'socket.cpp'),
        '-shared',
        '-I' + include,
        '-I' + os.path.join(CWD, 'include'),
        '-I' + os.path.join(SRC),
        '-I' + os.path.join(CWD, 'vendor', 'c', 'plog'),
        '-lws2_32',
        '-o' + os.path.join(output, 'bridge.dll')] +
        compiler_flags 
    )

    dll_dir, dll_name = os.path.split(dll)
    dll_name = dll_name.rsplit('.', 1)[0]

    print("Building wrapper.exe")
    subprocess.check_output([comp32,
        os.path.join(SRC, 'wrapper', 'wrapper.cpp'),
        os.path.join(SRC, 'common', 'msg_protocol.cpp'),
        os.path.join(SRC, 'common', 'socket.cpp'),
        '-static', '-static-libgcc', '-static-libstdc++',
        '-I' + include,
        '-I' + os.path.join(SRC),
        '-I' + os.path.join(CWD, 'vendor', 'c','plog'),
        '-lws2_32',
        '-L' + dll_dir,
        '-Wl,-Bdynamic',
        '-l' + dll_name,
        '-Wl,-Bstatic',
        '-o' + os.path.join(output, 'wrapper.exe')] +
        compiler_flags
    )


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Generate a bridge dLL and wrapper exe around a given library.")
    parser.add_argument('--compiler64', type=str, default=None, help='64bit GCC compiler to be used.')
    parser.add_argument('--compiler32', type=str, default=None, help='32bit GCC compiler to be used.')
    parser.add_argument('--dll', type=str, default=None, help='Path to the library to be wrapped.')
    parser.add_argument('--include', type=str, default=None, help="Path to the header(s) declaring the DLL's exported symbols.")
    parser.add_argument('--output', type=str, default=None, help='Directory where the generated binaries should be stored.')
    parser.add_argument('--debug', action='store_true', help='Build debug binaries.')
    args = parser.parse_args()

    main(comp64=args.compiler64, comp32=args.compiler32, dll=args.dll, include=args.include, output=args.output, debug=args.debug)