#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#define NOH_IMPLEMENTATION
#include "noh.h"

#define BACKGROUND_COLOR CLITERAL(Color) { 10, 10, 10, 255 }
#define GRID_COLOR CLITERAL(Color) { 25, 25, 25, 255 }
#define X_AXIS_COLOR CLITERAL(Color) { 0, 0, 200, 100 }
#define Y_AXIS_COLOR CLITERAL(Color) { 200, 0, 0, 100 }

#define TRIANGLE_STRIP_COLOR RED
#define TRIANGLE_LINES_COLOR GREEN
#define POINT_DRAGGING_COLOR WHITE
#define POINT_NORMAL_COLOR ORANGE
#define POINT_MOUSE_COLOR GREEN
#define POINT_NUMBER_COLOR ORANGE
#define CONNECTION_COLOR CLITERAL(Color) { 0, 64, 255, 255 }

typedef struct {
    Vector2 *elems;
    size_t count;
    size_t capacity;
} Points;

typedef struct {
    Points *elems;
    size_t count;
    size_t capacity;

    size_t active_layer;
    int comparison_layer; // I want to use -1 to indicate no comparison.
} Layers;

const int GRID_SPACING = 50;

typedef enum {
    Align_Top_Left, Align_Top_Right, Align_Top_Center,
    Align_Middle_Left, Align_Middle_Right, Align_Middle_Center,
    Align_Bottom_Left, Align_Bottom_Right, Align_Bottom_Center
} Text_Align;

/// Draws text at the specified position, where it is aligned to this position using the specified alignment.
void draw_text(char *text, Text_Align align, int font_size, int anchor_x, int anchor_y, Color color) {
    Font font = GetFontDefault();
    Vector2 text_size = MeasureTextEx(font, text, font_size, 1);
    Vector2 top_left = { .x = anchor_x, .y = anchor_y };

    // Align vertically.
    switch (align) {
        case Align_Middle_Left: case Align_Middle_Center: case Align_Middle_Right:
            top_left.y -= text_size.y / 2;
            break;
        case Align_Bottom_Left: case Align_Bottom_Right: case Align_Bottom_Center:
            top_left.y -= text_size.y;
            break;
        default: break;
    }

    // Align horizontally.
    switch (align) {
         case Align_Top_Right: case Align_Middle_Right: case Align_Bottom_Right:
             top_left.x -= text_size.x;
             break;
         case Align_Top_Center: case Align_Middle_Center: case Align_Bottom_Center:
             top_left.x -= text_size.x / 2;
             break;
        default: break;
    }

    DrawText(text, top_left.x, top_left.y, font_size, color);
}

/// Convert a point in grid coordinates to screen coordinates, provided the center point of the screen.
Vector2 grid_to_screen(Vector2 center, Vector2 point) {
    // The center is (0, 0), from there ever GRID_SPACING pixels adds one grid coordinate point.
    return Vector2Add(center, Vector2Scale(point, GRID_SPACING)); 
}

/// Convert a point in grid coordinates to screen coordinates, provided the center point of the screen.
Vector2 grid_to_screen_p(Vector2 center, float x, float y) {
    return grid_to_screen(center, CLITERAL(Vector2) { x, y });
}

/// Convert a point in screen coordinates to grid coordinates, provided the center point of the screen.
Vector2 screen_to_grid(Vector2 center, Vector2 point) {
    Vector2 point2 = Vector2Scale(Vector2Subtract(point, center), 1.0 / GRID_SPACING);
    Vector2 rounded = { .x = roundf(point2.x), .y = roundf(point2.y) };
    return rounded;
}

/// Convert a point in screen coordinates to grid coordinates, provided the center point of the screen.
Vector2 screen_to_grid_p(Vector2 center, float x, float y) {
    return screen_to_grid(center, CLITERAL(Vector2) { x, y });
}

Vector2 get_screen_size() {
    Vector2 result = { .x = GetScreenWidth(), .y = GetScreenHeight() };
    return result;
}

