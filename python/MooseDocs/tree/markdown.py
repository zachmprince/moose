# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html

import importlib.util
from typing import Any, Optional, Type, TYPE_CHECKING

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

    def _materialize_children(self) -> list[pf.Element]:
        rendered = []
        for child in self.children:
            assert isinstance(child, MarkdownNode)

            result = child.to_panflute()
            if result is None:
                continue
            if isinstance(result, (list, tuple)):
                rendered.extend(result)
            else:
                rendered.append(result)
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
            doc = pf.Doc(pf.Para(elem))
        # convert_text understands pf.Doc instances, so this yields raw markdown
        return pf.convert_text(doc, input_format="panflute", output_format="gfm")


class MarkdownDocument(MarkdownNode):
    """Root container that owns the panflute Doc."""

    DEFAULT_PF_CLASS = pf.Doc

    def __init__(self, **kwargs):
        super().__init__(None, name="MarkdownDocument", **kwargs)

    def to_panflute(self):
        children = self._materialize_children()
        sent_children = []
        for child in children:
            if not isinstance(child, pf.Block):
                sent_children.append(pf.Para(child))
            else:
                sent_children.append(child)
        return self._pf_cls(*sent_children, **self["pf_kwargs"])


class Text(MarkdownNode):
    DEFAULT_PF_CLASS = pf.Str
    DEFAULT_PF_KWARGS = {"text": "content"}


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
