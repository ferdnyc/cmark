#include <stdlib.h>
#include "ext_scanners.h"

bufsize_t _core_ext_scan_at(bufsize_t (*scanner)(const unsigned char *), const char *s, bufsize_t offset)
{
	bufsize_t res;
	bufsize_t len = strlen(s);
	unsigned char *ptr = (unsigned char *)s;

        if (ptr == NULL || offset > len) {
          return 0;
        } else {
	  res = scanner(ptr + offset);
        }

	return res;
}

/*!re2c
  re2c:define:YYCTYPE  = "unsigned char";
  re2c:define:YYCURSOR = p;
  re2c:define:YYMARKER = marker;
  re2c:define:YYCTXMARKER = marker;
  re2c:yyfill:enable = 0;

  spacechar = [ \t\v\f];
  newline = [\r]?[\n];

  escaped_char = [\\][|!"#$%&'()*+,./:;<=>?@[\\\]^_`{}~-];

  table_marker = [|](spacechar*[-]+spacechar*);
  table_cell = [|](escaped_char|[^|\r\n])+;
*/

bufsize_t _scan_table_cell(const unsigned char *p)
{
  const unsigned char *marker = NULL;
  const unsigned char *start = p;
/*!re2c
  table_cell { return (bufsize_t)(p - start); }
  .? { return 0; }
*/
}

bufsize_t _scan_table_row_end(const unsigned char *p)
{
  const unsigned char *marker = NULL;
  const unsigned char *start = p;
/*!re2c
  [|]newline { return (bufsize_t)(p - start); }
  .? { return 0; }
*/
}

bufsize_t _scan_table_start(const unsigned char *p)
{
  const unsigned char *marker = NULL;
  const unsigned char *start = p;
/*!re2c
  (table_marker)+ [|]newline { return (bufsize_t)(p - start); }
  .? { return 0; }
*/
}
