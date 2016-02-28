#ifndef CMARK_H
#define CMARK_H

#include <stdio.h>
#include <cmark_export.h>
#include <cmark_version.h>
#include <cmark_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/** # NAME
 *
 * **cmark** - CommonMark parsing, manipulating, and rendering
 */

/** # DESCRIPTION
 *
 * ## Simple Interface
 */

/** Convert 'text' (assumed to be a UTF-8 encoded string with length
 * 'len') from CommonMark Markdown to HTML, returning a null-terminated,
 * UTF-8-encoded string.
 */
CMARK_EXPORT
char *cmark_markdown_to_html(const char *text, size_t len, int options);

/** ## Node Structure
 */

typedef enum {
  /* Error status */
  CMARK_NODE_NONE,

  /* Block */
  CMARK_NODE_DOCUMENT,
  CMARK_NODE_BLOCK_QUOTE,
  CMARK_NODE_LIST,
  CMARK_NODE_ITEM,
  CMARK_NODE_CODE_BLOCK,
  CMARK_NODE_HTML_BLOCK,
  CMARK_NODE_CUSTOM_BLOCK,
  CMARK_NODE_PARAGRAPH,
  CMARK_NODE_HEADING,
  CMARK_NODE_THEMATIC_BREAK,

  CMARK_NODE_FIRST_BLOCK = CMARK_NODE_DOCUMENT,
  CMARK_NODE_LAST_BLOCK = CMARK_NODE_THEMATIC_BREAK,

  /* Inline */
  CMARK_NODE_TEXT,
  CMARK_NODE_SOFTBREAK,
  CMARK_NODE_LINEBREAK,
  CMARK_NODE_CODE,
  CMARK_NODE_HTML_INLINE,
  CMARK_NODE_CUSTOM_INLINE,
  CMARK_NODE_EMPH,
  CMARK_NODE_STRONG,
  CMARK_NODE_LINK,
  CMARK_NODE_IMAGE,

  CMARK_NODE_FIRST_INLINE = CMARK_NODE_TEXT,
  CMARK_NODE_LAST_INLINE = CMARK_NODE_IMAGE,
} cmark_node_type;

/* For backwards compatibility: */
#define CMARK_NODE_HEADER CMARK_NODE_HEADING
#define CMARK_NODE_HRULE CMARK_NODE_THEMATIC_BREAK
#define CMARK_NODE_HTML CMARK_NODE_HTML_BLOCK
#define CMARK_NODE_INLINE_HTML CMARK_NODE_HTML_INLINE

typedef enum {
  CMARK_NO_LIST,
  CMARK_BULLET_LIST,
  CMARK_ORDERED_LIST
} cmark_list_type;

typedef enum {
  CMARK_NO_DELIM,
  CMARK_PERIOD_DELIM,
  CMARK_PAREN_DELIM
} cmark_delim_type;

typedef struct cmark_node cmark_node;
typedef struct cmark_parser cmark_parser;
typedef struct cmark_iter cmark_iter;
typedef struct cmark_strbuf cmark_strbuf;

typedef int cmark_bufsize_t;

typedef void (*CMarkNodeUserDataFreeFunc) (void *user_data);

/**
 * ## Creating and Destroying Nodes
 */

/** Creates a new node of type 'type'.  Note that the node may have
 * other required properties, which it is the caller's responsibility
 * to assign.
 */
CMARK_EXPORT cmark_node *cmark_node_new(cmark_node_type type);

/** Frees the memory allocated for a node.
 */
CMARK_EXPORT void cmark_node_free(cmark_node *node);

/**
 * ## Tree Traversal
 */

/** Returns the next node in the sequence after 'node', or NULL if
 * there is none.
 */
CMARK_EXPORT cmark_node *cmark_node_next(cmark_node *node);

/** Returns the previous node in the sequence after 'node', or NULL if
 * there is none.
 */
CMARK_EXPORT cmark_node *cmark_node_previous(cmark_node *node);

/** Returns the parent of 'node', or NULL if there is none.
 */
CMARK_EXPORT cmark_node *cmark_node_parent(cmark_node *node);

/** Returns the first child of 'node', or NULL if 'node' has no children.
 */
CMARK_EXPORT cmark_node *cmark_node_first_child(cmark_node *node);