void draw_grid_and_axes(Vector2 center) {
    int x_steps = floorf(center.x / GRID_SPACING);
    int y_steps = floorf(center.y / GRID_SPACING);

    // Rows
    for (int row = 0; row <= 2*y_steps; row++)
        DrawLineV(
            grid_to_screen_p(center, -x_steps, row - y_steps),
            grid_to_screen_p(center, x_steps, row - y_steps),
            GRID_COLOR);
    // Cols
    for (int col = 0; col <= 2*x_steps; col++)
        DrawLineV(
            grid_to_screen_p(center, col - x_steps, -y_steps),
            grid_to_screen_p(center, col - x_steps, y_steps),
            GRID_COLOR);
    // x-axis
    DrawLineV(
        grid_to_screen_p(center, -x_steps, 0),
        grid_to_screen_p(center,  x_steps, 0),
        X_AXIS_COLOR);
    // y-axis
    DrawLineV(
        grid_to_screen_p(center, 0, -y_steps),
        grid_to_screen_p(center, 0, y_steps),
        Y_AXIS_COLOR);
}

void draw_mouse_pos(Noh_Arena *arena, Vector2 mouse, float x, float y) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "%i, %i", (int)mouse.x, (int)mouse.y);
    draw_text(text, Align_Top_Right, 20, x, y, LIME);
    noh_arena_rewind(arena);
}

void draw_number(Noh_Arena *arena, Vector2 pos, int number, Color color, bool other_side) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "%i", number);
    Vector2 text_pos = Vector2Add(pos, CLITERAL(Vector2) { 15, -15 });
    if (other_side) text_pos = Vector2Subtract(pos, CLITERAL(Vector2) { 15, -15 }); 
    draw_text(text, Align_Middle_Center, 29, text_pos.x, text_pos.y, color);
    noh_arena_rewind(arena);
}

// Draw a number near a grid point.
void draw_grid_number(Noh_Arena *arena, Vector2 center, Vector2 pos, int number, Color color, bool other_side) {
    draw_number(arena, grid_to_screen(center, pos), number, color, other_side);
}

void draw_animation_ms(Noh_Arena *arena, size_t duration, float x, float y) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "Animation: %zums", duration);
    draw_text(text, Align_Top_Left, 20, x, y, LIME);
    noh_arena_rewind(arena);
}

void draw_active_layer(Noh_Arena *arena, Layers *layers, float x, float y) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "Layer: %zu", layers->active_layer);
    draw_text(text, Align_Top_Left, 20, x, y, LIME);
    noh_arena_rewind(arena);
}

void draw_comparison_layer(Noh_Arena *arena, Layers *layers, float x, float y) {
    if (layers->comparison_layer == -1) return;

    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "Compare: %zu", layers->comparison_layer);
    draw_text(text, Align_Top_Left, 20, x, y, LIME);
    noh_arena_rewind(arena);
}

void add_point(Points *points, Vector2 pos) {
    bool point_exists = false;
    for (size_t i = 0; i < points->count; i++) {
        if (points->elems[i].x == pos.x && points->elems[i].y == pos.y) {
            point_exists = true;
            break;
        }
    }

    if (point_exists) return;

    Vector2 new_point = { .x = pos.x, .y = pos.y };
    noh_da_append(points, new_point);
}

void remove_point(Points *points, Vector2 pos) {
    int point_index = -1;
    for (size_t i = 0; i < points->count; i++) {
        if (points->elems[i].x == pos.x && points->elems[i].y == pos.y) {
            point_index = (int)i;
            break;
        }
    }

    if (point_index == -1) return;

    noh_da_remove_at(points, (size_t)point_index);
}

int start_moving(Points *points, Vector2 pos) {
    for (size_t i = 0; i < points->count; i++) {
        if (points->elems[i].x == pos.x && points->elems[i].y == pos.y) {
            return (int)i;
        }
    }

    return -1;
}

void stop_moving(Points *points, int moving_index, Vector2 pos) {
    bool point_exists = false;
    for (size_t i = 0; i < points->count; i++) {
        if (points->elems[i].x == pos.x && points->elems[i].y == pos.y) {
            point_exists = true;
            break;
        }
    }

    // If there is already another point at the target position, don't do anything.
    if (point_exists) return;

    points->elems[moving_index].x = pos.x;
    points->elems[moving_index].y = pos.y;
}

void move_point_index(Points *points, Vector2 pos, int direction) {
    int point_index = -1;
    for (size_t i = 0; i < points->count; i++) {
        if (points->elems[i].x == pos.x && points->elems[i].y == pos.y) {
            point_index = (int)i;
            break;
        }
    }

    if (point_index == -1) return;
    if (point_index + direction < 0) return;
    if (point_index + direction >= (int)points->count) return;

    Vector2 temp = points->elems[point_index];
    points->elems[point_index] = points->elems[point_index + direction];
    points->elems[point_index + direction] = temp;
}

