#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <garcon/garcon.h>
#include <gtk/gtk.h>

int req = 0;
char *reqs[] = {"", "  ", "    ", "      ", "        "};

static const char* pattern = " [^ ]*%[fFuUdDnNickvm]";
static GRegex *regex;
static GRegex *reg_escapes;
static GRegex *reg_quotes;

static GtkIconTheme *theme;

struct Menu {
    char name[1024];
    char iconpath[1024];
};

struct Menu current_menu = {{0}, {0}};

const char *icon_path(const char *icon, char *to) {
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

int compile_regexp() {
    GError *error = NULL;
    regex = g_regex_new(pattern, 0, 0, &error);
    if (!regex) {
        printf("Error compiling regex -> %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    reg_escapes = g_regex_new("\\\\", 0, 0, &error);
    if (!reg_escapes) {
        printf("Error compiling regex -> %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    reg_quotes = g_regex_new("\"", 0, 0, &error);
    if (!reg_quotes) {
        printf("Error compiling regex -> %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    return 0;
}

const char *escape_command(const char *command, char *to) {
    GError *error = NULL;

    char *res1 = g_regex_replace(regex, command, strlen(command), 0, "", 0, &error);
    char *res2 = g_regex_replace(reg_escapes, res1, strlen(res1), 0, "\\\\\\\\", 0, &error);
    char *res3 = g_regex_replace(reg_quotes, res2, strlen(res2), 0, "\\\\\"", 0, &error);

    strcpy(to, res3);
    free(res1);
    free(res2);
    free(res3);
    return to;
}

void process_menu(const char* name, const char* icon) {
    strcpy(current_menu.name, name);
    icon_path(icon, current_menu.iconpath);
    printf("\n{\"%s\", {", name);
    req++;
}

void process_menuend() {
    printf("}, \"%s\"},", current_menu.iconpath);
    req--;
}

void process_entry(const char* name, const char* icon, const char* command) {
    char buf1[1024] = {0};
    char buf2[1024] = {0};
    printf("\n%s{\"%s\", \"%s\", \"%s\"},", reqs[req], name, escape_command(command, buf1), icon_path(icon, buf2));
}

void process_separator() {
    //
}

#define IS(obj, type) g_type_check_instance_is_a((GTypeInstance *)obj, type)
#define ISMENU(obj) IS(obj, garcon_menu_get_type())
#define ISITEM(obj) IS(obj, garcon_menu_item_get_type())
#define ISSEP(obj) IS(obj, garcon_menu_separator_get_type())

void walk_menu(GarconMenuElement *element) {
    if (ISMENU(element)) {
        GarconMenu* menu = (GarconMenu *)element;
        GarconMenuDirectory* directory = garcon_menu_get_directory(menu);

        int show = directory != NULL && garcon_menu_directory_get_visible(directory);

        if (show) {
            process_menu(garcon_menu_directory_get_name(directory),
                    garcon_menu_directory_get_icon_name(directory));
        }

        GList *elements = garcon_menu_get_elements(menu);
        for (GList *el = elements; el != NULL; el = el->next) {
            GarconMenuElement *me = el->data;
            walk_menu(me);
        }
        g_list_free(elements);

        if (show) {
            process_menuend();
        }

    } else if (ISITEM(element)) {
        GarconMenuItem* item = (GarconMenuItem*)element;
        if (garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(item))) {
            process_entry(garcon_menu_item_get_name(item),
                    garcon_menu_item_get_icon_name(item),
                    garcon_menu_item_get_command(item));
        }
    } else if (ISSEP(element)) {
        process_separator();
    } else {
        printf("Unknown type: %s\n",
                garcon_menu_element_get_name(element));
    }
}

int main(int argc, char **argv) {
    if (argc > 1) {
        garcon_set_environment(argv[1]);
    }

    const char *menufile = argv[2];
    if (argc < 2) {
        menufile = NULL;
    } else {
        menufile = argv[2];
    }

    gtk_init(0, NULL);
    theme = gtk_icon_theme_get_default();

    GarconMenu *menu = menufile != NULL ? garcon_menu_new_for_path(menufile) : garcon_menu_new_applications();
    GError *error = NULL;

    if (!garcon_menu_load(menu, NULL, &error)) {
        printf("Error loading menu: %s\n", error->message);
        return EXIT_FAILURE;
    }

    compile_regexp();

    printf("local menu = {");
    walk_menu((GarconMenuElement*)menu);
    printf("}\n\nreturn menu");

    return EXIT_SUCCESS;
}
