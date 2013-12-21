#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <garcon/garcon.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "parser.h"

#define STR_IS_EMPTY(str) ((str) == NULL || *(str) == '\0')

static char *icon_path(char *to, const char *icon) {
    if (STR_IS_EMPTY(icon)) {
        to[0] = 0;
        return to;
    }
    GtkIconInfo* info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
            icon, GTK_ICON_SIZE_SMALL_TOOLBAR, 0);
    if (info != NULL) {
        const char *path = gtk_icon_info_get_filename(info);
        strcpy(to, path);
        gtk_icon_info_free(info);
    } else {
        strcpy(to, icon);
    }
    return to;
}

static GdkPixbuf *get_pixbuf_from_icon(const char *icon_name) {
    int w, h;
    GdkPixbuf *pixbuf = NULL;
    GError *error = NULL;

    gtk_icon_size_lookup(GTK_ICON_SIZE_SMALL_TOOLBAR, &w, &h);
    pixbuf = gdk_pixbuf_new_from_file_at_size(icon_name, w, h, &error);
    if (!error) {
        // try to rescale
        int nw = gdk_pixbuf_get_width(pixbuf), nh = gdk_pixbuf_get_height(pixbuf);
        if (nw != w || nh != h) {
            GdkPixbuf *newpixbuf = gdk_pixbuf_scale_simple(pixbuf, w, h, GDK_INTERP_BILINEAR);
            g_object_unref(pixbuf);
            pixbuf = newpixbuf;
        }
    } else {
        g_error_free(error);
        error = NULL;
        pixbuf = NULL;
    }

    return pixbuf;
}

static GtkWidget *get_image(const char *icon_name) {
    if (STR_IS_EMPTY(icon_name))
        icon_name = "applications-other";

    GtkWidget *image = NULL;

    if (!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), icon_name)) {
        GdkPixbuf *pixbuf = get_pixbuf_from_icon(icon_name);
        if (!pixbuf && *icon_name != '/') {
            char buf[1024] = { 0 };
            snprintf(buf, 1024, "/usr/share/pixmaps/%s", icon_name);
            pixbuf = get_pixbuf_from_icon(buf);
        }
        if (pixbuf) {
            image = gtk_image_new_from_pixbuf(pixbuf);
        }
    }

    if (!image) {
        image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR);
    }

    return image;
}

static void launcher_activated(GtkWidget *widget, gchar *command) {
    GError *error = NULL;
    GAppInfo *app = g_app_info_create_from_commandline(command, NULL, 0, &error);

    if (!g_app_info_launch(app, NULL, NULL, &error)) {
        exit(1);
    }
}

static void close_app(GtkWidget *widget, void *command) {
    gtk_main_quit();
}


static void *process_menu_item(struct menu_item *item, void *arg) {
    GtkMenu *menu = GTK_MENU(arg);

    GtkWidget *mitem;
    GtkWidget *submenu;
    GtkWidget *image;
    GtkWidget *separator;
    void *result = NULL;

    switch (item->type) {
    case MENU:
        submenu = gtk_menu_new();
        result = submenu;
        break;

    case MENUEND:
        submenu = item->prevarg;
        if (item->children) {
            mitem = gtk_image_menu_item_new_with_label(item->name);
            image =  get_image(item->icon);
            gtk_widget_show(image);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mitem), image);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), submenu);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
            gtk_widget_show(submenu);
            gtk_widget_show(mitem);
        } else {
            gtk_widget_destroy(submenu);
        }
        break;

    case SEPARATOR:
        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);
        gtk_widget_show(separator);
        break;

    case ENTRY:
        mitem = gtk_image_menu_item_new_with_label(item->name);
        image = get_image(item->icon);
        gtk_widget_show(image);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mitem), image);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
        g_signal_connect (G_OBJECT(mitem), "activate", G_CALLBACK (launcher_activated), g_strdup (item->command));
        gtk_widget_show(mitem);
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

    gtk_init(0, NULL);

    parser = menugen_parser_init(desktop, menufile);

    GtkWidget *mainMenu = gtk_menu_new();
    menugen_parser_apply(parser, process_menu_item, mainMenu);

    gtk_widget_show(mainMenu);
    gtk_menu_popup (GTK_MENU (mainMenu),
                        NULL, NULL, NULL, NULL,
                        0, 0);

    menugen_parser_destroy(parser);

    g_signal_connect (G_OBJECT(mainMenu), "hide", G_CALLBACK (close_app), NULL);
    gtk_main();


    return EXIT_SUCCESS;
}
