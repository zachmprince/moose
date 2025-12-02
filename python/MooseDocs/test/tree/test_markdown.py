# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html

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
        check_format("Underline", "<u>", "</u>")
        check_format("Strikeout", "~~")
        check_format("Subscript", "<sub>", "</sub>")
        check_format("Superscript", "<sup>", "</sup>")

    def test_table(self):
        flt = markdown.TableFloat()
        table = markdown.Table(flt, alignment=["left", "center", "right"])
        head = markdown.MarkdownNode(table, pf_cls="TableHead")
        body = markdown.MarkdownNode(table, pf_cls="TableBody")
        caption = markdown.Caption(flt)
        flt.id = "tab:table_label"

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
        cells = []
        for i in range(3):
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

        self.assertEqual(rows[3], "")
        self.assertEqual(rows[4], "This is a caption. {#tab:table_label}")

    def test_alert(self):
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


if __name__ == "__main__":
    unittest.main(verbosity=2)
