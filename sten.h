/*

MIT License

Copyright (c) 2026 NezertorcheaT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/*

STEN (Structured Temporary Exchange Notation) parser/generator
single header library
define STEN_IMPLEMENTATION in one .c file to get implementation
define STEN_SEPARATOR customize separator (default "==== ")
define STEN_malloc, STEN_free to override allocator

*/

#ifndef STEN_H_
#define STEN_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STEN_SEPARATOR
#define STEN_SEPARATOR "==== "
#endif

#ifndef STEN_malloc
#define STEN_malloc(X) malloc(X)
#endif
#ifndef STEN_free
#define STEN_free(X) free(X)
#endif

// main item structure
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

// parses a NUL-terminated string containing STEN data and returns a linked
// list of items. returns `NULL` on error. caller must sten_Delete result
sten_item *sten_Parse(const char *value);

// frees the entire list and all associated strings
void sten_Delete(sten_item *item);

// returns the number of items in the list
int sten_GetArraySize(const sten_item *item);

// returns the item at the given index (0-based), or `NULL` if out of range
sten_item *sten_GetArrayItem(const sten_item *item, int index);

// returns the first item whose tag exactly matches the given string. if `tag`
// is `NULL`, returns the first item that has no tag
sten_item *sten_FindByTag(const sten_item *item, const char *tag);

// returns the first item with the exact given role
sten_item *sten_FindByRole(const sten_item *item, const char *role);

const char *sten_GetRole(const sten_item *item);
const char *sten_GetTag(const sten_item *item);
const char *sten_GetContent(const sten_item *item);

// serializes the list of items into a STEN-formatted string. returned string
// is allocated with the configured allocator and must be freed by the caller
// using `STEN_free`
char *sten_Print(const sten_item *item);

#ifdef __cplusplus
}

#endif

#endif // !STEN_H_

#ifdef STEN_IMPLEMENTATION
#undef STEN_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

static char *sten_strdup(const char *str) {
  if (!str)
    return NULL;
  size_t len = strlen(str);
  char *copy = (char *)STEN_malloc(len + 1);
  if (copy)
    memcpy(copy, str, len + 1);
  return copy;
}

sten_item *sten_Parse(const char *value) {
  if (!value)
    return NULL;

  const char *ptr = value;
  const size_t sep_len = strlen(STEN_SEPARATOR);
  sten_item *head = NULL;
  sten_item *tail = NULL;
  int idx = 0;

  while (*ptr) {
    if (!((ptr == value) || (ptr[-1] == '\n')) ||
        strncmp(ptr, STEN_SEPARATOR, sep_len) != 0)
      goto error;

    const char *header_start = ptr + sep_len;
    const char *header_end = header_start;

    while (*header_end && *header_end != '\n' && *header_end != '\r')
      header_end++;

    size_t header_len = header_end - header_start;
    if (header_len == 0)
      goto error;

    char *role = NULL;
    char *tag = NULL;
    const char *colon = NULL;
    for (const char *p = header_start; p < header_end; p++) {
      if (*p == ':') {
        colon = p;
        break;
      }
    }
    if (colon) {
      size_t role_len = colon - header_start;

      role = (char *)STEN_malloc(role_len + 1);
      if (!role)
        goto error;

      memcpy(role, header_start, role_len);
      role[role_len] = '\0';

      size_t tag_len = header_end - (colon + 1);

      tag = (char *)STEN_malloc(tag_len + 1);
      if (!tag) {
        STEN_free(role);
        goto error;
      }

      memcpy(tag, colon + 1, tag_len);
      tag[tag_len] = '\0';

    } else {
      size_t role_len = header_len;

      role = (char *)STEN_malloc(role_len + 1);
      if (!role)
        goto error;

      memcpy(role, header_start, role_len);
      role[role_len] = '\0';
    }

    if (*header_end == '\r')
      ptr = (header_end[1] == '\n') ? header_end + 2 : header_end + 1;
    else if (*header_end == '\n')
      ptr = header_end + 1;
    else
      ptr = header_end;

    const char *content_start = ptr;
    const char *content_end = NULL;
    const char *scan = ptr;
    while (*scan) {
      if ((scan == value) || (scan[-1] == '\n')) {
        if (strncmp(scan, STEN_SEPARATOR, sep_len) == 0) {
          content_end = scan;
          break;
        }
      }
      scan++;
    }
    if (!content_end)
      content_end = scan;

    size_t content_len = content_end - content_start;
    char *content = (char *)STEN_malloc(content_len + 1);
    if (!content) {
      STEN_free(role);
      STEN_free(tag);
      goto error;
    }
    memcpy(content, content_start, content_len);
    content[content_len] = '\0';

    sten_item *item = (sten_item *)STEN_malloc(sizeof(sten_item));
    if (!item) {
      STEN_free(role);
      STEN_free(tag);
      STEN_free(content);
      goto error;
    }

    item->index = idx++;
    item->role = role;
    item->tag = tag;
    item->content = content;
    item->next = NULL;

    if (!head)
      head = tail = item;
    else {
      tail->next = item;
      tail = item;
    }

    ptr = content_end;
    if (*ptr == '\0')
      break;
  }

  return head;

error:
  sten_item *cur = head;
  while (cur) {
    sten_item *next = cur->next;
    STEN_free(cur->role);
    STEN_free(cur->tag);
    STEN_free(cur->content);
    STEN_free(cur);
    cur = next;
  }
  return NULL;
}

