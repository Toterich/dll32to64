#!/bin/python

import os
import subprocess

cwd = os.path.dirname(os.path.realpath(__file__))

import sys
sys.path.insert(0, os.path.dirname(cwd))
import build_params as bp
from build import main as build_dut

test_lib_path = os.path.join(cwd, 'test_lib.dll')

print("Building test_lib.dll")
subprocess.run([bp.COMPILER32,
    os.path.join(cwd, 'test_lib.cpp'),
    '-shared',
    '-lws2_32',
    '-g',
    '-Og',
    '-funsigned-char',
    '-o' + test_lib_path]
)

# TODO:
# Pass the following parameters
# * path to test_lib.dll
# * output directory
# The given params should override the contents of build_params.py
build_dut(dll=test_lib_path, output=cwd, debug=True)

print("Building test_app.exe")
subprocess.run([bp.COMPILER64,
    os.path.join(cwd, 'test_app.cpp'),
    '-static', '-static-libgcc', '-static-libstdc++',
    '-g',
    '-Og',
    '-lbridge',
    '-o' + os.path.join(cwd, 'test_app.exe')]
    )

print("Executing tests")
subprocess.run(os.path.join(cwd, 'test_app.exe'))