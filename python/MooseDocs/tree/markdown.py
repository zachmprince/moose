# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html
"""
MooseDocs tree nodes that render Markdown through panflute/pandoc.

This module defines `MarkdownNode` and a series of convenience wrappers that
mirror common Markdown constructs (headings, tables, etc.) while deferring the
final conversion to panflute. The classes manage panflute keyword translation,
block/inline normalization, and raw CommonMark emission.
"""

import importlib.util
from typing import TYPE_CHECKING, Optional, Type

if TYPE_CHECKING:
    import panflute as pf
else:
    if importlib.util.find_spec("panflute") is not None:
        import panflute as pf
    else:
        pf = None

from .base import NodeBase

MARKDOWN_FORMAT = "commonmark_x"
"""
Markdown "flavor" to render. Options, just for reference:
- commonmark
- commonmark_x
- gfm
- markdown
- markdown_mmd
- markdown_phpextra
- markdown_strict
"""

PANDOC_EXTENSIONS = [
    # Use commonmark sub/superscript syntax
    "+subscript",
    "+superscript",
    # Prevent prevent pandoc from adding raw HTML content
    "-raw_html",
    # Forces adding heading labels even if label is trivial
    "-gfm_auto_identifiers",
]
"""
Extensions to disable/enable for pandoc redending.
"+" prefix for enable, "-" prefix for disable.
"""


class MarkdownNode(NodeBase):
    """
    Generic node that mirrors panflute elements inside the MooseDocs tree.

    Attributes
    ----------
    DEFAULT_PF_CLASS : Type[pf.Element] or None
        Default panflute element to instantiate when no override is provided.
    DEFAULT_PF_KWARGS : dict[str, str]
        Mapping between panflute keyword arguments and __init__ kwarg names.
    ENSURE_CHILDREN_ARE_BLOCK : bool
        When True, inline children are wrapped in block containers.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> ordered = markdown.MarkdownNode(pf_cls="OrderedList")
    >>> item = markdown.ListItem(ordered)
    >>> markdown.Text(item, content="First entry")
    >>> ordered.write()
    '1.  First entry'

    """

    DEFAULT_PF_CLASS: Optional[Type[pf.Element]] = None
    DEFAULT_PF_KWARGS: dict[str, str] = {}
    ENSURE_CHILDREN_ARE_BLOCK = False

    def __init__(
        self,
        parent: Optional["MarkdownNode"] = None,
        pf_cls: Optional[Type[pf.Element]] = None,
        name: Optional[str] = None,
        **kwargs,
    ):
        """
        Constuct MarkdownNode instance.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node that will own the new child.
        pf_cls : Type[pf.Element] or str, optional
            Specific panflute element class (or its name) to instantiate.
        name : str, optional
            Friendly name used by the base Node implementation.
        **kwargs
            Extra keyword arguments forwarded to `NodeBase` and captured
            inside `pf_kwargs` for panflute construction.

        Raises
        ------
        ModuleNotFoundError
            If panflute is not available in the environment.
        ValueError
            If neither an explicit nor default panflute class is provided.

        """
        if pf is None:
            raise ModuleNotFoundError(
                "panflute must be installed to use markdown renderer."
            )

        if pf_cls is None:
            if self.DEFAULT_PF_CLASS is None:
                raise ValueError("MarkdownNode requires a panflute class")
            pf_cls = self.DEFAULT_PF_CLASS

        # Accept either the class object or its name as a string
        if isinstance(pf_cls, str):
            try:
                pf_cls = getattr(pf, pf_cls)
            except AttributeError as exc:
                raise ValueError(f"Unknown panflute element: {pf_cls!r}") from exc

        kwargs.setdefault("pf_kwargs", {})
        for pf_arg, kwarg_arg in self.DEFAULT_PF_KWARGS.items():
            kwargs["pf_kwargs"].setdefault(pf_arg, kwargs.pop(kwarg_arg, None))

        name = name or pf_cls.__name__
        super().__init__(name=name, parent=parent, **kwargs)

        self._pf_cls = pf_cls

    @property
    def children(self) -> list["MarkdownNode"]:
        """
        Return a list of children.

        Overridden for better type-hinting.
        """
        return super().children

    @property
    def id(self) -> str:
        """Identifier given to panflute class."""
        return self["pf_kwargs"].get("identifier", None)

    @id.setter
    def id(self, val: Optional[str]):
        """Set 'identifier' argument for panflute class construction."""
        if val is not None:
            self["pf_kwargs"]["identifier"] = val
        else:
            self["pf_kwargs"].pop("identifier", None)

    def _materialize_children(self) -> list[pf.Element]:
        """
        Convert children into panflute elements, optionally wrapping inline nodes.

        Returns
        -------
        list of pf.Element
            Ordered list of panflute elements ready to be attached to the parent.

        """
        rendered = []
        inline_results = []
        for child in self.children:
            assert isinstance(child, MarkdownNode)

            # Convert child to panflute object
            results = child.to_panflute()
            if results is None:
                continue
            if not isinstance(results, (list, tuple)):
                results = [results]

            # Ensure children are blocks, if necessary
            for result in results:
                if self.ENSURE_CHILDREN_ARE_BLOCK and isinstance(result, pf.Inline):
                    inline_results.append(result)
                else:
                    if inline_results:
                        rendered.append(pf.Plain(*inline_results))
                        inline_results = []
                    rendered.append(result)
        if inline_results:
            rendered.append(pf.Plain(*inline_results))

        return rendered

    def to_panflute(self) -> pf.Element:
        """
        Build a panflute element corresponding to this node.

        Returns
        -------
        pf.Element
            The rendered panflute element (or list) for the node.

        Raises
        ------
        ValueError
            If construction fails due to invalid child state.

        """
        children = self._materialize_children()
        try:
            return self._pf_cls(*children, **self["pf_kwargs"])
        except Exception as e:
            raise ValueError(
                f"Error converting {type(self)} to panflute:\n{self}"
            ) from e

    def write(self) -> str:
        """
        Render the node subtree to CommonMark-compatible Markdown text.

        Returns
        -------
        str
            Markdown representation produced via panflute and pandoc.

        Raises
        ------
        ValueError
            If the node cannot be converted into a panflute document.

        """
        try:
            elem = self.to_panflute()
        except Exception as e:
            raise ValueError(f"Error converting {type(self)} to panflute") from e

        if isinstance(elem, pf.Doc):
            doc = elem
        elif isinstance(elem, pf.Block):
            doc = pf.Doc(elem)
        elif isinstance(elem, (list, tuple)):
            doc = pf.Doc(*elem)
        else:
            doc = pf.Doc(pf.Plain(elem))
        # convert_text understands pf.Doc instances, so this yields raw markdown
        return pf.convert_text(
            doc,
            input_format="panflute",
            output_format=MARKDOWN_FORMAT + "".join(PANDOC_EXTENSIONS),
        )


