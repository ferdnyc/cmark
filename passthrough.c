#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cmark.h"
#include "node.h"

static const int S_leaf_mask =
    (1 << CMARK_NODE_HTML_BLOCK) | (1 << CMARK_NODE_THEMATIC_BREAK) |
    (1 << CMARK_NODE_CODE_BLOCK) | (1 << CMARK_NODE_TEXT) |
    (1 << CMARK_NODE_SOFTBREAK) | (1 << CMARK_NODE_LINEBREAK) |
    (1 << CMARK_NODE_CODE) | (1 << CMARK_NODE_HTML_INLINE);

static bool S_is_leaf(cmark_node *node) {
  return ((1 << node->type) & S_leaf_mask) != 0;
}

void
pass_ze_thing_through(cmark_node *root, char *p)
{
  cmark_iter *iter;
  cmark_event_type ev_type;
  bufsize_t i;

  iter = cmark_iter_new(root);

  while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
    cmark_node *cur = cmark_iter_get_node(iter);

    if (ev_type == CMARK_EVENT_ENTER) {
      for (i = cur->begin_offsets.start; i < cur->begin_offsets.stop; i++) {
        printf("%c", p[i]);
      }
      for (i = cur->extents.start; i < cur->extents.stop; i++) {
        printf("%c", p[i]);
      }
    }

    if (ev_type == CMARK_EVENT_EXIT || S_is_leaf(cur)) {
      for (i = cur->end_offsets.start; i < cur->end_offsets.stop; i++) {
        printf("%c", p[i]);
      }
    }
    fflush(stdout);
  }
}

int main(int argc, char **argv)
{
  struct stat sb;
  cmark_node *document;
  char *p;
  int fd;

  if (argc < 2) {
          fprintf (stderr, "usage: %s <file>\n", argv[0]);
          return 1;
  }

  fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
          perror ("open");
          return 1;
  }

  if (fstat (fd, &sb) == -1) {
          perror ("fstat");
          return 1;
  }

  if (!S_ISREG (sb.st_mode)) {
          fprintf (stderr, "%s is not a file\n", argv[1]);
          return 1;
  }

  p = mmap (0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
          perror ("mmap");
          return 1;
  }

  if (close (fd) == -1) {
          perror ("close");
          return 1;
  }

  document = cmark_parse_document(p, sb.st_size, CMARK_OPT_DEFAULT);

  pass_ze_thing_through(document, p);

  if (munmap (p, sb.st_size) == -1) {
          perror ("munmap");
          return 1;
  }

  return 0;
}
