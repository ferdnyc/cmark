#include "cmark.h"
#include "chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

bufsize_t _core_ext_scan_at(bufsize_t (*scanner)(const unsigned char *), const char *c,
                   bufsize_t offset);
bufsize_t _scan_table_start(const unsigned char *p);
bufsize_t _scan_table_cell(const unsigned char *p);
bufsize_t _scan_table_row_end(const unsigned char *p);

#define scan_table_start(c, n) _core_ext_scan_at(&_scan_table_start, c, n)
#define scan_table_cell(c, n) _core_ext_scan_at(&_scan_table_cell, c, n)
#define scan_table_row_end(c, n) _core_ext_scan_at(&_scan_table_row_end, c, n)

#ifdef __cplusplus
}
#endif