class MarkdownDocument(MarkdownNode):
    """
    Root container that produces a `pf.Doc` object for top-level rendering.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> doc = markdown.MarkdownDocument()
    >>> para = markdown.Paragraph(doc)
    >>> markdown.Text(para, content="Hello from MooseDocs")
    >>> doc.write()
    'Hello from MooseDocs'

    """

    DEFAULT_PF_CLASS = pf.Doc
    ENSURE_CHILDREN_ARE_BLOCK = True

    def __init__(self, **kwargs):
        """
        Construct MarkdownDocument.

        Parameters
        ----------
        **kwargs
            Keyword arguments passed to `MarkdownNode`.

        """
        super().__init__(None, name="MarkdownDocument", **kwargs)


class Text(MarkdownNode):
    r"""
    Inline textual element that optionally accepts raw Markdown.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> para = markdown.Paragraph()
    >>> markdown.Text(para, content="*formatted*", raw=False)
    >>> markdown.MarkdownNode(para, "Space")
    >>> markdown.Text(para, content="**bold**", raw=True)
    >>> para.write()
    '\*formatted\* **bold**'

    """

    DEFAULT_PF_CLASS = pf.Str
    DEFAULT_PF_KWARGS = {"text": "content"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        content: str = "",
        raw: bool = False,
        **kwargs,
    ):
        """
        Construct Text MarkdownNode.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node receiving the text.
        content : str, default=""
            Textual payload.
        raw : bool, default=False
            When True, treat the content as raw CommonMark.
        **kwargs
            Additional keyword arguments forwarded to `MarkdownNode`.

        """
        super().__init__(parent, content=content, **kwargs)
        if raw:
            self._pf_cls = pf.RawInline
            self["pf_kwargs"]["format"] = "commonmark"


# Float classes necessary since parent-child relationship is reversed in panflute
class Float(MarkdownNode):
    """
    Container used for figures or floats.

    Rendered parent resides inside the child in panflute's model.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> flt = markdown.Float()
    >>> caption = markdown.Caption(flt)
    >>> markdown.Text(caption, content="Figure caption")
    >>> img = markdown.Image(flt, src="diagram.png")
    >>> img.write()
    '![Figure caption](diagram.png)'

    """

    # This is arbitrary since we won't be using it
    DEFAULT_PF_CLASS = pf.Figure

    ENSURE_CHILDREN_ARE_BLOCK = True

    def to_panflute(self):
        """
        Build panflute tree for all children elements.

        This is overridden since this class is an empty container that enables
        the parent-children structure in the floats extension.

        Returns
        -------
        list of pf.Element
            Rendered children because the float itself is not emitted.

        """
        return self._materialize_children()