/** Returns the last child of 'node', or NULL if 'node' has no children.
 */
CMARK_EXPORT cmark_node *cmark_node_last_child(cmark_node *node);

/**
 * ## Iterator
 *
 * An iterator will walk through a tree of nodes, starting from a root
 * node, returning one node at a time, together with information about
 * whether the node is being entered or exited.  The iterator will
 * first descend to a child node, if there is one.  When there is no
 * child, the iterator will go to the next sibling.  When there is no
 * next sibling, the iterator will return to the parent (but with
 * a 'cmark_event_type' of `CMARK_EVENT_EXIT`).  The iterator will
 * return `CMARK_EVENT_DONE` when it reaches the root node again.
 * One natural application is an HTML renderer, where an `ENTER` event
 * outputs an open tag and an `EXIT` event outputs a close tag.
 * An iterator might also be used to transform an AST in some systematic
 * way, for example, turning all level-3 headings into regular paragraphs.
 *
 *     void
 *     usage_example(cmark_node *root) {
 *         cmark_event_type ev_type;
 *         cmark_iter *iter = cmark_iter_new(root);
 *
 *         while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
 *             cmark_node *cur = cmark_iter_get_node(iter);
 *             // Do something with `cur` and `ev_type`
 *         }
 *
 *         cmark_iter_free(iter);
 *     }
 *
 * Iterators will never return `EXIT` events for leaf nodes, which are nodes
 * of type:
 *
 * * CMARK_NODE_HTML_BLOCK
 * * CMARK_NODE_THEMATIC_BREAK
 * * CMARK_NODE_CODE_BLOCK
 * * CMARK_NODE_TEXT
 * * CMARK_NODE_SOFTBREAK
 * * CMARK_NODE_LINEBREAK
 * * CMARK_NODE_CODE
 * * CMARK_NODE_HTML_INLINE
 *
 * Nodes must only be modified after an `EXIT` event, or an `ENTER` event for
 * leaf nodes.
 */

typedef enum {
  CMARK_EVENT_NONE,
  CMARK_EVENT_DONE,
  CMARK_EVENT_ENTER,
  CMARK_EVENT_EXIT
} cmark_event_type;

/** Creates a new iterator starting at 'root'.  The current node and event
 * type are undefined until `cmark_iter_next` is called for the first time.
 */
CMARK_EXPORT
cmark_iter *cmark_iter_new(cmark_node *root);

/** Frees the memory allocated for an iterator.
 */
CMARK_EXPORT
void cmark_iter_free(cmark_iter *iter);

/** Advances to the next node and returns the event type (`CMARK_EVENT_ENTER`,
 * `CMARK_EVENT_EXIT` or `CMARK_EVENT_DONE`).
 */
CMARK_EXPORT
cmark_event_type cmark_iter_next(cmark_iter *iter);

/** Returns the current node.
 */
CMARK_EXPORT
cmark_node *cmark_iter_get_node(cmark_iter *iter);

/** Returns the current event type.
 */
CMARK_EXPORT
cmark_event_type cmark_iter_get_event_type(cmark_iter *iter);

/** Returns the root node.
 */
CMARK_EXPORT
cmark_node *cmark_iter_get_root(cmark_iter *iter);

/** Resets the iterator so that the current node is 'current' and
 * the event type is 'event_type'.  The new current node must be a
 * descendant of the root node or the root node itself.
 */
CMARK_EXPORT
void cmark_iter_reset(cmark_iter *iter, cmark_node *current,
                      cmark_event_type event_type);

/**
 * ## Accessors
 */

/** Returns the user data of 'node'.
 */
CMARK_EXPORT void *cmark_node_get_user_data(cmark_node *node);

/** Sets arbitrary user data for 'node'.  Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_user_data(cmark_node *node, void *user_data);

/** Set free function for user data */
CMARK_EXPORT
bool cmark_node_set_user_data_free_func(cmark_node *node,
                                        CMarkNodeUserDataFreeFunc free_func);

/** Returns the type of 'node', or `CMARK_NODE_NONE` on error.
 */
CMARK_EXPORT cmark_node_type cmark_node_get_type(cmark_node *node);

/** Change the type of 'node'.
 *
 * Return 'true' if the type could be changed, 'false' otherwise.
 */
