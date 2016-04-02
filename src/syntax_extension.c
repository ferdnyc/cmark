#include <stdlib.h>

#include "cmark.h"
#include "buffer.h"

void cmark_syntax_extension_free(cmark_syntax_extension *extension) {
  if (extension->free_function && extension->priv) {
    extension->free_function(extension->priv);
  }

  cmark_llist_free(extension->special_inline_chars);
  free(extension->name);
  free(extension);
}

cmark_syntax_extension *cmark_syntax_extension_new(const char *name) {
  cmark_syntax_extension *res = calloc(1, sizeof(cmark_syntax_extension));
  cmark_strbuf name_buf = GH_BUF_INIT;
  cmark_strbuf_sets(&name_buf, name);
  res->name = (char *) cmark_strbuf_detach(&name_buf);
  return res;
}