# Convenience factories mirroring common markdown constructs
class Paragraph(MarkdownNode):
    """
    Paragraph block node.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> para = markdown.Paragraph()
    >>> markdown.Text(para, content="Block quote entry")
    >>> para.write()
    'Block quote entry'

    """

    DEFAULT_PF_CLASS = pf.Para


class Heading(MarkdownNode):
    """
    Heading block node with level and identifier support.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> head = markdown.Heading(level=2, id="sec-demo")
    >>> markdown.Text(head, content="API Overview")
    >>> head.write()
    '## API Overview {#sec-demo}'

    """

    DEFAULT_PF_CLASS = pf.Header
    DEFAULT_PF_KWARGS = {"level": "level", "identifier": "id"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        level: int = 1,
        id: str = "",
        **kwargs,
    ):
        """
        Construct Header MarkdownNode.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent container, typically the document or section.
        level : int, default=1
            Heading depth (1 == H1).
        id : str, default=""
            Explicit identifier that maps to the CommonMark heading ID.
        **kwargs
            Additional keyword arguments forwarded to `MarkdownNode`.

        """
        super().__init__(parent, level=int(level), id=str(id), **kwargs)


class Code(Text):
    r"""
    Inline code span.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> code = markdown.Code(content='print(\"Hello\")')
    >>> code.write()
    '`print(\"Hello\")`'

    """

    DEFAULT_PF_CLASS = pf.Code


class CodeBlock(MarkdownNode):
    r"""
    Fenced code block supporting optional language classes.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> block = markdown.CodeBlock(content='print(\"Hello\")', language="python")
    >>> print(block.write())
    ``` python
    print(\"Hello\")
    ```

    """

    DEFAULT_PF_CLASS = pf.CodeBlock
    DEFAULT_PF_KWARGS = {"text": "content", "classes": "language"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        content: str = "",
        language: str = "",
        **kwargs,
    ):
        """
        Construct CodeBlock.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node receiving the block.
        content : str, default=""
            Source code placed inside the fence.
        language : str, default=""
            Lexer/style hint stored as a class.
        **kwargs
            Additional keyword arguments forwarded to `MarkdownNode`.

        """
        super().__init__(
            parent,
            content=content or "",
            language=[language] if language else [""],
            **kwargs,
        )


class Link(MarkdownNode):
    """
    Hyperlink wrapper with URL accessor.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> link = markdown.Link(url="https://mooseframework.inl.gov/")
    >>> markdown.Text(link, content="MOOSE")
    >>> link.write()
    '[MOOSE](https://mooseframework.inl.gov/)'

    """

    DEFAULT_PF_CLASS = pf.Link
    DEFAULT_PF_KWARGS = {"url": "url"}

    @property
    def url(self) -> str:
        """Link path from panflute constructor arguments."""
        return self["pf_kwargs"].get("url", None)

    @url.setter
    def url(self, value: Optional[str]):
        """Set link path of panflute constructor argument."""
        if value is not None:
            self["pf_kwargs"]["url"] = value
        elif "url" in self["pf_kwargs"]:
            self["pf_kwargs"].pop("url")


class ListItem(MarkdownNode):
    """
    Single item inside ordered or unordered lists.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> ul = markdown.MarkdownNode(pf_cls="BulletList")
    >>> markdown.Text(markdown.ListItem(ul), content="Item 0")
    >>> ul.write()
    '- Item 0'

    """

    DEFAULT_PF_CLASS = pf.ListItem
    ENSURE_CHILDREN_ARE_BLOCK = True