CMARK_EXPORT bool cmark_node_set_type(cmark_node *node, cmark_node_type type);

/** Like 'cmark_node_get_type', but returns a string representation
    of the type, or `"<unknown>"`.
 */
CMARK_EXPORT
const char *cmark_node_get_type_string(cmark_node *node);

/** Returns the string contents of 'node', or an empty
    string if none is set.
 */
CMARK_EXPORT const char *cmark_node_get_literal(cmark_node *node);

/** Sets the string contents of 'node'.  Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_literal(cmark_node *node, const char *content);

/** Returns the heading level of 'node', or 0 if 'node' is not a heading.
 */
CMARK_EXPORT int cmark_node_get_heading_level(cmark_node *node);

/* For backwards compatibility */
#define cmark_node_get_header_level cmark_node_get_heading_level
#define cmark_node_set_header_level cmark_node_set_heading_level

/** Sets the heading level of 'node', returning 1 on success and 0 on error.
 */
CMARK_EXPORT int cmark_node_set_heading_level(cmark_node *node, int level);

/** Returns the list type of 'node', or `CMARK_NO_LIST` if 'node'
 * is not a list.
 */
CMARK_EXPORT cmark_list_type cmark_node_get_list_type(cmark_node *node);

/** Sets the list type of 'node', returning 1 on success and 0 on error.
 */
CMARK_EXPORT int cmark_node_set_list_type(cmark_node *node,
                                          cmark_list_type type);

/** Returns the list delimiter type of 'node', or `CMARK_NO_DELIM` if 'node'
 * is not a list.
 */
CMARK_EXPORT cmark_delim_type cmark_node_get_list_delim(cmark_node *node);

/** Sets the list delimiter type of 'node', returning 1 on success and 0
 * on error.
 */
CMARK_EXPORT int cmark_node_set_list_delim(cmark_node *node,
                                           cmark_delim_type delim);

/** Returns starting number of 'node', if it is an ordered list, otherwise 0.
 */
CMARK_EXPORT int cmark_node_get_list_start(cmark_node *node);

/** Sets starting number of 'node', if it is an ordered list. Returns 1
 * on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_list_start(cmark_node *node, int start);

/** Returns 1 if 'node' is a tight list, 0 otherwise.
 */
CMARK_EXPORT int cmark_node_get_list_tight(cmark_node *node);

/** Sets the "tightness" of a list.  Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_list_tight(cmark_node *node, int tight);

/** Returns the info string from a fenced code block.
 */
CMARK_EXPORT const char *cmark_node_get_fence_info(cmark_node *node);

/** Sets the info string in a fenced code block, returning 1 on
 * success and 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_fence_info(cmark_node *node, const char *info);

/** Returns the URL of a link or image 'node', or an empty string
    if no URL is set.
 */
CMARK_EXPORT const char *cmark_node_get_url(cmark_node *node);

/** Sets the URL of a link or image 'node'. Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_url(cmark_node *node, const char *url);

/** Returns the title of a link or image 'node', or an empty
    string if no title is set.
 */
CMARK_EXPORT const char *cmark_node_get_title(cmark_node *node);

/** Sets the title of a link or image 'node'. Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_title(cmark_node *node, const char *title);

/** Returns the literal "on enter" text for a custom 'node', or
    an empty string if no on_enter is set.
 */
CMARK_EXPORT const char *cmark_node_get_on_enter(cmark_node *node);

/** Sets the literal text to render "on enter" for a custom 'node'.
    Any children of the node will be rendered after this text.
    Returns 1 on success 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_on_enter(cmark_node *node,
                                         const char *on_enter);

/** Returns the literal "on exit" text for a custom 'node', or
    an empty string if no on_exit is set.
 */
CMARK_EXPORT const char *cmark_node_get_on_exit(cmark_node *node);

/** Sets the literal text to render "on exit" for a custom 'node'.
    Any children of the node will be rendered before this text.
    Returns 1 on success 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_on_exit(cmark_node *node, const char *on_exit);

/** Returns the line on which 'node' begins.
 */
CMARK_EXPORT int cmark_node_get_start_line(cmark_node *node);

/** Returns the column at which 'node' begins.
 */
CMARK_EXPORT int cmark_node_get_start_column(cmark_node *node);

