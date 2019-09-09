#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <garcon/garcon.h>
#include "parser.h"

#define STR_IS_EMPTY(str) ((str) == NULL || *(str) == '\0')

static void *process_menu_item(struct menu_item *item) {
    void *result = NULL;
    
    switch (item->type) {
    case MENU:

        printf("\n%s,^checkout(%s),%s\n\n", item->name, item->name, item->icon);
        printf("\n^tag(%s)\n", item->name);
        break;

    case MENUEND:
        /* sub_dir? not sure how this works!
        if (g_strdup (item->icon) == NULL) {
          printf("\n%s,%p,%s\n", item->name, item->prevarg, "applications-other");
        } else {
          printf("\n%s,%p,%s\n", item->name, item->prevarg, item->icon);
        }
        */
        break;

    case SEPARATOR:
        printf("^sep()\n");
        break;

    case ENTRY:
        if (g_strdup (item->icon) == NULL) {
          printf("%s,%s,%s\n", item->name, item->command, "applications-other");
        } else {
          printf("%s,%s,%s\n", item->name, item->command, item->icon);
        }
        break;

    default:
        break;
    }


    return result;
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

    parser = menugen_parser_init(desktop, menufile);


    menugen_parser_apply(parser, process_menu_item);


    menugen_parser_destroy(parser);


    return EXIT_SUCCESS;
}
