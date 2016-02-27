#include <dirent.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cmark.h"
#include "registry.h"
#include "plugin.h"
#include "hash.h"


static cmark_hash_table *block_parsers = NULL;
static cmark_llist *plugin_handles = NULL;

static cmark_plugin *scan_file(char* filename) {
  char* last_slash = strrchr(filename, '/');
  char* name_start = last_slash ? last_slash + 1 : filename;
  char* last_dot = strrchr(filename, '.');
  cmark_plugin *plugin = NULL;
  char *init_func_name = NULL;
  int i;
  void *libhandle;
  char *libname = NULL;

  if (!last_dot || strcmp(last_dot, ".so"))
      goto done;

  libname = malloc(sizeof(char) * (strlen(EXTENSION_DIR) + strlen(filename) + 2));
  snprintf(libname, strlen(EXTENSION_DIR) + strlen(filename) + 2, "%s/%s",
      EXTENSION_DIR, filename);
  libhandle = dlopen(libname, RTLD_NOW);
  free(libname);

  if (!libhandle) {
      printf("Error loading DSO: %s\n", dlerror());
      goto done;
  }

  name_start[last_dot - name_start] = '\0';

  for (i = 0; name_start[i]; i++) {
    if (name_start[i] == '-')
      name_start[i] = '_';
  }

  init_func_name = malloc(sizeof(char) * (strlen(name_start) + 6));

  snprintf(init_func_name, strlen(name_start) + 6, "init_%s", name_start);

  PluginInitFunc initfunc = (PluginInitFunc)
      (intptr_t) dlsym(libhandle, init_func_name);
  free(init_func_name);

  plugin = cmark_plugin_new();

  if (initfunc) {
    if (initfunc(plugin)) {
      plugin_handles = cmark_llist_append(plugin_handles, libhandle);
    } else {
      cmark_plugin_free(plugin);
      printf("Error Initializing plugin %s\n", name_start);
      plugin = NULL;
      dlclose(libhandle);
    }
  } else {
    printf("Error loading init function: %s\n", dlerror());
    dlclose(libhandle);
  }

done:
  return plugin;
}

static void scan_path(char *path) {
  DIR *dir = opendir(path);
  struct dirent* direntry;

  if (!dir)
    return;

  while ((direntry = readdir(dir))) {
    cmark_plugin *plugin = scan_file(direntry->d_name);
    if (plugin) {
      cmark_llist *block_parsers_list = cmark_plugin_steal_block_parsers(plugin);
      cmark_llist *tmp;

      for (tmp = block_parsers_list; tmp; tmp=tmp->next) {
        cmark_block_parser *ext = (cmark_block_parser *) tmp->data;
        cmark_hash_put(block_parsers, ext->name, ext);
      }

      cmark_llist_free(block_parsers_list);
      cmark_plugin_free(plugin);
    }
  }

 closedir(dir);
}

void cmark_discover_plugins(void) {
  cmark_release_plugins();
  block_parsers = cmark_hash_new_with_free_fn((CMarkHashFreeFunc) cmark_block_parser_free);
  scan_path(EXTENSION_DIR);
}

static void
release_plugin_handle(void *libhandle) {
  dlclose(libhandle);
}

void cmark_release_plugins(void) {
  if (block_parsers) {
    cmark_hash_destroy(block_parsers);
    block_parsers = NULL;
  }

  cmark_llist_free_full(plugin_handles, release_plugin_handle);
  plugin_handles = NULL;
}

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#       define CMARK_GNUC_UNUSED         __attribute__((__unused__))
#else
#       define CMARK_GNUC_UNUSED
#endif

static int
fill_block_parser_list(CMARK_GNUC_UNUSED void *key,
                    CMARK_GNUC_UNUSED void *key_size,
                    void *data,
                    CMARK_GNUC_UNUSED size_t data_size,
                    cmark_llist **list)
{
  (void) key;
  (void) key_size;
  (void) data_size;

  *list = cmark_llist_append(*list, data);
  return 0;
}

cmark_llist *cmark_list_block_parsers(void) {
  cmark_llist *list = NULL;

  cmark_hash_foreach(block_parsers,
                     (CMarkHashForeachFunc) fill_block_parser_list,
                     &list);

  return list;
}

cmark_block_parser *cmark_find_block_parser(const char *name) {
  return cmark_hash_get(block_parsers, name);
}
