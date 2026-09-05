# STEN - Structured Temporary Exchange Notation

STEN is a minimal, human-readable text format for storing a list of string
records. each record has a role, an optional tag, and content. it is designed
to be easily edited by hand and parsed/generated programmatically

the library is a single-header C implementation suitable for embedding in
applications

## TOC

- [license](#license)
- [format](#format)
  - [header syntax](#header-syntax)
  - [content](#content)
  - [example](#example)
- [library usage](#library-usage)
  - [customization](#customization)
  - [API reference](#api-reference)
    - [parsing and cleanup](#parsing-and-cleanup)
    - [accessing items](#accessing-items)
    - [getters](#getters)
    - [generation](#generation)
  - [example](#example-1)
- [building](#building)
- [developer notes](#developer-notes)

## license

> MIT License
>
> Copyright (c) 2026 NezertorcheaT
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

---

## format

STEN document consists of a sequence of items. each item starts with a line
containing a separator (by default `==== `) followed by a header. the header
defines the role and optionally a tag. after the header line, all following
lines until the next separator line form the item's content

### header syntax

```
<separator><role>
```

or with a tag:

```
<separator><role>:<tag>
```

- `<separator>` - configurable string (default `"==== "`). It must appear at
  the beginning of a line, with no leading white space
- `<role>` - non-empty string identifying the role of the item (`user`,
  `assistant`, or anything). it cannot contain `:` or a newline
- `<tag>` (optional) - string after a colon (`:`) that provides additional
  classification. it can contain spaces and any character except newline. a tag
  may be empty (`role:`)
- header line ends with a newline (`\n`) or `\r\n`

### content

- content begins on the next line after the header
- content continues until the next line that starts with the separator string,
  or the end of the file
- content may be empty (no lines before the next separator)
- content preserves all characters including newlines, except that a trailing
  newline before the next separator is not considered part of the content. the
  parser trims trailing newlines if they are directly before the next separator
  line (i.e., the content does not include the final newline that separates it
  from the next item)
- if the content ends at the end of the file, trailing newlines may be included
  depending on the file; it is recommended to always end the file with a
  newline after the last content block

### Example

```
==== assistant
hello i am AI assistant! how can i help you?
==== user
write me a python hello world
==== assistant:helpful tag
print('hello world')

there is your programm!
==== user:input

```

this defines four items:

1. role: `assistant`, no tag, content: `hello i am AI assistant! how can i help you today?\n`
2. role: `user`, no tag, content: `write me a python hello world\n`
3. role: `assistant`, tag: `helpful tag`, content: `print('hello world')\n\nthere is your programm!\n`
4. role: `user`, tag: `input`, content: empty (zero-length)

---

## library usage

library is contained in a single header file `sten.h`. to use it:

1. include the header in any source file where you need the API:

```c
#include "sten.h"
```

2. in exactly one `.c` file, define the implementation macro before including the header:

```c
#define STEN_IMPLEMENTATION
#include "sten.h"
```

that's it. no external dependencies are required

### customization

you can customize the separator and the memory allocator by defining macros
before including the header.

custom separator:

```c
#define STEN_SEPARATOR ">> "
#define STEN_IMPLEMENTATION
#include "sten.h"
```

custom allocator:

```c
#define STEN_malloc(X) arena_malloc(&ar,X)
#define STEN_free(X) arena_free(&ar,X)
#define STEN_IMPLEMENTATION
#include "sten.h"
```

allocator functions must resemble signatures of `malloc` and `free`

### API reference

all functions operate on a linked list of `sten_item` structures

```c
typedef struct sten_item {

  // position in list (0-based)
  int index;

  // required
  char *role;

  // may be NULL
  char *tag;

  // raw block content
  char *content;

  // for internal use
  struct sten_item *next;

} sten_item;
```

#### parsing and cleanup

parses a NUL-terminated string containing STEN data and returns a linked list
of items. returns `NULL` on error. caller must sten_Delete result

```c
sten_item *sten_Parse(const char *value);
```

frees the entire list and all associated strings

```c
void sten_Delete(sten_item *item);
```

#### accessing items

returns the number of items in the list

```c
int sten_GetArraySize(const sten_item *item);
```

returns the item at the given index (0-based), or `NULL` if out of range

```c
sten_item *sten_GetArrayItem(const sten_item *item, int index);
```

returns the first item whose tag exactly matches the given string. if `tag` is
`NULL`, returns the first item that has no tag

```c
sten_item *sten_FindByTag(const sten_item *item, const char *tag);
```

returns the first item with the exact given role

```c
sten_item *sten_FindByRole(const sten_item *item, const char *role);
```

#### getters

```c
const char *sten_GetRole(const sten_item *item);
const char *sten_GetTag(const sten_item *item);
const char *sten_GetContent(const sten_item *item);
```

return the respective fields, or `NULL` if the item is `NULL`

#### generation

serializes the list of items into a STEN-formatted string. returned string is
allocated with the configured allocator and must be freed by the caller using
`STEN_free`

```c
char *sten_Print(const sten_item *item);
```

### example

```c
#define STEN_IMPLEMENTATION
#include "sten.h"

int main() {
  const char *input = "==== assistant\n"
                      "hello i am AI assistant! how can i help you?\n"
                      "==== user\n"
                      "write me a python hello world\n"
                      "==== assistant:helpful tag\n"
                      "print('hello world')\n"
                      "\n"
                      "there is your programm!\n"
                      "==== user:input\n"
                      "\n";

  sten_item *items = sten_Parse(input);
  if (!items)
    return 1;

  sten_item *tagged = sten_FindByTag(items, "helpful tag");
  if (tagged) {
    printf("Found tag: %s\n", sten_GetTag(tagged));
    printf("Content: %s\n", sten_GetContent(tagged));
  }

  char *serialized = sten_Print(items);
  printf("%s", serialized);
  STEN_free(serialized);

  sten_Delete(items);
  return 0;
}
```

---

## building

no special build system is required. add `sten.h` to your project and compile
your source files as usual

if you want to compile the implementation in a separate translation unit,
create a file `sten.c`:

```c
#define STEN_IMPLEMENTATION
#include "sten.h"
```

then compile both `myapp.c` and `sten.c` and link them.

the library is written in standard C (C89/C90 compatible) and is cross‑platform
(Windows, Linux, macOS, etc.)

---

## developer notes

- the parser expects the input string to be well-formed. Malformed input may
  cause partial memory allocation and will return `NULL` after freeing any
  partially constructed list
- the separator is compared verbatim; it must appear exactly at the beginning
  of a line (after the previous newline or at the start of the string)
- content is stored as a NUL-terminated string, which means binary content
  containing NUL bytes is not supported. STEN is intended for text data
- the library does not perform any escaping. if your content contains the
  separator string at the beginning of a line, it will be interpreted as a new
  item. this is a limitation of the format. the parser cannot distinguish such
  content from a separator line. if you need to store such content, consider
  changing the separator to a string that does not appear naturally in your
  data
- the tag comparison is case-sensitive and exact; no wildcards are supported
- the generator (`sten_Print`) ensures that each item's content ends with
  exactly one newline before the next separator, except possibly after the last
  item. trailing newlines in the original content are preserved as part of the
  content (the parser strips only the newline that directly precedes the next
  separator line, so round-tripping is generally consistent)