void switch_to_next_layer(Layers *layers) {
    if (layers->active_layer == layers->count - 1) {
        if (layers->elems[layers->active_layer].count == 0) return;
        Points new_points = {0};
        noh_da_append(layers, new_points);
    }

    layers->active_layer++;
}

void switch_to_previous_layer(Layers *layers) {
    if (layers->active_layer == 0) return;

    if (layers->active_layer == layers->count - 1) {
        if (layers->elems[layers->active_layer].count == 0) layers->count--;
    }
    layers->active_layer--;
}

Vector2 *translate_points_to_screen(Noh_Arena *arena, Vector2 center, Points *points, size_t count) {
    noh_assert(count >= points->count && "Count is lower than points count");
    Vector2 *result = noh_arena_alloc(arena, sizeof(Vector2) * count);
    for (size_t i = 0; i < count; i++) {
        int j = min(points->count - 1, i);
        result[i] = grid_to_screen(center, points->elems[j]);
    }
    return result;
}

Vector2 *lerp_points(Noh_Arena *arena, Vector2 *from, Vector2 *to, float factor, size_t count) {
    Vector2 *result = noh_arena_alloc(arena, sizeof(Vector2) * count);
    for (size_t i = 0; i < count; i++) {
        result[i] = Vector2Lerp(from[i], to[i], factor);
    }

    return result;
}

void draw_layer(Noh_Arena *arena, Vector2 center, int moving_index, Points *points, bool comparison) {
    Color triStripColor = comparison ? ColorBrightness(TRIANGLE_STRIP_COLOR, -0.85) : TRIANGLE_STRIP_COLOR;
    Color triLinesColor = comparison ? ColorBrightness(TRIANGLE_LINES_COLOR, -0.85) : TRIANGLE_LINES_COLOR;
    Color pointColor = comparison ? ColorBrightness(POINT_NORMAL_COLOR, -0.85) : POINT_NORMAL_COLOR;
    Color pointNumberColor = comparison ? ColorBrightness(POINT_NUMBER_COLOR, -0.85) : POINT_NUMBER_COLOR;

    // Calculate points of active layer.
    noh_arena_save(arena);
    Vector2 *screen_points = translate_points_to_screen(arena, center, points, points->count);

    // Draw triangle strip of active layer
    DrawTriangleStrip(screen_points, points->count, triStripColor);

    // Draw lines between points of active layer.
    for (size_t i = 1; i < points->count; i++) {
        DrawLineV(screen_points[i-1], screen_points[i], triLinesColor);
    }

    // Draw points and numbers of active layer.
    for (size_t i = 0; i < points->count; i++) {
        if ((int)i == moving_index && !comparison) {
            DrawCircleV(screen_points[i], 7, POINT_DRAGGING_COLOR);
        } else {
            DrawCircleV(screen_points[i], 5, pointColor);
        }
        draw_grid_number(arena, center, points->elems[i], i, pointNumberColor, comparison);
    }

    noh_arena_rewind(arena);
}

void draw_connections(Vector2 center, Points *active, Points *comparison) {
    size_t no_connections = min(active->count, comparison->count);
    if (no_connections == 0) return;

    for (size_t i = 0; i < no_connections; i++) {
        DrawLineV(
            grid_to_screen_p(center, active->elems[i].x, active->elems[i].y),
            grid_to_screen_p(center, comparison->elems[i].x, comparison->elems[i].y),
            CONNECTION_COLOR);
    }
}

bool draw_animation(Noh_Arena *arena, Vector2 center, float *animation_time, Points *from, Points *to, size_t duration) {
    float ft = GetFrameTime();
    if(*animation_time <= 0.0) {
        *animation_time = 0.0;
        return false;
    }
    *animation_time -= ft / duration * 1000;

    size_t total_points = max(from->count, to->count);
    size_t shared_points = min(from->count, to->count);
    noh_arena_save(arena);
    Vector2 *from_screen = translate_points_to_screen(arena, center, from, total_points);
    Vector2 *to_screen = translate_points_to_screen(arena, center, to, total_points);
    Vector2 *int_screen = lerp_points(arena, from_screen, to_screen, *animation_time, total_points);

    DrawTriangleStrip(int_screen, total_points, TRIANGLE_STRIP_COLOR);
    for (size_t i = 1; i < shared_points; i++) {
        DrawLineV(int_screen[i-1], int_screen[i], TRIANGLE_LINES_COLOR);
    }
    for (size_t i = 0; i < shared_points; i++) {
        DrawCircleV(int_screen[i], 5, POINT_NORMAL_COLOR);
        draw_number(arena, int_screen[i], i, POINT_NUMBER_COLOR, false);
    }

    noh_arena_rewind(arena);

    return true;
}

