#pragma once
#include <CoreText/CoreText.h>
#include "misc/helpers.h"
#include "color.h"

// Decorative primitives drawn onto an item's canvas. Shapes are positioned
// relative to the item's content and never catch mouse events.
//
// A shape either flows (claims horizontal room after the icon, pushing the
// label right: meters, progress bars) or overlays (draws without affecting
// layout: dividers, highlights). width -1 on an overlay resolves to the
// item's content width; height -1 resolves to the bar height.

enum shape_type {
  SHAPE_RECT = 0,
  SHAPE_ROUND_RECT,
  SHAPE_LINE,
  SHAPE_CIRCLE,
};

struct shape {
  enum shape_type type;
  bool drawing;
  bool flow;
  char* name;
  struct color color;
  int x;
  int y;
  int width;
  int height;
  uint32_t radius;
  uint32_t repeat;
  uint32_t gap;
  char anchor; // 'l' content left, 'i' icon end, 'e' content right
  CGRect bounds;
};

struct shape_list {
  struct shape** shapes;
  uint32_t count;
};

void shapes_init(struct shape_list* list);
void shapes_destroy(struct shape_list* list);
void shapes_copy(struct shape_list* list, struct shape_list* source);

struct shape* shapes_get(struct shape_list* list, const char* name);
uint32_t shapes_get_length(struct shape_list* list);

void shapes_calculate_bounds(struct shape_list* list,
                             uint32_t content_x,
                             uint32_t icon_end_x,
                             uint32_t content_right_x,
                             uint32_t content_y,
                             uint32_t content_width,
                             uint32_t canvas_height        );

void shapes_draw(struct shape_list* list, CGContextRef context);
void shapes_serialize(struct shape_list* list, char* indent, FILE* rsp);
bool shapes_parse_sub_domain(struct shape_list* list,
                             FILE* rsp,
                             struct token entry,
                             char* message                 );
