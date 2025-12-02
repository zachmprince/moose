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
        elem = self.to_panflute()
        if isinstance(elem, pf.Doc):
            doc = elem
        elif isinstance(elem, pf.Block):
            doc = pf.Doc(elem)
        else:
            doc = pf.Doc(pf.Plain(elem))
        # convert_text understands pf.Doc instances, so this yields raw markdown
        return pf.convert_text(doc, input_format="panflute", output_format="gfm")


class MarkdownDocument(MarkdownNode):
    """Root container that owns the panflute Doc."""

    DEFAULT_PF_CLASS = pf.Doc
    ENSURE_CHILDREN_ARE_BLOCK = True

    def __init__(self, **kwargs):
        super().__init__(None, name="MarkdownDocument", **kwargs)


class Text(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Str
    DEFAULT_PF_KWARGS = {"text": "content"}


# Float classes necessary since parent-child relationship is reversed in panflute
class TableFloat(MarkdownNode):
    # This is arbitrary since we won't be using it
    DEFAULT_PF_CLASS = pf.Table

    def to_panflute(self):
        table: "Table" = None
        caption: "Caption" = None
        for child in self.children:
            assert isinstance(child, (Table, Caption))
            if isinstance(child, Table):
                assert table is None
                table = child
            elif isinstance(child, Caption):
                assert caption is None
                caption = child

        if caption is not None:
            table["pf_kwargs"]["caption"] = caption.to_panflute()
        table.id = self.id
        return table.to_panflute()


# Convenience factories mirroring common markdown constructs
class Paragraph(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Para


class Heading(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Header
    DEFAULT_PF_KWARGS = {"level": "level"}


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
    DEFAULT_PF_CLASS = pf.Caption
    ENSURE_CHILDREN_ARE_BLOCK = True