/** Returns the line on which 'node' ends.
 */
CMARK_EXPORT int cmark_node_get_end_line(cmark_node *node);

/** Returns the column at which 'node' ends.
 */
CMARK_EXPORT int cmark_node_get_end_column(cmark_node *node);

/**
 * ## Tree Manipulation
 */

/** Unlinks a 'node', removing it from the tree, but not freeing its
 * memory.  (Use 'cmark_node_free' for that.)
 */
CMARK_EXPORT void cmark_node_unlink(cmark_node *node);

/** Inserts 'sibling' before 'node'.  Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_insert_before(cmark_node *node,
                                          cmark_node *sibling);

/** Inserts 'sibling' after 'node'. Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_insert_after(cmark_node *node, cmark_node *sibling);

/** Replaces 'oldnode' with 'newnode' and unlinks 'oldnode' (but does
 * not free its memory).
 * Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_replace(cmark_node *oldnode, cmark_node *newnode);

/** Adds 'child' to the beginning of the children of 'node'.
 * Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_prepend_child(cmark_node *node, cmark_node *child);

/** Adds 'child' to the end of the children of 'node'.
 * Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_append_child(cmark_node *node, cmark_node *child);

/** Consolidates adjacent text nodes.
 */
CMARK_EXPORT void cmark_consolidate_text_nodes(cmark_node *root);

/**
 * ## Parsing
 *
 * Simple interface:
 *
 *     cmark_node *document = cmark_parse_document("Hello *world*", 13,
 *                                                 CMARK_OPT_DEFAULT);
 *
 * Streaming interface:
 *
 *     cmark_parser *parser = cmark_parser_new(CMARK_OPT_DEFAULT);
 *     FILE *fp = fopen("myfile.md", "r");
 *     while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
 *     	   cmark_parser_feed(parser, buffer, bytes);
 *     	   if (bytes < sizeof(buffer)) {
 *     	       break;
 *     	   }
 *     }
 *     document = cmark_parser_finish(parser);
 *     cmark_parser_free(parser);
 */

/** Creates a new parser object.
 */
CMARK_EXPORT
cmark_parser *cmark_parser_new(int options);

/** Frees memory allocated for a parser object.
 */
CMARK_EXPORT
void cmark_parser_free(cmark_parser *parser);

/** Feeds a string of length 'len' to 'parser'.
 */
CMARK_EXPORT
void cmark_parser_feed(cmark_parser *parser, const char *buffer, size_t len);

/** Finish parsing and return a pointer to a tree of nodes.
 */
CMARK_EXPORT
cmark_node *cmark_parser_finish(cmark_parser *parser);

/** Parse a CommonMark document in 'buffer' of length 'len'.
 * Returns a pointer to a tree of nodes.
 */
CMARK_EXPORT
cmark_node *cmark_parse_document(const char *buffer, size_t len, int options);

/** Parse a CommonMark document in file 'f', returning a pointer to
 * a tree of nodes.
 */
CMARK_EXPORT
cmark_node *cmark_parse_file(FILE *f, int options);

/** Return the index of the line currently being parsed, starting with 1.
 */
CMARK_EXPORT
int cmark_parser_get_line_number(cmark_parser *parser);

/** Return the offset in bytes in the line being processed.
 *
 * Example:
 *
 * ### foo
 *
 * Here, offset will first be 0, then 5 (the index of the 'f' character).
 */
CMARK_EXPORT
cmark_bufsize_t cmark_parser_get_offset(cmark_parser *parser);

/**
 * Return the offset in 'columns' in the line being processed.
 *
 * This value may differ from the value returned by
 * cmark_parser_get_offset() in that it accounts for tabs,
 * and as such should not be used as an index in the current line's
 * buffer.
 *
 * Example:
 *
 * cmark_parser_advance_offset() can be called to advance the
 * offset by a number of columns, instead of a number of bytes.
 *
 * In that case, if offset falls "in the middle" of a tab
 * character, 'column' and offset will differ.
 *
 * ```
 * foo                 \t bar
 * ^                   ^^
 * offset (0)          20
 * ```
 *
 * If cmark_parser_advance_offset is called here with 'columns'
 * set to 'true' and 'offset' set to 22, cmark_parser_get_offset()
 * will return 20, whereas cmark_parser_get_column() will return
 * 22.
 *
 * Additionally, as tabs expand to the next multiple of 4 column,
 * cmark_parser_has_partially_consumed_tab() will now return
 * 'true'.
 */
