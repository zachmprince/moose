"""Wrapper for MOOSE HIT Parser."""

import os
import subprocess
import sys
from importlib.util import find_spec

# First, attempt to import hit. If that fails, try adding the hit source
# directory to the path and try the import again. If that fails, try running
# "make hit" before importing.
if find_spec("hit") is None:
    moose_dir = os.getenv(
        "MOOSE_DIR", os.path.join(os.path.dirname(__file__), "..", "..")
    )
    hit_dir = os.path.join(moose_dir, "framework", "contrib", "hit")
    sys.path.append(hit_dir)

    if find_spec("hit") is None:
        moose_test_dir = os.path.abspath(os.path.join(moose_dir, "test"))
        subprocess.run(["make", "hit"], cwd=moose_test_dir)

from hit import Token, TokenType

from .pyhit import Node, load, parse, tokenize, write

__all__ = ["TokenType", "Token", "Node", "load", "write", "parse", "tokenize"]
