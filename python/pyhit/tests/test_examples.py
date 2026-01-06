#!/usr/bin/env python3
# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html
"""Tests pyhit API on input files."""

import os
import unittest


class TestExamples(unittest.TestCase):
    """Tests loading and writing input files with pyhit."""

    def setUp(self):
        """Change to the directory this file is in."""
        self._origwd = os.getcwd()
        os.chdir(os.path.dirname(__file__))

    def tearDown(self):
        """Change back to original working directory."""
        os.chdir(self._origwd)

    def test(self):
        """Test loading and writing input files with pyhit."""
        # MOOSEDOCS:example-begin
        # Load the packages
        import moosepy.tree as moosetree
        import pyhit

        # Read the file
        root = pyhit.load("input.i")

        # Locate and modify "x_max" parameter for the mesh
        mesh = moosetree.find(root, func=lambda n: n.fullpath == "/Mesh/gen")
        mesh["x_max"] = 4

        # Set the comment on altered parameter
        mesh.setComment("x_max", "Changed from 3 to 4")

        # Write the modified file
        pyhit.write("input_modified.i", root)

        # MOOSEDOCS:example-end
        self.assertEqual(mesh["x_max"], 4)
        self.assertEqual(mesh.comment("x_max"), "Changed from 3 to 4")

        out = mesh.render()
        self.assertIn("x_max = 4", out)
        self.assertIn("Changed from 3 to 4", out)


if __name__ == "__main__":
    unittest.main(module=__name__, verbosity=2)