int main() {
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Triangle strip visualizer");
    SetWindowMonitor(0);

    Noh_Arena arena = noh_arena_init(1 KB);
    Layers layers = {0};
    layers.comparison_layer = -1;
    Points points = {0};
    noh_da_append(&layers, points);

    while (!WindowShouldClose()) {
        Vector2 screen_size = get_screen_size();
        Vector2 screen_center = Vector2Scale(screen_size, 0.5);
        Vector2 mouse = screen_to_grid(screen_center, GetMousePosition());

        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);

        // Hud
        draw_grid_and_axes(screen_center);
        DrawFPS(10, 10);
        draw_mouse_pos(&arena, mouse, screen_size.x - 10, 10);
        static size_t animation_ms = 200;
        draw_animation_ms(&arena, animation_ms, 10, 40);
        draw_active_layer(&arena, &layers, 10, 70);
        draw_comparison_layer(&arena, &layers, 10, 100);

        Points *active_points = &layers.elems[layers.active_layer];

        // Update
        // Usage: Left click to add a point.
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            add_point(active_points, mouse);
        }

        // Usage: Right click to remove a point.
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            remove_point(active_points, mouse);
        }

        // Usage: Hold left button to move a point.
        static int moving_index = -1;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && moving_index == -1) {
            moving_index = start_moving(active_points, mouse);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && moving_index > -1) {
            stop_moving(active_points, moving_index, mouse);
            moving_index = -1;
        }

        // Usage: scroll the mouse wheel to move a point's position in the list.
        Vector2 scroll = GetMouseWheelMoveV();
        if (scroll.y > 0) {
            move_point_index(active_points, mouse, -1);
        } else if (scroll.y < 0) {
            move_point_index(active_points, mouse, 1);
        }

        // Usage: left and right arrow keys to move between layers.
        // A new layer is created when moving right from the last layer if it has points.
        if (IsKeyPressed(KEY_RIGHT)) {
            switch_to_next_layer(&layers);
        } else if (IsKeyPressed(KEY_LEFT)) {
            switch_to_previous_layer(&layers);
        }

        // Usage: space to mark a layer as comparison layer.
        // Pressing space on the active comparison layer disables it.
        if (IsKeyPressed(KEY_SPACE)) {
            if (layers.comparison_layer == (int)layers.active_layer) {
                layers.comparison_layer = -1;
            } else {
                layers.comparison_layer = (int)layers.active_layer;
            }
        }

        // Usage: up and down arrow keys increase and decrease animation time.
        if (IsKeyPressed(KEY_UP)) {
            if (animation_ms < 6400) animation_ms *= 2;
        } else if (IsKeyPressed(KEY_DOWN)) {
            if (animation_ms > 50) animation_ms /= 2;
        }

#define ACTIVE &layers.elems[layers.active_layer]
#define COMPARE &layers.elems[layers.comparison_layer]
#define HAS_COMPARISON                                     \
    layers.comparison_layer >= 0                           \
    && layers.comparison_layer < (int)layers.count         \
    && layers.comparison_layer != (int)layers.active_layer \
    && (ACTIVE)->count > 0 && (COMPARE)->count > 0

        static float animation_time = 0.0;
        if (IsKeyPressed(KEY_A) && HAS_COMPARISON) {
            int temp = layers.active_layer;
            layers.active_layer = layers.comparison_layer;
            layers.comparison_layer = temp;
            animation_time = 1.0;
        }

        // Draw
        if (!draw_animation(&arena, screen_center, &animation_time, ACTIVE, COMPARE, animation_ms)) {
            if (HAS_COMPARISON) draw_layer(&arena, screen_center, moving_index, COMPARE, true);
            draw_layer(&arena, screen_center, moving_index, ACTIVE, false);
            if (HAS_COMPARISON) draw_connections(screen_center, ACTIVE, COMPARE);
        }

        // Draw which point the mouse is hovering over.
        DrawCircleV(grid_to_screen(screen_center, mouse), 3, POINT_MOUSE_COLOR);

        EndDrawing();
    }

    CloseWindow();
}