class Table(MarkdownNode):
    """
    Markdown table node that wires MooseDocs rows into panflute.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> table = markdown.Table(alignment=["left", "right"])
    >>> head = markdown.MarkdownNode(table, pf_cls="TableHead")
    >>> header_row = markdown.MarkdownNode(head, pf_cls="TableRow")
    >>> for title in ("Name", "Value"):
    ...     markdown.Text(markdown.TableCell(header_row), content=title)
    >>> body = markdown.MarkdownNode(table, pf_cls="TableBody")
    >>> row = markdown.MarkdownNode(body, pf_cls="TableRow")
    >>> markdown.Text(markdown.TableCell(row), content="foo")
    >>> markdown.Text(markdown.TableCell(row), content="42")
    >>> print(table.write())
    | Name | Value |
    | :--- | ----: |
    | foo  | 42    |

    """

    DEFAULT_PF_CLASS = pf.Table
    DEFAULT_PF_KWARGS = {"colspec": "alignment"}

    ALIGNMENT_MAP = {
        "left": "AlignLeft",
        "right": "AlignRight",
        "center": "AlignCenter",
        "l": "AlignLeft",
        "r": "AlignRight",
        "c": "AlignCenter",
    }

    def __init__(self, parent=None, alignment=None, **kwargs):
        """
        Construct Table.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Node owning the table.
        alignment : Iterable[str], optional
            Column alignment specifiers (`l`, `c`, `r`, `left`, etc.).
        **kwargs
            Additional keyword arguments forwarded to `MarkdownNode`.

        """
        if alignment is not None:
            alignment = [
                (self.ALIGNMENT_MAP[a.lower()], "ColWidthDefault") for a in alignment
            ]

        super().__init__(parent, alignment=alignment, **kwargs)

    def to_panflute(self):
        """
        Build panflute Table class.

        Returns
        -------
        pf.Table
            Panflute table composed of the collected head/body children.

        Raises
        ------
        ValueError
            If the children are not valid table fragments.

        """
        head = None
        bodies = []
        for child in self._materialize_children():
            if isinstance(child, pf.TableHead):
                head = child
            elif isinstance(child, pf.TableBody):
                bodies.append(child)
            else:
                raise ValueError(
                    f"Children of {self.name} must be `TableHead` or `TableBody`, "
                    f"not {type(child)}"
                )
        return self._pf_cls(*bodies, head=head, **self["pf_kwargs"])


class TableCell(MarkdownNode):
    """
    Single table cell that ensures block-level children.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> table = markdown.Table(alignment=["left"])
    >>> head = markdown.MarkdownNode(table, pf_cls="TableHead")
    >>> row = markdown.MarkdownNode(head, pf_cls="TableRow")
    >>> cell = markdown.TableCell(row)
    >>> markdown.Text(cell, content="Header")

    """

    DEFAULT_PF_CLASS = pf.TableCell
    ENSURE_CHILDREN_ARE_BLOCK = True


class Caption(MarkdownNode):
    """
    Caption wrapper used above floats or tables.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> flt = markdown.Float()
    >>> caption = markdown.Caption(flt)
    >>> caption.id = "label"
    >>> markdown.Text(caption, content="Awesome Caption")
    >>> p = markdown.Paragraph(flt)
    >>> markdown.Text(p, content="Hello World!")
    >>> print(flt.write())
    [Awesome Caption]{#label}

    Hello World!

    """

    DEFAULT_PF_CLASS = pf.Span


class Icon(Text):
    """Maps Material-esque icon names to Unicode emoji fallbacks."""

    ICON_EMOJI_DICT = {
        "help": b"\xe2\x9d\x93",  # Red question mark
        "home": b"\xf0\x9f\x8f\xa0",  # House
        "error": b"\xe2\x9d\x97",  # Red exclamation mark
        "error_outline": b"\xe2\x9d\x97",  # Red exclamation mark
        "warning": b"\xe2\x9a\xa0\xef\xb8\x8f",  # Warning
        "arrow_back": b"\xe2\xac\x85",  # Left arrow
        "arrow_forward": b"\xe2\x9e\xa1",  # Right arrow
        "comment": b"\xf0\x9f\x92\xac",  # Speech balloon
        "build": b"\xf0\x9f\x94\xa7",  # Wrench
        "school": b"\xf0\x9f\x8e\x93",  # Graduation Cap
        "report": b"\xf0\x9f\x9b\x91",  # Stop sign
        "device_hub": b"\xf0\x9f\x8c\x90",  # Globe with Meridians
        "storage": b"\xf0\x9f\x93\x82",  # Open file folder
        "computer": b"\xf0\x9f\x92\xbb",  # Laptop computer
        "flash_on": b"\xe2\x9a\xa1",  # High voltage
        "group": b"\xf0\x9f\x91\xa5",  # Busts in silhouette
        "toys": b"\xf0\x9f\xaa\x80",  # Yo-yo
        "settings": b"\xe2\x9a\x99",  # Gear
        "assessment": b"\xf0\x9f\x93\x8a",  # Bar chart
    }

    def __init__(self, parent: Optional[MarkdownNode] = None, icon: str = "", **kwargs):
        """
        Construct Icon node.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node to receive the icon.
        icon : str, default=""
            Case-insensitive icon name defined in `ICON_EMOJI_DICT`.
        **kwargs
            Additional keyword arguments forwarded to `Text`.

        """
        icon = icon.lower().replace(" ", "_")
        content = self.ICON_EMOJI_DICT.get(
            icon, b"\xe2\x98\x90"
        )  # Default is empty box
        super().__init__(parent, content=content.decode("utf-8"), **kwargs)


