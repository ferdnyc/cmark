#include <stdio.h>
#include <string.h>

#include <cmark.h>
#include "ext_scanners.h"

static bool table_can_contain(cmark_block_parser *self, cmark_node *parent, cmark_node *child) {
  bool res = false;
  cmark_node_type child_type = cmark_node_get_type(child);

  if (child_type != CMARK_NODE_EXTENSION_BLOCK) {
    if (!strcmp(cmark_node_get_pretty_name(parent), "table_cell"))
      res = (child_type == CMARK_NODE_TEXT);
    else if (!strcmp(cmark_node_get_pretty_name(parent), "table_header_cell"))
      res = (child_type == CMARK_NODE_TEXT);
  } else {
    if (!strcmp(cmark_node_get_pretty_name(parent), "table_header")) {
      if (!strcmp(cmark_node_get_pretty_name(child), "table_header_cell"))
        res = true;
    } else if (!strcmp(cmark_node_get_pretty_name(parent), "table")) {
      if (!strcmp(cmark_node_get_pretty_name(child), "table_header"))
        res = true;
      else if (!strcmp(cmark_node_get_pretty_name(child), "table_row"))
        res = true;
    } else if (!strcmp(cmark_node_get_pretty_name(parent), "table_row")) {
      if (!strcmp(cmark_node_get_pretty_name(child), "table_cell"))
        res = true;
    }
  }

  return res;
}

static bool table_can_contain_inlines(cmark_block_parser *self, cmark_node *parent) {
  const char *node_type = cmark_node_get_pretty_name(parent);
  return (!strcmp(node_type, "table_cell") || !strcmp(node_type, "table_header_cell"));
}

typedef struct {
  unsigned int n_columns;
  cmark_llist *cells;
} table_row;

typedef struct {
  unsigned int n_columns;
} table_custom;

static void free_table_row(table_row *row) {

  if (!row)
    return;

  cmark_llist_free_full(row->cells, (CMarkListFreeFunc) cmark_strbuf_free);

  free(row);
}

static table_row *row_from_string(const char *string) {
  table_row *row;
  bufsize_t cell_matched = 0;
  bufsize_t cell_offset = 0;

  row = malloc(sizeof(table_row));
  row->n_columns = 0;
  row->cells = NULL;

  do {
    cell_matched = scan_table_cell(string, cell_offset);
    if (cell_matched) {
      cmark_strbuf *cell_buf = cmark_strbuf_new(0);
      cmark_strbuf_put(cell_buf,
                       (unsigned char *) string + cell_offset + 1,
                       cell_matched - 1);
      row->n_columns += 1;
      row->cells = cmark_llist_append(row->cells, cell_buf);
    }
    cell_offset += cell_matched;
  } while (cell_matched);

  cell_matched = scan_table_row_end(string, cell_offset);
  cell_offset += cell_matched;

  if (!cell_matched || cell_offset != (cmark_bufsize_t) strlen(string)) {
    free_table_row(row);
    return NULL;
  }

  return row;
}

static cmark_node *try_opening_table_header(cmark_block_parser *self,
                                            cmark_parser * parser,
                                            cmark_node   * parent_container,
                                            const char   * input) {
  bufsize_t matched = scan_table_start(input, cmark_parser_get_first_nonspace(parser));
  cmark_node *table_header;
  table_row *header_row = NULL;
  table_row *marker_row = NULL;
  table_custom *custom = NULL;

  if (!matched)
    goto done;

  header_row = row_from_string(cmark_node_get_string_content(parent_container));

  if (!header_row) {
    goto done;
  }

  marker_row = row_from_string(input);

  assert(marker_row);

  if (header_row->n_columns != marker_row->n_columns) {
    goto done;
  }

  cmark_node_set_type(parent_container, CMARK_NODE_EXTENSION_BLOCK);
  cmark_node_set_pretty_name(parent_container, "table");
  cmark_node_set_block_parser(parent_container, self);

  custom = malloc(sizeof(table_custom));
  custom->n_columns = header_row->n_columns;

  cmark_node_set_user_data(parent_container, custom);
  cmark_node_set_user_data_free_func(parent_container, free);

  table_header = cmark_node_new(CMARK_NODE_EXTENSION_BLOCK);
  cmark_node_set_pretty_name(table_header, "table_header");
  cmark_node_set_block_parser(table_header, self);
  cmark_node_append_child(parent_container, table_header);

  {
    cmark_llist *tmp;

    for (tmp = header_row->cells; tmp; tmp = tmp->next) {
      cmark_strbuf *cell_buf = (cmark_strbuf *) tmp->data;
      cmark_node *header_cell = cmark_node_new(CMARK_NODE_EXTENSION_BLOCK);
      cmark_node_set_string_content(header_cell, cmark_strbuf_get(cell_buf));
      cmark_node_set_pretty_name(header_cell, "table_header_cell");
      cmark_node_set_block_parser(header_cell, self);
      cmark_node_append_child(table_header, header_cell);
    }
  }

  cmark_parser_advance_offset(parser, input,
                   strlen(input) - 1 - cmark_parser_get_offset(parser),
                   false);
done:
  free_table_row(header_row);
  free_table_row(marker_row);
  return parent_container;
}

