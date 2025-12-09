# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html

import os
import re
import unittest

from MooseDocs.tree import markdown


class TestMarkdownTree(unittest.TestCase):
    def test_heading(self):
        head = markdown.Heading(level=1)
        markdown.Text(head, content="Very Cool Heading")
        self.assertEqual(head.write(), "# Very Cool Heading")

        head = markdown.Heading(level=5)
        markdown.Text(head, content="Very Cool Heading")
        self.assertEqual(head.write(), "##### Very Cool Heading")

        head = markdown.Heading(level=1, id="label")
        markdown.Text(head, content="Very Cool Heading With ID")
        self.assertEqual(head.write(), "# Very Cool Heading With ID {#label}")

    def test_code(self):
        code = markdown.Code(content='print("Hello World!")')
        self.assertEqual(code.write(), '`print("Hello World!")`')

        code_block = markdown.CodeBlock(content='print("Hello World!")')
        self.assertEqual(code_block.write(), '``` \nprint("Hello World!")\n```')
        code_block = markdown.CodeBlock(
            content='print("Hello World!")', language="python"
        )
        self.assertEqual(code_block.write(), '``` python\nprint("Hello World!")\n```')

    def test_link(self):
        link = markdown.Link(url="https://mooseframework.inl.gov/")
        markdown.Text(link, content="MOOSE Website")
        self.assertEqual(
            link.write(), "[MOOSE Website](https://mooseframework.inl.gov/)"
        )

    def test_lists(self):
        def make_list(parent):
            for i in range(3):
                li = markdown.MarkdownNode(parent=parent, pf_cls="ListItem")
                p = markdown.Paragraph(li)
                markdown.Text(p, content=str(i))

        ol = markdown.MarkdownNode(pf_cls="OrderedList")
        make_list(ol)
        self.assertEqual(ol.write(), "1.  0\n\n2.  1\n\n3.  2")

        ul = markdown.MarkdownNode(pf_cls="BulletList")
        make_list(ul)
        self.assertEqual(ul.write(), "- 0\n\n- 1\n\n- 2")

        nl = markdown.MarkdownNode(pf_cls="OrderedList")
        li1 = markdown.MarkdownNode(nl, pf_cls="ListItem")
        p1 = markdown.Paragraph(li1)
        markdown.Text(p1, content="Nested ordered")
        ol = markdown.MarkdownNode(li1, pf_cls="OrderedList")
        make_list(ol)
        li2 = markdown.MarkdownNode(nl, pf_cls="ListItem")
        p2 = markdown.Paragraph(li2)
        markdown.Text(p2, content="Nested unordered")
        ul = markdown.MarkdownNode(li2, pf_cls="BulletList")
        make_list(ul)
        expected = """1.  Nested ordered

    1.  0

    2.  1

    3.  2

2.  Nested unordered

    - 0

    - 1

    - 2"""
        self.assertEqual(nl.write(), expected)

    def test_block_quote(self):
        message = ["Hello from MooseDocs!", "I think therefore I am"]
        q = markdown.MarkdownNode(pf_cls="BlockQuote")
        p = markdown.Paragraph(q)
        expected = ""
        for m in message:
            markdown.Text(p, content=m + "\n")
            expected += f"> {m}\n"
        self.assertEqual(q.write(), expected.strip())

    def test_formats(self):
        message = "formatted_string"

        def check_format(pf_cls, deco_st, deco_ed=None):
            f = markdown.MarkdownNode(pf_cls=pf_cls)
            markdown.Text(f, content=message)
            self.assertEqual(
                f.write(), f"{deco_st}{message}{deco_ed or deco_st}", msg=str(pf_cls)
            )

        check_format("Strong", "**")
        check_format("Emph", "*")
        check_format("Underline", "[", "]{.underline}")
        check_format("Strikeout", "~~")
        check_format("Subscript", "~")
        check_format("Superscript", "^")

    def test_cite(self):
        p = markdown.Paragraph()
        markdown.Text(p, content="This is one citation: ")
        markdown.Cite(p, key="foo")
        markdown.MarkdownNode(p, pf_cls="LineBreak")
        markdown.Text(p, content="This is another citation: ")
        markdown.Cite(p, key="bar")

        content = p.write().splitlines()
        self.assertEqual(len(content), 2)
        self.assertEqual(content[0].strip(), "This is one citation: [^foo]")
        self.assertEqual(content[1].strip(), "This is another citation: [^bar]")

    def test_bibliography(self):
        from pybtex.database import parse_file
        from pybtex.plugin import find_plugin

        this_dir = os.path.dirname(__file__)
        moose_dir = os.path.abspath(os.path.join(this_dir, *([".."] * 4)))
        bib_file = os.path.join(
            moose_dir, "framework", "doc", "content", "bib", "moose.bib"
        )

        db = parse_file(bib_file)
        keys = ["testkey", "libMeshPaper"]
        backend = find_plugin("pybtex.backends", "markdown")(encoding="utf-8")
        style = find_plugin("pybtex.style.formatting", "plain")
        formatted_bib = style().format_bibliography(db, keys)
        entries = {entry.key: entry.text.render(backend) for entry in formatted_bib}

        backslash_re = re.compile(r"\\(?=[\.\-\(\)\\\+\*_\{\}\[\]#!`])")
        doc = markdown.MarkdownDocument()
        bib = markdown.Bibliography(doc)
        for key, text in entries.items():
            bib.add_citation(key, backslash_re.sub("", text))

        content = doc.write()
        expected = r"""[^libMeshPaper]: B. S. Kirk, J. W. Peterson, R. H. Stogner, and G. F. Carey.
    \texttt libMesh: A C++ Library for Parallel Adaptive Mesh Refinement/Coarsening Simulations.
    *Engineering with Computers*, 22(3–4):237–254, 2006.
    URL: [http://dx.doi.org/10.1007/s00366-006-0049-3](http://dx.doi.org/10.1007/s00366-006-0049-3).

[^testkey]: Jane Smith and John Doe.
    A test citation without special characters for easy testing.
    *A Prestigous Journal*, 1980."""
        self.assertEqual(content, expected)


