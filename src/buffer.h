#ifndef CMARK_BUFFER_H
#define CMARK_BUFFER_H

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "cmark.h"

#ifdef __cplusplus
extern "C" {
#endif

#define bufsize_t cmark_bufsize_t

struct cmark_strbuf {
  unsigned char *ptr;
  bufsize_t asize, size;
};

extern unsigned char cmark_strbuf__initbuf[];

#define GH_BUF_INIT                                                            \
  { cmark_strbuf__initbuf, 0, 0 }
#define BUFSIZE_MAX INT_MAX

/** Initialize a cmark_strbuf structure.
 *
 * For the cases where GH_BUF_INIT cannot be used to do static
 * initialization.
 */
void cmark_strbuf_init(cmark_strbuf *buf, cmark_bufsize_t initial_size);

static CMARK_INLINE const char *cmark_strbuf_cstr(const cmark_strbuf *buf) {
  return (char *)buf->ptr;
}

/* Print error and abort. */
void cmark_strbuf_overflow_err(void);

static CMARK_INLINE bufsize_t cmark_strbuf_check_bufsize(size_t size) {
  if (size > BUFSIZE_MAX) {
    cmark_strbuf_overflow_err();
  }
  return (bufsize_t)size;
}

static CMARK_INLINE bufsize_t cmark_strbuf_safe_strlen(const char *str) {
  return cmark_strbuf_check_bufsize(strlen(str));
}

unsigned char *cmark_strbuf_detach(cmark_strbuf *buf);

void cmark_strbuf_release(cmark_strbuf *buf);

#define cmark_strbuf_at(buf, n) ((buf)->ptr[n])

#ifdef __cplusplus
}
#endif

#endif
