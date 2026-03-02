/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#define GFOREACH(list, item) \
    for(GList *__glist = list; \
        __glist && (item = __glist->data, TRUE); \
        __glist = __glist->next)

#define GFOREACH_SUB(list, item) \
    for(GList *__glist_sub = list; \
        __glist_sub && (item = __glist_sub->data, TRUE); \
        __glist_sub = __glist_sub->next)

void write_to_file (const char *filename, const char *value);
GList *get_applications (void);
GList *get_cgroup_apps (const char *path);
GList *get_cgroup_slices (const char *path);
GList *get_list_from_variant (GVariant *value);
