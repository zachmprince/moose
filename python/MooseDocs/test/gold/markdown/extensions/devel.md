# Devel Extension {#devel-extension}

## Examples {#examples}

``` text
This is a +test+ without float.
```

This is a **test** without float.

[Example 1: The example command.]{#markdown-example}

``` text
This is a +test+.
```

This is a **test**.

[Example 2: The example command with block format.]{#markdown-example2}

``` text
First paragraph.

Second paragraph.
```

First paragraph.

Second paragraph.

## Settings {#settings}

The available settings for code blocks are listed in
[Table 1](#code-settings).

| Key      | Default | Description                                              |
|:---------|:--------|:---------------------------------------------------------|
| style    | None    | The style settings that are passed to rendered HTML tag. |
| class    | None    | The class settings to be passed to rendered HTML tag.    |
| id       | None    | Identifier to link against this object.                  |
| language | text    | The code language to use for highlighting.               |

[Table 1: Settings for code blocks.]{#code-settings}

| Key      | Default | Description                                              |
|:---------|:--------|:---------------------------------------------------------|
| style    | None    | The style settings that are passed to rendered HTML tag. |
| class    | None    | The class settings to be passed to rendered HTML tag.    |
| id       | None    | Identifier to link against this object.                  |
| language | text    | The code language to use for highlighting.               |

## Eq. in Example {#eq-in-example}

Test with Equations

``` text
[foo] is famous.

!equation id=foo
E = mc^2
```

[Eq. (1)](#foo) is famous.

$$  
\label{foo}  
E = mc^2  
$$  
  