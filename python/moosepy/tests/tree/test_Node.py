#!/usr/bin/env python3
"""Tests moospy.tree.Node.Node."""
import time
import unittest

import moosepy.tree as moosetree

try:
    import anytree  # type: ignore
except ModuleNotFoundError:
    anytree = None

class TestNodeInitTime(unittest.TestCase):
    """Test timing of Node initialization."""

    if anytree is not None:

        class _AnytreeNode(anytree.NodeMixin):
            def __init__(self, parent, name):
                anytree.NodeMixin.__init__(self)
                self.parent = parent
                self.name = name

    @staticmethod
    def _createTree(N, cls):
        body = cls(None, "body")
        for i in range(N):
            div0 = cls(body, "div")
            for j in range(N):
                div1 = cls(div0, "div")
                for k in range(N):
                    cls(div1, "p")

    @staticmethod
    def _run_time(function, *args, **kwargs):
        start = time.time()
        function(*args, **kwargs)
        return time.time() - start

    @unittest.skipIf(anytree is None, "anytree not install")
    def testTime(self):
        """Compare timing of Node construction versus anytree."""
        N = 100
        t0 = time.time()
        self._createTree(N, TestNodeInitTime._AnytreeNode)
        t1 = time.time()
        self._createTree(N, moosetree.Node)
        t2 = time.time()
        print("\nmoosetree: {}\nanytree: {}".format(t2 - t1, t1 - t0))
        self.assertTrue((t2 - t1) < (t1 - t0))


