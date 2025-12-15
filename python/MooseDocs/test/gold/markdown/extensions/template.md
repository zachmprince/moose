# Template Extension {#template-extension}

The MOOSE project is amazing!

## Field with Defaults {#field-with-defaults}

This is the default message, it is great.

## Field with Replacement {#field-with-replacement}

This is some content that should be below the second heading.

## Field with Missing Replacement {#field-with-missing-replacement}

> [!ERROR]  
> ❗ **Missing Template Item: \"field-without-item\"**
>
> This item should be supplied, if it isn\'t then you get an error.
>
> The document must include the \"field-without-item\" template item, this can be included by adding the following to the markdown file (extensions/template.md):
>
> ``` text
> !template! item key=field-without-item
> Include text (in MooseDocs format) regarding the "field-without-item" template item here.
> !template-end!
> ```