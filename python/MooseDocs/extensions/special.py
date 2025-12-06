# This file is part of the MOOSE framework
# https://mooseframework.inl.gov
#
# All rights reserved, see COPYRIGHT for full restrictions
# https://github.com/idaholab/moose/blob/master/COPYRIGHT
#
# Licensed under LGPL 2.1, please see LICENSE for details
# https://www.gnu.org/licenses/lgpl-2.1.html
import re

from ..base import components, Extension
from ..tree import tokens, markdown
from ..extensions import core


def make_extension(**kwargs):
    """
    Create and return the CoreExtension object for converting from markdown to html/latex.
    """
    return SpecialExtension(**kwargs)


HTMLCode = tokens.newToken("HTMLCode", tokens.String)


class SpecialExtension(Extension):
    @staticmethod
    def defaultConfig():
        """CoreExtension configuration options."""
        config = Extension.defaultConfig()
        return config

    def extend(self, reader, renderer):
        reader.addInline(HTMLNumberCode(), location="<PunctuationInline")
        reader.addInline(HTMLEntityCode(), location="<PunctuationInline")

        renderer.add("HTMLCode", RenderHTMLCode())


class HTMLNumberCode(components.ReaderComponent):
    RE = re.compile(r"(?P<code>&#[0-9]+;)")

    def createToken(self, parent, info, page, settings):
        HTMLCode(parent, content=info["code"], escape=False)
        return parent


class HTMLEntityCode(components.ReaderComponent):
    RE = re.compile(r"(?P<code>&[A-Za-z0-9]+;)")

    def createToken(self, parent, info, page, settings):
        HTMLCode(parent, content=info["code"], escape=False)
        return parent


class RenderHTMLCode(core.RenderString):
    def createMarkdown(self, parent, token, page):
        import html

        return markdown.Text(parent, content=html.unescape(token["content"]))