class TestNode(unittest.TestCase):
    """Tests methods in moospy.tree.Node.Node."""

    def testInit(self):
        """Test Node constuction."""
        n0 = moosetree.Node(None, "root")
        self.assertIs(n0.parent, None)
        self.assertEqual(n0.name, "root")
        self.assertEqual(n0.children, [])
        self.assertEqual(n0.attributes, dict())

        n1 = moosetree.Node(n0, "one")
        self.assertEqual(n1.name, "one")
        self.assertIs(n1.parent, n0)
        self.assertEqual(n1.children, [])
        self.assertEqual(n1.attributes, dict())
        self.assertIs(n0(0), n1)
        self.assertEqual(n0.children, [n1])

        with self.assertRaises(AttributeError):
            n0.name = "foo"

    def testChildren(self):
        """Test Node.children."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n0, "n2")

        self.assertIs(n0.children[0], n1)
        self.assertIs(n0.children[1], n2)

        children = [c for c in n0]
        self.assertEqual(children, n0.children)

        with self.assertRaises(AttributeError):
            n0.children = [n0]

    def testInsert(self):
        """Test Node.insert."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(None, "n2")

        self.assertEqual(n0.children, [n1])
        n0.insert(0, n2)
        self.assertEqual(n0.children, [n2, n1])
        self.assertIs(n2.parent, n0)

    def testParent(self):
        """Test Node.parent."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n0, "n2")

        self.assertIs(n1.parent, n0)
        self.assertIs(n2.parent, n0)
        self.assertEqual(n0.children, [n1, n2])
        self.assertEqual(n1.children, [])
        self.assertEqual(n2.children, [])

        n2.parent = n1
        self.assertIs(n1.parent, n0)
        self.assertIs(n2.parent, n1)
        self.assertEqual(n0.children, [n1])
        self.assertEqual(n1.children, [n2])
        self.assertEqual(n2.children, [])

    def testAttributes(self):
        """Test Node attributes."""
        n = moosetree.Node(None, "root", year=1980)
        self.assertEqual(n.attributes, dict(year=1980))
        self.assertEqual(n["year"], 1980)
        self.assertEqual(n.get("year"), 1980)
        self.assertEqual(n.get("month", "Aug"), "Aug")
        self.assertNotIn("month", n)

        n["year"] = 1949
        n["month"] = 8

        self.assertEqual(n["year"], 1949)
        self.assertEqual(n["month"], 8)
        self.assertIn("month", n)

        keys = []
        values = []
        for k, v in n.items():
            keys.append(k)
            values.append(v)

        self.assertEqual(keys, ["year", "month"])
        self.assertEqual(values, [1949, 8])

    def testRoot(self):
        """Test Node.root."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n1, "n2")

        self.assertIs(n0.root, n0)
        self.assertIs(n1.root, n0)
        self.assertIs(n2.root, n0)

        self.assertTrue(n0.is_root)
        self.assertFalse(n1.is_root)
        self.assertFalse(n2.is_root)

    def testLen(self):
        """Test Node.__len__."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n0, "n2")
        self.assertEqual(len(n0), 2)
        self.assertEqual(len(n1), 0)
        self.assertEqual(len(n2), 0)

    def testBool(self):
        """Test Node.__bool__."""
        n0 = moosetree.Node(None, "root")
        self.assertTrue(n0)

    def testSiblings(self):
        """Test Node.siblings."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n1, "n2")
        n3 = moosetree.Node(n1, "n3")
        n4 = moosetree.Node(n0, "n4")
        n5 = moosetree.Node(n0, "n5")

        self.assertEqual(n0.siblings, [])
        self.assertEqual(n1.siblings, [n4, n5])
        self.assertEqual(n2.siblings, [n3])
        self.assertEqual(n3.siblings, [n2])
        self.assertEqual(n4.siblings, [n1, n5])
        self.assertEqual(n5.siblings, [n1, n4])

    def testPrevious(self):
        """Test Node.previous."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n0, "n2")
        n3 = moosetree.Node(n0, "n3")

        self.assertIs(n0.previous, None)
        self.assertIs(n1.previous, None)
        self.assertIs(n2.previous, n1)
        self.assertIs(n3.previous, n2)

    def testNext(self):
        """Test Node.next."""
        n0 = moosetree.Node(None, "root")
        n1 = moosetree.Node(n0, "n1")
        n2 = moosetree.Node(n0, "n2")
        n3 = moosetree.Node(n0, "n3")

        self.assertIs(n0.next, None)
        self.assertIs(n1.next, n2)
        self.assertIs(n2.next, n3)
        self.assertIs(n3.next, None)

    def testPath(self):
        """Test Node.path."""
        n0 = moosetree.Node(None, "0")
        n1 = moosetree.Node(n0, "1")
        n2 = moosetree.Node(n1, "2")
        n3 = moosetree.Node(n1, "3")
        self.assertEqual(n3.path, [n0, n1, n3])
        self.assertEqual(n2.path, [n0, n1, n2])
        self.assertEqual(n1.path, [n0, n1])
        self.assertEqual(n0.path, [n0])

    def testCall(self):
        """Test Node.__call__."""
        n0 = moosetree.Node(None, "0")
        n1 = moosetree.Node(n0, "1")
        n2 = moosetree.Node(n1, "2")
        n3 = moosetree.Node(n1, "3")
        n4 = moosetree.Node(n0, "4")
        n5 = moosetree.Node(n4, "5")
        n6 = moosetree.Node(n4, "6")

        self.assertIs(n0(0), n1)
        self.assertIs(n0(1), n4)

        self.assertIs(n0(0, 0), n2)
        self.assertIs(n0(0, 1), n3)
        self.assertIs(n0(0)(0), n2)
        self.assertIs(n0(0)(1), n3)

        self.assertIs(n0(1, 0), n5)
        self.assertIs(n0(1, 1), n6)
        self.assertIs(n0(1)(0), n5)
        self.assertIs(n0(1)(1), n6)

    def testCount(self):
        """Test Node.count."""
        n0 = moosetree.Node(None, "0")
        n1 = moosetree.Node(n0, "1")
        moosetree.Node(n1, "2")
        moosetree.Node(n1, "3")
        n4 = moosetree.Node(n0, "4")
        moosetree.Node(n4, "5")
        moosetree.Node(n4, "6")

        self.assertEqual(n0.count, 6)


if __name__ == "__main__":
    unittest.main(verbosity=2)
