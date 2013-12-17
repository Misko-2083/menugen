#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <garcon/garcon.h>
#include <gtk/gtk.h>

#include "parser.h"

int req = 0;
char *reqs[] = {"", "  ", "    ", "      ", "        "};

static GtkIconTheme *theme;

struct {
    char name[1024];
    char iconpath[1024];
    char temp_buf[1024];
} Context = {{0}, {0}, {0}};

char *icon_path(char *to, const char *icon) {
    if (!icon) {
        to[0] = 0;
        return to;
    }
    GtkIconInfo* info = gtk_icon_theme_lookup_icon(theme, icon, 16, 0);
    if (info != NULL) {
        const char *path = gtk_icon_info_get_filename(info);
        strcpy(to, path);
        g_object_unref(info);
    } else {
        strcpy(to, icon);
    }
    return to;
}

void process_menu_item(struct menu_item *item, void *arg) {

    switch (item->type) {
    case MENU:
        strcpy(Context.name, item->name);
        icon_path(Context.iconpath, item->icon);
        printf("\n{\"%s\", {", item->name);
        req++;
        break;

    case MENUEND:
        printf("}, \"%s\"},", Context.iconpath);
        req--;
        break;

    case ENTRY:
        printf("\n%s{\"%s\", \"%s\", \"%s\"},", reqs[req], item->name, item->command, icon_path(Context.temp_buf, item->icon));
        break;

    default:
        break;
    }
}

int main(int argc, char **argv) {
    struct menugen_parser *parser;
    char *desktop = NULL;
    char *menufile = NULL;

    if (argc > 1) {
        desktop = argv[1];
    }

    if (argc >= 2) {
        menufile = argv[2];
    }

    gtk_init(0, NULL);
    theme = gtk_icon_theme_get_default();

    parser = menugen_parser_init(desktop, menufile);

    printf("local menu = {");
    menugen_parser_apply(parser, process_menu_item, NULL);
    printf("}\n\nreturn menu");

    menugen_parser_destroy(parser);

    return EXIT_SUCCESS;
}