class TestMarkdownAlert(unittest.TestCase):
    def test_icon(self):
        for icon_name, emoji in markdown.Icon.ICON_EMOJI_DICT.items():
            icon = markdown.Icon(icon=icon_name)
            self.assertEqual(icon.write(), emoji.decode("utf-8"))

    def test_alert_no_title(self):
        alert = markdown.Alert()
        markdown.Text(alert, content="[!NOTE]", raw=True)
        markdown.MarkdownNode(alert, "LineBreak")
        markdown.Text(markdown.Paragraph(alert), content="Alert content")

        content = alert.write()
        expected = r"^> \[\!NOTE\]\s*\n>\s*\n> Alert content$"
        self.assertRegex(content, expected)

    def test_alert_title_no_icon(self):
        alert = markdown.Alert()
        markdown.Text(alert, content="[!NOTE]", raw=True)
        markdown.MarkdownNode(alert, "LineBreak")
        markdown.Text(
            markdown.MarkdownNode(alert, "Strong"),
            content="Alert title",
        )
        markdown.Text(markdown.Paragraph(alert), content="Alert content")

        content = alert.write()
        expected = r"^> \[\!NOTE\]\s*\n> \*\*Alert title\*\*\n>\n> Alert content$"
        self.assertRegex(content, expected)

    def test_alert(self):
        alert = markdown.Alert()
        markdown.Text(alert, content="[!NOTE]", raw=True)
        markdown.MarkdownNode(alert, "LineBreak")

        markdown.Icon(alert, icon="comment")
        icon = markdown.Icon.ICON_EMOJI_DICT["comment"].decode("utf-8")
        markdown.MarkdownNode(alert, "Space")

        markdown.Text(
            markdown.MarkdownNode(alert, "Strong"),
            content="Alert title",
        )

        markdown.Text(markdown.Paragraph(alert), content="Alert content")

        content = alert.write()
        expected = f"^> \\[\\!NOTE\\]\\s*\\n> {icon} \\*\\*Alert title\\*\\*\\n>\\n> Alert content$"
        self.assertRegex(content, expected)


