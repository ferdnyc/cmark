#include <stdlib.h>

#include "cmark.h"
#include "buffer.h"

void cmark_block_parser_free(cmark_block_parser *block_parser) {
  free(block_parser->name);
  free(block_parser);
}

cmark_block_parser *cmark_block_parser_new(const char *name) {
  cmark_block_parser *res = calloc(1, sizeof(cmark_block_parser));
  cmark_strbuf name_buf = GH_BUF_INIT;
  cmark_strbuf_sets(&name_buf, name);
  res->name = (char *) cmark_strbuf_detach(&name_buf);
  return res;
}
