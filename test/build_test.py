#!/bin/python

import os
import subprocess

cwd = os.path.dirname(os.path.realpath(__file__))

import sys
sys.path.insert(0, os.path.dirname(cwd))
import build_params as bp

subprocess.run([bp.COMPILER32,
    os.path.join(cwd, 'test_lib.cpp'),
    '-shared',
    '-lws2_32',
    '-g',
    '-Og',
    '-funsigned-char',
    '-o' + os.path.join(cwd, 'test_lib.dll')]
)


