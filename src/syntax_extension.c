#include <stdlib.h>

#include "cmark.h"
#include "buffer.h"

void cmark_syntax_extension_free(cmark_syntax_extension *extension) {
  free(extension->name);
  free(extension);
}

cmark_syntax_extension *cmark_syntax_extension_new(const char *name) {
  extern cmark_mem DEFAULT_MEM_ALLOCATOR;
  cmark_syntax_extension *res = calloc(1, sizeof(cmark_syntax_extension));
  cmark_strbuf name_buf = CMARK_BUF_INIT(&DEFAULT_MEM_ALLOCATOR);
  cmark_strbuf_sets(&name_buf, name);
  res->name = (char *) cmark_strbuf_detach(&name_buf);
  return res;
}