void sten_Delete(sten_item *item) {
  while (item) {
    sten_item *next = item->next;
    STEN_free(item->role);
    STEN_free(item->tag);
    STEN_free(item->content);
    STEN_free(item);
    item = next;
  }
}

int sten_GetArraySize(const sten_item *item) {
  int count = 0;
  while (item) {
    count++;
    item = item->next;
  }
  return count;
}

sten_item *sten_GetArrayItem(const sten_item *item, int index) {
  while (item && item->index != index)
    item = item->next;
  return (sten_item *)item;
}

sten_item *sten_FindByTag(const sten_item *item, const char *tag) {
  while (item) {
    if ((item->tag && tag && strcmp(item->tag, tag) == 0) ||
        (item->tag == NULL && tag == NULL))
      return (sten_item *)item;

    item = item->next;
  }
  return NULL;
}

sten_item *sten_FindByRole(const sten_item *item, const char *role) {
  while (item) {
    if (item->role && role && strcmp(item->role, role) == 0)
      return (sten_item *)item;
    item = item->next;
  }
  return NULL;
}

const char *sten_GetRole(const sten_item *item) {
  return item ? item->role : NULL;
}
const char *sten_GetTag(const sten_item *item) {
  return item ? item->tag : NULL;
}
const char *sten_GetContent(const sten_item *item) {
  return item ? item->content : NULL;
}

char *sten_Print(const sten_item *item) {
  const size_t sep_len = strlen(STEN_SEPARATOR);
  size_t total = 1;
  const sten_item *cur = item;
  while (cur) {
    total += sep_len + strlen(cur->role);
    if (cur->tag)
      total += 1 + strlen(cur->tag);
    total += 1;
    size_t content_len = cur->content ? strlen(cur->content) : 0;
    total += content_len;
    if (content_len == 0 || cur->content[content_len - 1] != '\n')
      total += 1;
    cur = cur->next;
  }

  char *out = (char *)STEN_malloc(total);
  if (!out)
    return NULL;
  char *p = out;

  cur = item;
  while (cur) {
    memcpy(p, STEN_SEPARATOR, sep_len);
    p += sep_len;
    size_t role_len = strlen(cur->role);
    memcpy(p, cur->role, role_len);
    p += role_len;
    if (cur->tag) {
      *p++ = ':';
      size_t tag_len = strlen(cur->tag);
      memcpy(p, cur->tag, tag_len);
      p += tag_len;
    }
    *p++ = '\n';
    if (cur->content) {
      size_t content_len = strlen(cur->content);
      memcpy(p, cur->content, content_len);
      p += content_len;
      if (content_len == 0 || cur->content[content_len - 1] != '\n')
        *p++ = '\n';
    } else {
      *p++ = '\n';
    }
    cur = cur->next;
  }
  *p = '\0';
  return out;
}

#endif // STEN_IMPLEMENTATION