class TestMarkdownFloat(unittest.TestCase):
    def test_float(self):
        flt = markdown.Float()
        cap = markdown.Caption(flt)
        cap.id = "label"
        markdown.Text(cap, content="This is a caption.")

        p = markdown.Paragraph(flt)
        markdown.Text(p, content="Hello World!")

        lines = flt.write().splitlines()
        self.assertEqual(len(lines), 3)
        self.assertEqual(lines[0], "[This is a caption.]{#label}")
        self.assertEqual(lines[1], "")
        self.assertEqual(lines[2], "Hello World!")

    def test_float_no_label_no_caption(self):
        flt = markdown.Float()
        p = markdown.Paragraph(flt)
        markdown.Text(p, content="Hello World!")

        lines = flt.write().splitlines()
        self.assertEqual(len(lines), 1)
        self.assertEqual(lines[0], "Hello World!")

    def test_float_no_label(self):
        flt = markdown.Float()
        cap = markdown.Caption(flt)
        markdown.Text(cap, content="This is a caption.")

        p = markdown.Paragraph(flt)
        markdown.Text(p, content="Hello World!")

        lines = flt.write().splitlines()
        self.assertEqual(len(lines), 3)
        self.assertEqual(lines[0], "This is a caption.")
        self.assertEqual(lines[1], "")
        self.assertEqual(lines[2], "Hello World!")

    def test_table(self):
        flt = markdown.Float()
        caption = markdown.MarkdownNode(flt, "Span")
        caption.id = "tab:table_label"

        table = markdown.Table(flt, alignment=["left", "center", "right"])
        head = markdown.MarkdownNode(table, pf_cls="TableHead")
        body = markdown.MarkdownNode(table, pf_cls="TableBody")

        markdown.Text(caption, content="This is a caption.")

        def get_table_cell(parent, i, j):
            content = f"Heading {j + 1}" if i == 0 else f"Item {i} {j + 1}"
            return markdown.Text(parent, content=content)

        row1 = markdown.MarkdownNode(head, pf_cls="TableRow")
        get_table_cell(markdown.TableCell(row1), 0, 0)
        get_table_cell(markdown.TableCell(row1), 0, 1)
        get_table_cell(markdown.TableCell(row1), 0, 2)

        row2 = markdown.MarkdownNode(body, pf_cls="TableRow")
        get_table_cell(markdown.TableCell(row2), 1, 0)
        get_table_cell(markdown.TableCell(row2), 1, 1)
        get_table_cell(markdown.TableCell(row2), 1, 2)

        table_str = flt.write()
        rows = table_str.split("\n")
        self.assertEqual(len(rows), 5)

        self.assertEqual(rows[0], "[This is a caption.]{#tab:table_label}")
        self.assertEqual(rows[1], "")

        cells = []
        for i in range(2, len(rows)):
            tmp = rows[i].split("|")
            self.assertEqual(len(tmp), 5)
            self.assertEqual(tmp[0], "")
            self.assertEqual(tmp[-1], "")
            cells.append([c.strip() for c in tmp[1:-1]])

        self.assertListEqual(cells[0], ["Heading 1", "Heading 2", "Heading 3"])
        self.assertListEqual(cells[2], ["Item 1 1", "Item 1 2", "Item 1 3"])
        self.assertRegex(cells[1][0], r"^:-+$")
        self.assertRegex(cells[1][1], r"^:-+:$")
        self.assertRegex(cells[1][2], r"^-+:$")

    def test_image(self):
        # No caption no label
        img = markdown.Image(src="image.png")
        self.assertEqual(img.write(), "![](image.png)")

        # No caption
        img = markdown.Image(src="image.png")
        img.id = "label"
        self.assertEqual(img.write(), "![](image.png){#label}")

        # No label
        img = markdown.Image(src="image.png")
        markdown.Text(markdown.Caption(img), content="Caption")
        self.assertEqual(img.write(), "![Caption](image.png)")

        # Caption and label
        img = markdown.Image(src="image.png")
        markdown.Text(markdown.Caption(img), content="Caption")
        img.id = "label"
        self.assertEqual(img.write(), "![Caption](image.png){#label}")

        # From float, no caption no label
        flt = markdown.Float()
        img = markdown.Image(flt, src="image.png")
        self.assertEqual(flt.write(), "![](image.png)")

        # From float, no label
        flt = markdown.Float()
        cap = markdown.Caption(flt)
        markdown.Text(cap, content="Caption")
        img = markdown.Image(flt, src="image.png")
        self.assertEqual(flt.write(), "![Caption](image.png)")

        # From float
        flt = markdown.Float()
        cap = markdown.Caption(flt)
        cap.id = "label"
        markdown.Text(cap, content="Caption")
        img = markdown.Image(flt, src="image.png")
        self.assertEqual(flt.write(), "![Caption](image.png){#label}")


if __name__ == "__main__":
    unittest.main(verbosity=2)
