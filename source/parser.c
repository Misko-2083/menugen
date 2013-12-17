#include "parser.h"

#include <garcon/garcon.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct regex_it {
    const char *pattern;
    const char *replace;
};

static struct regex_it regex_items[] = {
        {" [^ ]*%[fFuUdDnNickvm]", ""},
        {"\\\\", "\\\\\\\\"},
        {"\"", "\\\\\""}
};
#define REG_COUNT (sizeof(regex_items) / sizeof(regex_items[0]))

struct menugen_parser {
    GarconMenu *menu;
    GRegex *regex[REG_COUNT];
};


struct menugen_parser *menugen_parser_init(char *desktop, char *menufile) {
    if (desktop)
        garcon_set_environment(desktop);

    struct menugen_parser *result;

    GarconMenu *menu = menufile != NULL ? garcon_menu_new_for_path(menufile) : garcon_menu_new_applications();
    GError *error = NULL;

    if (!garcon_menu_load(menu, NULL, &error)) {
        printf("Error loading menu: %s\n", error->message);
        free(error);
        return NULL;
    }

    result = malloc(sizeof(struct menugen_parser));
    result->menu = menu;

    // compile regex
    for (int i = 0; i < REG_COUNT; ++i) {
        GError *error = NULL;
        struct regex_it *rit = (regex_items + i);
        GRegex *regex = g_regex_new(rit->pattern, 0, 0, &error);

        if (!regex) {
            printf("Error compiling regex (BUG) -> %s\n", error->message);
            g_error_free(error);
            exit(EXIT_FAILURE);
        }

        result->regex[i] = regex;
    }

    return result;

}

void menugen_parser_destroy(struct menugen_parser *self) {
    for (int i = 0; i < REG_COUNT; ++i) {
        g_regex_unref(self->regex[i]);
    }
    free(self);
}

static char *escape_command(struct menugen_parser *self, const char *command) {
    GError *error = NULL;
    char *result;
    if (REG_COUNT > 0) {
        char *last = g_regex_replace(self->regex[0], command, strlen(command), 0, regex_items[0].replace, 0, &error);
        for (int i = 1; i < REG_COUNT; ++i) {
            struct regex_it *rit = (regex_items + i);
            char *res = g_regex_replace(self->regex[i], last, strlen(last), 0, rit->replace, 0, &error);
            free(last);
            last = res;
        }
        result = last;
    } else {
        result = malloc(strlen(command));
        strcpy(result, command);
    }
    return result;
}

#define IS(obj, type) g_type_check_instance_is_a((GTypeInstance *)obj, type)
#define ISMENU(obj) IS(obj, garcon_menu_get_type())
#define ISITEM(obj) IS(obj, garcon_menu_item_get_type())
#define ISSEP(obj) IS(obj, garcon_menu_separator_get_type())

static void walk_menu(GarconMenuElement *element, struct menugen_parser *self, menugen_parser_cb callback, void *arg) {
    if (ISMENU(element)) {
        GarconMenu* menu = (GarconMenu *)element;
        GarconMenuDirectory* directory = garcon_menu_get_directory(menu);

        int show = directory != NULL && garcon_menu_directory_get_visible(directory);

        if (show) {
            struct menu_item item = {
                .type = MENU,
                .name = garcon_menu_directory_get_name(directory),
                .icon = garcon_menu_directory_get_icon_name(directory)
            };
            callback(&item, arg);
        }

        GList *elements = garcon_menu_get_elements(menu);
        for (GList *el = elements; el != NULL; el = el->next) {
            GarconMenuElement *me = el->data;
            walk_menu(me, self, callback, arg);
        }
        g_list_free(elements);

        if (show) {
            struct menu_item item = {
                .type = MENUEND
            };
            callback(&item, arg);
        }

    } else if (ISITEM(element)) {
        GarconMenuItem* gmi = (GarconMenuItem*)element;
        if (garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(gmi))) {
            char *command = escape_command(self, garcon_menu_item_get_command(gmi));
            struct menu_item item = {
                .type = ENTRY,
                .name = garcon_menu_item_get_name(gmi),
                .icon = garcon_menu_item_get_icon_name(gmi),
                .command = command
            };
            callback(&item, arg);
            free(command);
        }
    } else if (ISSEP(element)) {
        struct menu_item item = {
            .type = SEPARATOR
        };
        callback(&item, arg);
    } else {
        printf("Unknown type: %s\n",
                garcon_menu_element_get_name(element));
    }
}


void menugen_parser_apply(struct menugen_parser *self, menugen_parser_cb cb, void *arg) {
    walk_menu((GarconMenuElement*)self->menu, self, cb, arg);
}
