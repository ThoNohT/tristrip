#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#define NOH_IMPLEMENTATION
#include "noh.h"

#define BACKGROUND_COLOR CLITERAL(Color) { 10, 10, 10, 255 }
#define GRID_COLOR CLITERAL(Color) { 25, 25, 25, 255 }
#define X_AXIS_COLOR CLITERAL(Color) { 0, 0, 200, 100 }
#define Y_AXIS_COLOR CLITERAL(Color) { 200, 0, 0, 100 }

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
    Align_Top_Left,
    Align_Top_Right,
    Align_Top_Center,
    Align_Middle_Left,
    Align_Middle_Right,
    Align_Middle_Center,
    Align_Bottom_Left,
    Align_Bottom_Right,
    Align_Bottom_Center
} Text_Align;

/// Draws text at the specified position, where it is aligned to this position using the specified alignment.
void draw_text(char *text, Text_Align align, int font_size, int anchor_x, int anchor_y, Color color) {
    Font font = GetFontDefault();
    Vector2 text_size = MeasureTextEx(font, text, font_size, 1);
    Vector2 top_left = { .x = anchor_x, .y = anchor_y };

    // Align vertically.
    switch (align) {
        case Align_Middle_Left:
        case Align_Middle_Center:
        case Align_Middle_Right:
            top_left.y -= text_size.y / 2;
            break;
        case Align_Bottom_Left:
        case Align_Bottom_Right:
        case Align_Bottom_Center:
            top_left.y -= text_size.y;
            break;
        default: break;
    }

    // Align horizontally.
    switch (align) {
         case Align_Top_Right:
         case Align_Middle_Right:
         case Align_Bottom_Right:
             top_left.x -= text_size.x;
             break;
         case Align_Top_Center:
         case Align_Middle_Center:
         case Align_Bottom_Center:
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
    DrawLineV(grid_to_screen_p(center, -x_steps, 0), grid_to_screen_p(center,  x_steps, 0), X_AXIS_COLOR);
    // y-axis
    DrawLineV(grid_to_screen_p(center, 0, -y_steps), grid_to_screen_p(center, 0, y_steps), Y_AXIS_COLOR);
}

void draw_mouse_pos(Noh_Arena *arena, Vector2 mouse, float x, float y) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "%i, %i", (int)mouse.x, (int)mouse.y);
    draw_text(text, Align_Top_Right, 20, x, y, LIME);
    noh_arena_rewind(arena);
}

// Draw a number near a grid point.
void draw_number(Noh_Arena *arena, Vector2 center, Vector2 pos, int number) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "%i", number);
    Vector2 text_pos = Vector2Add(grid_to_screen(center, pos), CLITERAL(Vector2) { 10, -10 });
    DrawText(text, text_pos.x, text_pos.y, 20, ORANGE);
    noh_arena_rewind(arena);
}

void draw_active_layer(Noh_Arena *arena, Layers *layers, float x, float y) {
    noh_arena_save(arena);
    char *text = noh_arena_sprintf(arena, "Layer: %zu", layers->active_layer);
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

int main() {
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Triangle strip visualizer");
    SetWindowMonitor(0);

    Noh_Arena arena = noh_arena_init(1 KB);
    Layers layers = {0};
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
        draw_active_layer(&arena, &layers, 10, 40);

        // Update
        // Usage: Left click to add a point.
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            add_point(&points, mouse);
        }

        // Usage: Right click to remove a point.
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            remove_point(&points, mouse);
        }

        // Usage: Hold left button to move a point.
        static int moving_index = -1;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && moving_index == -1) {
            moving_index = start_moving(&points, mouse);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && moving_index > -1) {
            stop_moving(&points, moving_index, mouse);
            moving_index = -1;
        }

        // Usage: scroll the mouse wheel to move a point's position in the list.
        Vector2 scroll = GetMouseWheelMoveV();
        if (scroll.y > 0) {
            move_point_index(&points, mouse, -1);
        } else if (scroll.y < 0) {
            move_point_index(&points, mouse, 1);
        }

        // Draw
        DrawCircleV(grid_to_screen(screen_center, mouse), 3, GREEN);

        noh_arena_save(&arena);
        Vector2 *screen_points = noh_arena_alloc(&arena, sizeof(Vector2) * points.count);
        for (size_t i = 0; i < points.count; i++) {
            screen_points[i] = grid_to_screen(screen_center, points.elems[i]);
        }

        DrawTriangleStrip(screen_points, points.count, CLITERAL(Color) { 255, 0, 0, 255 });

        for (size_t i = 1; i < points.count; i++) {
            DrawLineV(screen_points[i-1], screen_points[i], GREEN);
        }


        for (size_t i = 0; i < points.count; i++) {
            if ((int)i == moving_index) {
                DrawCircleV(screen_points[i], 7, WHITE);
            } else {
                DrawCircleV(screen_points[i], 5, ORANGE);
            }
            draw_number(&arena, screen_center, points.elems[i], i);
        }
        noh_arena_rewind(&arena);

        EndDrawing();
    }

    CloseWindow();
}
