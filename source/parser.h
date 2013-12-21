#ifndef PARSER_H_
#define PARSER_H_

enum MENU_ITEM_TYPE {
    MENU,
    MENUEND,
    ENTRY,
    SEPARATOR
};

struct menu_item {
    int type;
    int children;
    const char *name;
    const char *icon;
    const char *command;
    void *prevarg;
};

typedef void* (*menugen_parser_cb)(struct menu_item *item, void *arg);

struct menugen_parser *menugen_parser_init(char *desktop, char *file);
void menugen_parser_destroy(struct menugen_parser *self);
void menugen_parser_apply(struct menugen_parser *self, menugen_parser_cb cb, void *arg);

#endif /* PARSER_H_ */
