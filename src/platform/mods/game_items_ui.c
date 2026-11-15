#include "p4pc/game_items_ui.h"
#include <stdlib.h>
#include <string.h>

typedef struct ItemImage { uint32_t id, token; } ItemImage;
struct P4GameItemsUi {
    const P4GameItems *items;
    size_t item_count, category_count, pinned;
    ItemImage images[];
};
void p4_game_tables_items_ui_destroy(P4GameItemsUi *ui)
{
    if (!ui) return;
    for (size_t i = 0; i < ui->pinned; ++i)
        if (ui->images[i].token) p4_resources_release(ui->images[i].token);
    free(ui);
}
static int resolve(P4GameItemsUi *ui, const P4Resources *resources, uint32_t id, const P4ResourceRef *ref)
{
    if (!resources || !p4_resources_revision(resources)) return P4_TABLE_NOT_READY;
    const P4ResourceImage *image = p4_resources_find(resources, ref->key);
    if (!image) return P4_TABLE_NOT_FOUND;
    if (strcmp(image->owner, ref->owner) || image->revision != ref->revision) return P4_TABLE_CONFLICT;
    if (!image->token || p4_resources_pin(image->token)) return P4_TABLE_NOT_READY;
    ui->images[ui->pinned++] = (ItemImage){id, image->token};
    return P4_TABLE_OK;
}
int p4_game_tables_items_ui_prepare(const P4GameItems *items, const P4Resources *resources, P4GameItemsUi **out)
{
    if (!items || !out || *out || !p4_game_tables_items_semantic_revision(items)) return P4_TABLE_INVALID;
    size_t count = p4_game_tables_items_count(items), categories = p4_game_tables_items_category_count(items);
    if (!count && !categories) return P4_TABLE_OK;
    if (count > SIZE_MAX - categories || count + categories > (SIZE_MAX - sizeof(P4GameItemsUi)) / sizeof(ItemImage))
        return P4_TABLE_NOMEM;
    P4GameItemsUi *ui = malloc(sizeof *ui + (count + categories) * sizeof *ui->images);
    if (!ui) return P4_TABLE_NOMEM;
    ui->items = items; ui->item_count = count; ui->category_count = categories; ui->pinned = 0;
    int result = P4_TABLE_OK;
    for (size_t i = 0; !result && i < count; ++i) {
        const P4GameItemView *row = p4_game_tables_items_at(items, i);
        if (row->icon_anchor_plus_one) ui->images[ui->pinned++] = (ItemImage){row->id, 0};
        else result = resolve(ui, resources, row->id, &row->icon);
    }
    for (size_t i = 0; !result && i < categories; ++i) {
        const P4GameItemCategoryView *row = p4_game_tables_items_category_at(items, i);
        result = resolve(ui, resources, row->id, &row->icon);
    }
    if (result) { p4_game_tables_items_ui_destroy(ui); return result; }
    *out = ui;
    return P4_TABLE_OK;
}
const P4GameItems *p4_game_tables_items_ui_model(const P4GameItemsUi *ui) { return ui ? ui->items : NULL; }
static uint32_t find(const ItemImage *rows, size_t count, uint32_t id)
{
    size_t low = 0, high = count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (rows[middle].id < id) low = middle + 1; else high = middle;
    }
    return low < count && rows[low].id == id ? rows[low].token : 0;
}
uint32_t p4_game_tables_items_ui_item(const P4GameItemsUi *ui, uint32_t id)
{ return ui ? find(ui->images, ui->item_count, id) : 0; }
uint32_t p4_game_tables_items_ui_category(const P4GameItemsUi *ui, uint32_t id)
{ return ui ? find(ui->images + ui->item_count, ui->category_count, id) : 0; }