static cmark_node *try_opening_table_row(cmark_block_parser *self,
                                         cmark_parser * parser,
                                         cmark_node   * parent_container,
                                         const char   * input) {
  table_row *row;

  if (cmark_parser_is_blank(parser))
    return NULL;

  cmark_node *table_row = cmark_parser_make_block(CMARK_NODE_EXTENSION_BLOCK,
    cmark_parser_get_line_number(parser), cmark_parser_get_offset(parser));

  cmark_node_set_pretty_name(table_row, "table_row");
  cmark_node_set_block_parser(table_row, self);

  cmark_parser_add_child(parser, parent_container, table_row);

  /* We don't advance the offset here */

  row = row_from_string(input);

  {
    cmark_llist *tmp;

    for (tmp = row->cells; tmp; tmp = tmp->next) {
      cmark_strbuf *cell_buf = (cmark_strbuf *) tmp->data;
      cmark_node *cell = cmark_node_new(CMARK_NODE_EXTENSION_BLOCK);
      cmark_node_set_string_content(cell, cmark_strbuf_get(cell_buf));
      cmark_node_set_pretty_name(cell, "table_cell");
      cmark_node_set_block_parser(cell, self);
      cmark_node_append_child(table_row, cell);
    }
  }

  free_table_row(row);

  cmark_parser_advance_offset(parser, input,
                   strlen(input) - 1 - cmark_parser_get_offset(parser),
                   false);

  return table_row;
}

static cmark_node *try_opening_table_block(cmark_block_parser * block_parser,
                                           bool              indented,
                                           cmark_parser    * parser,
                                           cmark_node      * parent_container,
                                           const char      * input) {
  cmark_node_type parent_type = cmark_node_get_type(parent_container);

  if (!indented && parent_type == CMARK_NODE_PARAGRAPH) {
    return try_opening_table_header(block_parser, parser, parent_container, input);
  } else if (!indented && parent_type == CMARK_NODE_EXTENSION_BLOCK) {
    if (!strcmp(cmark_node_get_pretty_name(parent_container), "table")) {
      return try_opening_table_row(block_parser, parser, parent_container, input);
    }
  }

  return NULL;
}

static bool table_matches(cmark_block_parser *self,
                          cmark_parser * parser,
                          const char   * input,
                          cmark_node   * parent_container) {
  bool res = false;

  if (!strcmp(cmark_node_get_pretty_name(parent_container), "table")) {
    table_row *new_row = row_from_string(input);
    if (new_row) {
        if (new_row->n_columns ==
          ((table_custom *) cmark_node_get_user_data(parent_container))->n_columns)
          res = true;
    }
    free_table_row(new_row);
  }

  return res;
}

static bool table_render_block(cmark_block_parser *self, cmark_strbuf *buf, cmark_node * node,
    bool entering, const char *format) {
  bool res = false;

  if (!strcmp(format, "html")) {
    res = true;
    const char *node_type = cmark_node_get_pretty_name(node);

    if (!strcmp(node_type, "table")) {
      if (entering) {
        cmark_strbuf_puts(buf, "<table>\n");
      } else {
        cmark_strbuf_puts(buf, "</tbody>\n</table>\n");
      }
    } else if (!strcmp(node_type, "table_header")) {
      if (entering) {
        cmark_strbuf_puts(buf, "<thead>\n<tr>\n");
      } else {
        cmark_strbuf_puts(buf, "</tr>\n</thead>\n<tbody>\n");
      }
    } else if (!strcmp(node_type, "table_header_cell")) {
      if (entering) {
        cmark_strbuf_puts(buf, "<th>");
      } else {
        cmark_strbuf_puts(buf, "</th>\n");
      }
    } else if (!strcmp(node_type, "table_row")) {
      if (entering) {
        cmark_strbuf_puts(buf, "<tr>\n");
      } else {
        cmark_strbuf_puts(buf, "</tr>\n");
      }
    } else if (!strcmp(node_type, "table_cell")) {
      if (entering) {
        cmark_strbuf_puts(buf, "<td>");
      } else {
        cmark_strbuf_puts(buf, "</td>\n");
      }
    }
  }

  return res;
}

static cmark_block_parser *register_table_block_parser(void) {
  cmark_block_parser *ext = cmark_block_parser_new("piped-tables");

  ext->block_can_contain = table_can_contain;
  ext->last_block_matches = table_matches;
  ext->block_can_contain_inlines = table_can_contain_inlines;
  ext->try_opening_block = try_opening_table_block;
  ext->render_block = table_render_block;

  return ext;
}

bool init_libcmarkextensions(cmark_plugin *plugin) {
  cmark_plugin_register_block_parser(plugin, register_table_block_parser());
  return true;
}
