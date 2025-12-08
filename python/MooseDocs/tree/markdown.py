# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html

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


class MarkdownNode(NodeBase):
    """Generic panflute-backed node (similar in spirit to html.Tag)."""

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
        return super().children

    @property
    def id(self) -> str:
        return self["pf_kwargs"].get("identifier", None)

    @id.setter
    def id(self, val: Optional[str]):
        if val is not None:
            self["pf_kwargs"]["identifier"] = val
        else:
            self["pf_kwargs"].pop("identifier", None)

    def _materialize_children(self) -> list[pf.Element]:
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
        children = self._materialize_children()
        return self._pf_cls(*children, **self["pf_kwargs"])

    def write(self) -> str:
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
            output_format="commonmark_x+subscript+superscript-raw_html",
        )


class MarkdownDocument(MarkdownNode):
    """Root container that owns the panflute Doc."""

    DEFAULT_PF_CLASS = pf.Doc
    ENSURE_CHILDREN_ARE_BLOCK = True

    def __init__(self, **kwargs):
        super().__init__(None, name="MarkdownDocument", **kwargs)


class Text(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Str
    DEFAULT_PF_KWARGS = {"text": "content"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        content: str = "",
        raw: bool = False,
        **kwargs,
    ):
        super().__init__(parent, content=content, **kwargs)
        if raw:
            self._pf_cls = pf.RawInline
            self["pf_kwargs"]["format"] = "commonmark"


# Float classes necessary since parent-child relationship is reversed in panflute
class Float(MarkdownNode):
    # This is arbitrary since we won't be using it
    DEFAULT_PF_CLASS = pf.Figure

    ENSURE_CHILDREN_ARE_BLOCK = True

    def to_panflute(self):
        return self._materialize_children()


# Convenience factories mirroring common markdown constructs
class Paragraph(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Para


class Heading(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Header
    DEFAULT_PF_KWARGS = {"level": "level", "identifier": "id"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        level: int = 1,
        id: str = "",
        **kwargs,
    ):
        super().__init__(parent, level=level, id=str(id), **kwargs)


class Code(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Code
    DEFAULT_PF_KWARGS = {"text": "content"}


class CodeBlock(MarkdownNode):
    DEFAULT_PF_CLASS = pf.CodeBlock
    DEFAULT_PF_KWARGS = {"text": "content", "classes": "language"}

    def __init__(
        self,
        parent: Optional[MarkdownNode] = None,
        content: str = "",
        language: str = "",
        **kwargs,
    ):
        super().__init__(parent, content=content, language=[language], **kwargs)


class Link(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Link
    DEFAULT_PF_KWARGS = {"url": "url"}


class Table(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Table
    DEFAULT_PF_KWARGS = {"colspec": "alignment"}

    ALIGNMENT_MAP = {
        "left": "AlignLeft",
        "right": "AlignRight",
        "center": "AlignCenter",
    }

    def __init__(self, parent=None, alignment=None, **kwargs):
        if alignment is not None:
            alignment = [(self.ALIGNMENT_MAP[a], "ColWidthDefault") for a in alignment]

        super().__init__(parent, alignment=alignment, **kwargs)

    def to_panflute(self):
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
    DEFAULT_PF_CLASS = pf.TableCell
    ENSURE_CHILDREN_ARE_BLOCK = True


class Caption(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Span


class Icon(Text):
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
        icon = icon.lower().replace(" ", "_")
        content = self.ICON_EMOJI_DICT.get(
            icon, b"\xe2\x98\x90"
        )  # Default is empty box
        super().__init__(parent, content=content.decode("utf-8"), **kwargs)


class Alert(MarkdownNode):
    DEFAULT_PF_CLASS = pf.BlockQuote
    ENSURE_CHILDREN_ARE_BLOCK = True


class Cite(Text):

    def __init__(self, parent: Optional[MarkdownNode] = None, key: str = "", **kwargs):
        if not key:
            raise ValueError("Citation requires a key.")
        content = f"[^{key}]"
        super().__init__(parent, content, True, **kwargs)


class Bibliography(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Para

    def __init__(self, parent: Optional[MarkdownNode] = None, **kwargs):
        self._citations: dict[str, Text] = {}
        super().__init__(parent, **kwargs)

    def add_citation(self, key: str, raw_markdown: str) -> Text:
        if key not in self._citations:
            content = f"[^{key}]: "
            indent = ""
            for line in raw_markdown.splitlines(keepends=True):
                content += indent + line
                indent = "    "

        self._citations[key] = Text(self, content=content, raw=True)
        return self._citations[key]

    def to_panflute(self):
        return [
            self._pf_cls(child, **self["pf_kwargs"])
            for child in self._materialize_children()
        ]
