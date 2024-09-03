#!/bin/python

import os
import subprocess

cwd = os.path.dirname(os.path.realpath(__file__))

import sys
sys.path.insert(0, os.path.dirname(cwd))
import build_params as bp
from build import main as build_dut

test_output_path = os.path.join(cwd, 'build')
os.makedirs(test_output_path, exist_ok=True)

test_lib_path = os.path.join(test_output_path, 'test_lib.dll')

print("Building test_lib.dll")
subprocess.check_output([bp.COMPILER32,
    os.path.join(cwd, 'test_lib.cpp'),
    '-shared',
    '-lws2_32',
    '-g',
    '-Og',
    '-funsigned-char',
    '-o' + test_lib_path]
)

build_dut(dll=test_lib_path, include=cwd, output=test_output_path, debug=True)

print("Building test_app.exe")
subprocess.check_output([bp.COMPILER64,
    os.path.join(cwd, 'test_app.cpp'),
    '-static', '-static-libgcc', '-static-libstdc++',
    '-g',
    '-Og',
    '-I' + os.path.join(cwd, '..', 'include'),
    '-L' + test_output_path,
    '-Wl,-Bdynamic',
    '-lbridge',
    '-Wl,-Bstatic',
    '-o' + os.path.join(test_output_path, 'test_app.exe')]
    )

print("Executing tests")
subprocess.run(os.path.join(test_output_path, 'test_app.exe'))