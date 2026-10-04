#include "shapes.h"
#include "misc/defines.h"

#define SHAPE_TYPE                      "type"
#define SHAPE_ANCHOR                    "anchor"
#define SHAPE_REPEAT                    "repeat"
#define SHAPE_GAP                       "gap"

static struct shape* shape_create(void) {
  struct shape* shape = malloc(sizeof(struct shape));
  memset(shape, 0, sizeof(struct shape));
  shape->drawing = true;
  shape->type = SHAPE_RECT;
  shape->color = g_transparent;
  shape->anchor = 'i';
  shape->width = 10;
  shape->height = 10;
  shape->repeat = 1;
  shape->gap = 0;
  return shape;
}

void shapes_init(struct shape_list* list) {
  list->shapes = NULL;
  list->count = 0;
}

void shapes_destroy(struct shape_list* list) {
  for (uint32_t i = 0; i < list->count; i++) {
    if (list->shapes[i]->name) free(list->shapes[i]->name);
    free(list->shapes[i]);
  }
  if (list->shapes) free(list->shapes);
  shapes_init(list);
}

void shapes_copy(struct shape_list* list, struct shape_list* source) {
  shapes_destroy(list);
  for (uint32_t i = 0; i < source->count; i++) {
    struct shape* shape = shape_create();
    struct shape* src = source->shapes[i];
    shape->type = src->type;
    shape->drawing = src->drawing;
    shape->flow = src->flow;
    shape->name = src->name ? string_copy(src->name) : NULL;
    shape->color = src->color;
    shape->x = src->x;
    shape->y = src->y;
    shape->width = src->width;
    shape->height = src->height;
    shape->radius = src->radius;
    shape->repeat = src->repeat;
    shape->gap = src->gap;
    shape->anchor = src->anchor;
    list->shapes = realloc(list->shapes, sizeof(struct shape*) * (list->count + 1));
    list->shapes[list->count++] = shape;
  }
}

struct shape* shapes_get(struct shape_list* list, const char* name) {
  for (uint32_t i = 0; i < list->count; i++) {
    if (list->shapes[i]->name && strcmp(list->shapes[i]->name, name) == 0)
      return list->shapes[i];
  }

  struct shape* shape = shape_create();
  shape->name = string_copy((char*)name);
  list->shapes = realloc(list->shapes, sizeof(struct shape*) * (list->count + 1));
  list->shapes[list->count++] = shape;
  return shape;
}

uint32_t shapes_get_length(struct shape_list* list) {
  uint32_t length = 0;
  for (uint32_t i = 0; i < list->count; i++) {
    struct shape* shape = list->shapes[i];
    if (!shape->drawing || !shape->flow) continue;
    length += shape->x + shape->repeat * shape->width
              + (shape->repeat > 1 ? (shape->repeat - 1) * shape->gap : 0);
  }
  return length;
}

void shapes_calculate_bounds(struct shape_list* list,
                             uint32_t content_x,
                             uint32_t icon_end_x,
                             uint32_t content_right_x,
                             uint32_t content_y,
                             uint32_t content_width,
                             uint32_t canvas_height        ) {
  for (uint32_t i = 0; i < list->count; i++) {
    struct shape* shape = list->shapes[i];
    if (!shape->drawing) continue;

    uint32_t x = content_x;
    if (shape->anchor == 'i') x = icon_end_x;
    else if (shape->anchor == 'e') x = content_right_x;

    uint32_t width = shape->width < 0 ? content_width : (uint32_t)shape->width;
    uint32_t height = shape->height < 0 ? canvas_height : (uint32_t)shape->height;

    shape->bounds = CGRectMake((int)x + shape->x,
                               (int)content_y + shape->y,
                               width,
                               height                   );
  }
}

void shapes_draw(struct shape_list* list, CGContextRef context) {
  for (uint32_t i = 0; i < list->count; i++) {
    struct shape* shape = list->shapes[i];
    if (!shape->drawing) continue;

    CGContextSetRGBFillColor(context,
                             shape->color.r,
                             shape->color.g,
                             shape->color.b,
                             shape->color.a  );

    uint32_t stride = shape->width + shape->gap;
    for (uint32_t r = 0; r < shape->repeat; r++) {
      CGRect bounds = shape->bounds;
      bounds.origin.x += r * stride;

      if (shape->type == SHAPE_ROUND_RECT) {
        CGPathRef path = CGPathCreateWithRoundedRect(bounds,
                                                     shape->radius,
                                                     shape->radius,
                                                     NULL         );
        CGContextAddPath(context, path);
        CGContextFillPath(context);
        CFRelease(path);
      } else if (shape->type == SHAPE_CIRCLE) {
        CGContextFillEllipseInRect(context, bounds);
      } else {
        // SHAPE_RECT and SHAPE_LINE; a line is a rect with its height
        // clamped to the last pixel row it covers.
        if (shape->type == SHAPE_LINE && bounds.size.height > 1)
          bounds.origin.y += bounds.size.height - 1;
        if (shape->type == SHAPE_LINE) bounds.size.height = 1;
        CGContextFillRect(context, bounds);
      }
    }
  }
}