CMARK_EXPORT
cmark_bufsize_t cmark_parser_get_column(cmark_parser *parser);

/** Return the absolute index in bytes of the first nonspace
 * character coming after the offset as returned by
 * cmark_parser_get_offset() in the line currently being processed.
 *
 * Example:
 *
 * ```
 *   foo        bar            baz  \n
 * ^               ^           ^
 * 0            offset (16) first_nonspace (28)
 * ```
 */
CMARK_EXPORT
cmark_bufsize_t cmark_parser_get_first_nonspace(cmark_parser *parser);

/** Return the absolute index of the first nonspace column coming after 'offset'
 * in the line currently being processed, counting tabs as multiple
 * columns as appropriate.
 *
 * See the documentation for cmark_parser_get_first_nonspace() and
 * cmark_parser_get_column() for more information.
 */
CMARK_EXPORT
cmark_bufsize_t cmark_parser_get_first_nonspace_column(cmark_parser *parser);

/** Return the difference between the values returned by
 * cmark_parser_get_first_nonspace_column() and
 * cmark_parser_get_column().
 *
 * This is not a byte offset, as it can count one tab as multiple
 * characters.
 */
CMARK_EXPORT
int cmark_parser_get_indent(cmark_parser *parser);

/** Return 'true' if the line currently being processed has been entirely
 * consumed, 'false' otherwise.
 *
 * Example:
 *
 * ```
 *   foo        bar            baz  \n
 * ^
 * offset
 * ```
 *
 * This function will return 'false' here.
 *
 * ```
 *   foo        bar            baz  \n
 *                 ^
 *              offset
 * ```
 * This function will still return 'false'.
 *
 * ```
 *   foo        bar            baz  \n
 *                                ^
 *                             offset
 * ```
 *
 * At this point, this function will now return 'true'.
 */
CMARK_EXPORT
bool cmark_parser_is_blank(cmark_parser *parser);

/** Return 'true' if the value returned by cmark_parser_get_offset()
 * is 'inside' an expanded tab.
 *
 * See the documentation for cmark_parser_get_column() for more
 * information.
 */
CMARK_EXPORT
bool cmark_parser_has_partially_consumed_tab(cmark_parser *parser);

/** Return the length in bytes of the previously processed line, excluding potential
 * newline (\n) and carriage return (\r) trailing characters.
 */
CMARK_EXPORT
cmark_bufsize_t cmark_parser_get_last_line_length(cmark_parser *parser);

/** Add a child to 'parent' during the parsing process.
 *
 * If 'parent' isn't the kind of node that can accept this child,
 * this function will back up till it hits a node that can, closing
 * blocks as appropriate.
 */
CMARK_EXPORT
cmark_node*cmark_parser_add_child(cmark_parser *parser,
                                  cmark_node *parent,
                                  cmark_node_type block_type,
                                  int start_column);

/** Advance the 'offset' of the parser in the current line.
 *
 * See the documentation of cmark_parser_get_offset() and
 * cmark_parser_get_column() for more information.
 */
CMARK_EXPORT
void cmark_parser_advance_offset(cmark_parser *parser,
                                 const char *input,
                                 int count,
                                 bool columns);

/**
 * ## Rendering
 */

/** Render a 'node' tree as XML.
 */
CMARK_EXPORT
char *cmark_render_xml(cmark_node *root, int options);

/** Render a 'node' tree as an HTML fragment.  It is up to the user
 * to add an appropriate header and footer.
 */
CMARK_EXPORT
char *cmark_render_html(cmark_node *root, int options);

/** Render a 'node' tree as a groff man page, without the header.
 */
CMARK_EXPORT
char *cmark_render_man(cmark_node *root, int options, int width);

/** Render a 'node' tree as a commonmark document.
 */
CMARK_EXPORT
char *cmark_render_commonmark(cmark_node *root, int options, int width);

/** Render a 'node' tree as a LaTeX document.
 */
CMARK_EXPORT
char *cmark_render_latex(cmark_node *root, int options, int width);

