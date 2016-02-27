#include <stdlib.h>

#include "plugin.h"

bool cmark_plugin_register_block_parser(cmark_plugin    * plugin,
                                        cmark_block_parser * block_parser) {
  plugin->block_parsers = cmark_llist_append(plugin->block_parsers, block_parser);
  return true;
}

cmark_plugin *
cmark_plugin_new(void) {
  cmark_plugin *res = malloc(sizeof(cmark_plugin));

  res->block_parsers = NULL;

  return res;
}

void
cmark_plugin_free(cmark_plugin *plugin) {
  cmark_llist_free_full(plugin->block_parsers,
                        (CMarkListFreeFunc) cmark_block_parser_free);
  free(plugin);
}

cmark_llist *
cmark_plugin_steal_block_parsers(cmark_plugin *plugin) {
  cmark_llist *res = plugin->block_parsers;

  plugin->block_parsers = NULL;
  return res;
}