void shapes_serialize(struct shape_list* list, char* indent, FILE* rsp) {
  for (uint32_t i = 0; i < list->count; i++) {
    struct shape* shape = list->shapes[i];
    const char* type = shape->type == SHAPE_ROUND_RECT ? "round_rect"
                     : shape->type == SHAPE_CIRCLE     ? "circle"
                     : shape->type == SHAPE_LINE       ? "line"
                                                       : "rect";
    if (i > 0) fprintf(rsp, ",\n");
    fprintf(rsp, "%s{\n"
                 "%s%s\"name\": \"%s\",\n"
                 "%s%s\"type\": \"%s\",\n"
                 "%s%s\"drawing\": \"%s\",\n"
                 "%s%s\"flow\": \"%s\",\n"
                 "%s%s\"color\": \"0x%x\",\n"
                 "%s%s\"x\": %d,\n"
                 "%s%s\"y\": %d,\n"
                 "%s%s\"width\": %d,\n"
                 "%s%s\"height\": %d,\n"
                 "%s%s\"radius\": %u,\n"
                 "%s%s\"repeat\": %u,\n"
                 "%s%s\"gap\": %u,\n"
                 "%s%s\"anchor\": \"%c\"",
                 indent,
                 indent, indent, shape->name ? shape->name : "",
                 indent, indent, type,
                 indent, indent, format_bool(shape->drawing),
                 indent, indent, format_bool(shape->flow),
                 indent, indent, shape->color.hex,
                 indent, indent, shape->x,
                 indent, indent, shape->y,
                 indent, indent, shape->width,
                 indent, indent, shape->height,
                 indent, indent, shape->radius,
                 indent, indent, shape->repeat,
                 indent, indent, shape->gap,
                 indent, indent, shape->anchor             );
    fprintf(rsp, "\n%s}", indent);
  }
}

bool shapes_parse_sub_domain(struct shape_list* list,
                             FILE* rsp,
                             struct token entry,
                             char* message                 ) {
  struct key_value_pair pair = get_key_value_pair(entry.text, '.');
  if (!pair.key || !pair.value) {
    respond(rsp, "[!] Shapes: Expected 'shape.<name>.<property>'\n");
    return false;
  }

  struct token property = { pair.value, strlen(pair.value) };
  struct shape* shape = shapes_get(list, pair.key);

  if (token_equals(property, SHAPE_TYPE)) {
    struct token token = get_token(&message);
    if (token_equals(token, ARGUMENT_ROUND_RECT)) shape->type = SHAPE_ROUND_RECT;
    else if (token_equals(token, ARGUMENT_LINE)) shape->type = SHAPE_LINE;
    else if (token_equals(token, ARGUMENT_CIRCLE)) shape->type = SHAPE_CIRCLE;
    else shape->type = SHAPE_RECT;
    return true;
  } else if (token_equals(property, PROPERTY_COLOR)) {
    struct token token = get_token(&message);
    return color_set_hex(&shape->color, token_to_int(token));
  } else if (token_equals(property, PROPERTY_XOFFSET)) {
    struct token token = get_token(&message);
    shape->x = token_to_int(token);
    return true;
  } else if (token_equals(property, PROPERTY_YOFFSET)) {
    struct token token = get_token(&message);
    shape->y = token_to_int(token);
    return true;
  } else if (token_equals(property, PROPERTY_WIDTH)) {
    struct token token = get_token(&message);
    shape->width = token_to_int(token);
    return true;
  } else if (token_equals(property, PROPERTY_HEIGHT)) {
    struct token token = get_token(&message);
    shape->height = token_to_int(token);
    return true;
  } else if (token_equals(property, PROPERTY_CORNER_RADIUS)) {
    struct token token = get_token(&message);
    shape->radius = token_to_int(token);
    return true;
  } else if (token_equals(property, SHAPE_REPEAT)) {
    struct token token = get_token(&message);
    shape->repeat = token_to_int(token);
    return true;
  } else if (token_equals(property, SHAPE_GAP)) {
    struct token token = get_token(&message);
    shape->gap = token_to_int(token);
    return true;
  } else if (token_equals(property, SHAPE_ANCHOR)) {
    struct token token = get_token(&message);
    shape->anchor = token.text[0];
    return true;
  } else if (token_equals(property, PROPERTY_FLOW)) {
    struct token token = get_token(&message);
    shape->flow = evaluate_boolean_state(token, shape->flow);
    return true;
  } else if (token_equals(property, PROPERTY_DRAWING)) {
    struct token token = get_token(&message);
    shape->drawing = evaluate_boolean_state(token, shape->drawing);
    return true;
  }
  else {
    respond(rsp, "[!] Shapes: Invalid property '%s'\n", property.text);
    return false;
  }
}