/**
 * ## Character buffer interface
 */

/**Create a new buffer with size 'initial size'
 */
CMARK_EXPORT
cmark_strbuf *cmark_strbuf_new(cmark_bufsize_t initial_size);

/** Grow the buffer to hold at least `target_size` bytes.
 */
CMARK_EXPORT
void cmark_strbuf_grow(cmark_strbuf *buf, cmark_bufsize_t target_size);

/** Free 'buf'.
 */
CMARK_EXPORT
void cmark_strbuf_free(cmark_strbuf *buf);

/** Swap 'buf_a' and 'buf_b'.
 */
CMARK_EXPORT
void cmark_strbuf_swap(cmark_strbuf *buf_a, cmark_strbuf *buf_b);

/** Get the size in bytes of 'buf'.
 */
CMARK_EXPORT
cmark_bufsize_t cmark_strbuf_size(const cmark_strbuf *buf);

/**Compare 'a' and 'b' contents, return 0 if they are the same.
 *
 * Return a negative value is a < b, a positive value if a > b.
 */
CMARK_EXPORT
int cmark_strbuf_cmp(const cmark_strbuf *a, const cmark_strbuf *b);

/** Copy the contents of 'buf' to a previously allocated 'data' pointer
 */
CMARK_EXPORT
void cmark_strbuf_copy_cstr(char *data, cmark_bufsize_t datasize,
                            const cmark_strbuf *buf);

/** Get the contents of 'buf'.
 *
 * The returned string is guaranteed to be NULL-terminated.
 */
CMARK_EXPORT
const char * cmark_strbuf_get(cmark_strbuf *buf);

/** Set the contents of 'buf' to the given 'data'
 */
CMARK_EXPORT
void cmark_strbuf_set(cmark_strbuf *buf, const unsigned char *data,
                      cmark_bufsize_t len);

/** Set the contents of 'buf' to the given NULL-terminated 'string'
 */
CMARK_EXPORT
void cmark_strbuf_sets(cmark_strbuf *buf, const char *string);

/** Append the character 'c' to 'buf'
 */
CMARK_EXPORT
void cmark_strbuf_putc(cmark_strbuf *buf, int c);

/** Append the given 'data' to 'buf'
 */
CMARK_EXPORT
void cmark_strbuf_put(cmark_strbuf *buf, const unsigned char *data,
                      cmark_bufsize_t len);

/** Append the given NULL-terminated 'string' to 'buf'
 */
CMARK_EXPORT
void cmark_strbuf_puts(cmark_strbuf *buf, const char *string);

/** Reset 'buf' to the empty state
 */
CMARK_EXPORT
void cmark_strbuf_clear(cmark_strbuf *buf);

/** Return the index in 'buf' where 'c' is first encountered, starting from
 * 'pos'.
 *
 * Return -1 if 'c' isn't encountered.
 */
CMARK_EXPORT
cmark_bufsize_t cmark_strbuf_strchr(const cmark_strbuf *buf, int c, cmark_bufsize_t pos);

/** Return the index in 'buf' where 'c' is last encountered, starting from
 * 'pos'.
 *
 * Return -1 if 'c' isn't encountered.
 */
CMARK_EXPORT
cmark_bufsize_t cmark_strbuf_strrchr(const cmark_strbuf *buf, int c, cmark_bufsize_t pos);

/** Drop 'n' bytes from the beginning of 'buf'.
 */
CMARK_EXPORT
void cmark_strbuf_drop(cmark_strbuf *buf, cmark_bufsize_t n);

/** Truncate buf to len bytes if its size is greater than len.
 */
CMARK_EXPORT
void cmark_strbuf_truncate(cmark_strbuf *buf, cmark_bufsize_t len);

/** Trim whitespaces at the end of 'buf'
 */
CMARK_EXPORT
void cmark_strbuf_rtrim(cmark_strbuf *buf);

/** Trim whitespaces on both ends of 'buf'
 */
CMARK_EXPORT
void cmark_strbuf_trim(cmark_strbuf *buf);

/** Destructively modify 's', collapsing consecutive
 * space and newline characters into a single space.
 */
CMARK_EXPORT
void cmark_strbuf_normalize_whitespace(cmark_strbuf *s);

