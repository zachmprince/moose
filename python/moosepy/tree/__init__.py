"""
A package for building tree structures.

This package is designed as a faster alternative to the
anytree package, although it is not a direct replacement.
"""

from .Node import Node
from .search import IterMethod, find, findall, iterate

__all__ = ["Node", "findall", "find", "iterate", "IterMethod"]