class Alert(MarkdownNode):
    """
    Block quote wrapper used for informational alert callouts.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> alert = markdown.Alert()
    >>> markdown.Text(alert, content='[!NOTE]', raw=True)
    >>> markdown.MarkdownNode(alert, "LineBreak")
    >>> markdown.Text(markdown.Paragraph(alert), content='Details')
    >>> print(alert.write())
    > [!NOTE]
    > Details

    """

    DEFAULT_PF_CLASS = pf.BlockQuote
    ENSURE_CHILDREN_ARE_BLOCK = True


class Cite(Text):
    """
    Inline citation marker backed by Markdown footnotes.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> para = markdown.Paragraph()
    >>> markdown.Text(para, content="See reference ")
    >>> markdown.Cite(para, key="foo")
    >>> para.write()
    'See reference [^foo]'

    """

    def __init__(self, parent: Optional[MarkdownNode] = None, key: str = "", **kwargs):
        """
        Construct Cite node.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node receiving the citation.
        key : str, default=""
            Footnote key that anchors the citation.
        **kwargs
            Additional keyword arguments forwarded to `Text`.

        Raises
        ------
        ValueError
            If a citation key is not provided.

        """
        if not key:
            raise ValueError("Citation requires a key.")
        content = f"[^{key}]"
        super().__init__(parent, content, True, **kwargs)


class Bibliography(MarkdownNode):
    """
    Container responsible for collecting and emitting citations.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> bib = markdown.Bibliography(doc)
    >>> bib.add_citation("foo", "Jane Doe. Example entry.")
    >>> bib.add_citation("bar", "Albert Einstein. Another example.")
    >>> print(bib.write())
    [^foo]: Jane Doe. Example entry.

    [^bar]: Albert Einstein. Another example.

    """

    DEFAULT_PF_CLASS = pf.Para

    def __init__(self, parent: Optional[MarkdownNode] = None, **kwargs):
        """
        Construct Bibliography node.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node that will contain the bibliography entries.
        **kwargs
            Additional keyword arguments forwarded to `MarkdownNode`.

        """
        self._citations: dict[str, Text] = {}
        super().__init__(parent, **kwargs)

    def add_citation(self, key: str, raw_markdown: str) -> Text:
        """
        Register or retrieve a citation node.

        Parameters
        ----------
        key : str
            Citation key used for both the footnote marker and entry.
        raw_markdown : str
            Markdown content appended after the `[^{key}]:` prefix.

        Returns
        -------
        Text
            The `Text` node storing the raw footnote entry.

        """
        if key not in self._citations:
            content = f"[^{key}]: "
            indent = ""
            for line in raw_markdown.splitlines(keepends=True):
                content += indent + line
                indent = "    "

        self._citations[key] = Text(self, content=content, raw=True)
        return self._citations[key]

    def to_panflute(self):
        """
        Build a Paragraph panflute object for every citation added.

        Returns
        -------
        list of pf.Para
            The rendered bibliography entries as paragraph blocks.

        """
        return [
            self._pf_cls(child, **self["pf_kwargs"])
            for child in self._materialize_children()
        ]


class Image(MarkdownNode):
    """
    Image node that automatically hoists captions from float parents.

    Examples
    --------
    >>> from MooseDocs.tree import markdown
    >>> figure = markdown.Float()
    >>> caption = markdown.Caption(figure)
    >>> markdown.Text(caption, content="System diagram")
    >>> img = markdown.Image(figure, src="diagram.png")
    >>> img.id = label
    >>> img.write()
    '![System diagram](diagram.png){#label}'

    """

    DEFAULT_PF_CLASS = pf.Image
    DEFAULT_PF_KWARGS = {"url": "src"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        src: str = "",
        **kwargs,
    ):
        """
        Construct Image Node.

        Parameters
        ----------
        parent : MarkdownNode, optional
            Parent node; often a `Float`.
        src : str, default=""
            Path or URL to the image asset.
        **kwargs
            Additional keyword arguments forwarded to `MarkdownNode`.

        """
        super().__init__(parent, src=src, **kwargs)

        # Get caption
        if isinstance(parent, Float):
            for child in parent.children:
                if isinstance(child, Caption):
                    # Set this as caption's parent
                    child.parent = self
                    # Retrieve ID and reset caption ID
                    self.id = child.id
                    child.id = None
                    break