/** Destructively unescape 's': remove backslashes before punctuation chars.
 */
CMARK_EXPORT
void cmark_strbuf_unescape(cmark_strbuf *s);

/**
 * ## Options
 */

/** Default options.
 */
#define CMARK_OPT_DEFAULT 0

/**
 * ### Options affecting rendering
 */

/** Include a `data-sourcepos` attribute on all block elements.
 */
#define CMARK_OPT_SOURCEPOS (1 << 1)

/** Render `softbreak` elements as hard line breaks.
 */
#define CMARK_OPT_HARDBREAKS (1 << 2)

/** Suppress raw HTML and unsafe links (`javascript:`, `vbscript:`,
 * `file:`, and `data:`, except for `image/png`, `image/gif`,
 * `image/jpeg`, or `image/webp` mime types).  Raw HTML is replaced
 * by a placeholder HTML comment. Unsafe links are replaced by
 * empty strings.
 */
#define CMARK_OPT_SAFE (1 << 3)

/**
 * ### Options affecting parsing
 */

/** Normalize tree by consolidating adjacent text nodes.
 */
#define CMARK_OPT_NORMALIZE (1 << 8)

/** Validate UTF-8 in the input before parsing, replacing illegal
 * sequences with the replacement character U+FFFD.
 */
#define CMARK_OPT_VALIDATE_UTF8 (1 << 9)

/** Convert straight quotes to curly, --- to em dashes, -- to en dashes.
 */
#define CMARK_OPT_SMART (1 << 10)

/**
 * ## Version information
 */

/** The library version as integer for runtime checks. Also available as
 * macro CMARK_VERSION for compile time checks.
 *
 * * Bits 16-23 contain the major version.
 * * Bits 8-15 contain the minor version.
 * * Bits 0-7 contain the patchlevel.
 *
 * In hexadecimal format, the number 0x010203 represents version 1.2.3.
 */
CMARK_EXPORT
int cmark_version();

/** The library version string for runtime checks. Also available as
 * macro CMARK_VERSION_STRING for compile time checks.
 */
CMARK_EXPORT
const char *cmark_version_string();

/** # AUTHORS
 *
 * John MacFarlane, Vicent Marti,  Kārlis Gaņģis, Nick Wellnhofer.
 */

#ifndef CMARK_NO_SHORT_NAMES
#define NODE_DOCUMENT CMARK_NODE_DOCUMENT
#define NODE_BLOCK_QUOTE CMARK_NODE_BLOCK_QUOTE
#define NODE_LIST CMARK_NODE_LIST
#define NODE_ITEM CMARK_NODE_ITEM
#define NODE_CODE_BLOCK CMARK_NODE_CODE_BLOCK
#define NODE_HTML_BLOCK CMARK_NODE_HTML_BLOCK
#define NODE_CUSTOM_BLOCK CMARK_NODE_CUSTOM_BLOCK
#define NODE_PARAGRAPH CMARK_NODE_PARAGRAPH
#define NODE_HEADING CMARK_NODE_HEADING
#define NODE_HEADER CMARK_NODE_HEADER
#define NODE_THEMATIC_BREAK CMARK_NODE_THEMATIC_BREAK
#define NODE_HRULE CMARK_NODE_HRULE
#define NODE_TEXT CMARK_NODE_TEXT
#define NODE_SOFTBREAK CMARK_NODE_SOFTBREAK
#define NODE_LINEBREAK CMARK_NODE_LINEBREAK
#define NODE_CODE CMARK_NODE_CODE
#define NODE_HTML_INLINE CMARK_NODE_HTML_INLINE
#define NODE_CUSTOM_INLINE CMARK_NODE_CUSTOM_INLINE
#define NODE_EMPH CMARK_NODE_EMPH
#define NODE_STRONG CMARK_NODE_STRONG
#define NODE_LINK CMARK_NODE_LINK
#define NODE_IMAGE CMARK_NODE_IMAGE
#define BULLET_LIST CMARK_BULLET_LIST
#define ORDERED_LIST CMARK_ORDERED_LIST
#define PERIOD_DELIM CMARK_PERIOD_DELIM
#define PAREN_DELIM CMARK_PAREN_DELIM
#endif

#ifdef __cplusplus
}
#endif

#endif
