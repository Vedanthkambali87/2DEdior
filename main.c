#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
 
/* ── Grid dimensions ─────────────────────────────────────── */
#define ROWS  25
#define COLS  60
#define MAX_OBJECTS 20
 
/* ── Characters used ─────────────────────────────────────── */
#define FILL_CHAR   '*'
#define EDGE_CHAR   '_'
#define EMPTY_CHAR  ' '
 
/* ── Shape type codes ─────────────────────────────────────── */
#define SHAPE_CIRCLE   1
#define SHAPE_RECT     2
#define SHAPE_LINE     3
#define SHAPE_TRIANGLE 4
 
/* ============================================================
 *  DATA STRUCTURES
 * ============================================================ */
 
/* One shape object */
typedef struct {
    int  type;      /* SHAPE_* constant        */
    int  active;    /* 1 = exists, 0 = deleted */
    int  visible;   /* 1 = shown,  0 = hidden  */
 
    /* Circle  : r, c, radius                  */
    /* Rect    : r, c, height, width           */
    /* Line    : r0,c0, r1,c1                  */
    /* Triangle: r0,c0, r1,c1, r2,c2           */
    int r, c, radius;
    int height, width;
    int r0, c0, r1, c1, r2, c2;
} Object;
 
/* The canvas – a 2D array of characters */
char canvas[ROWS][COLS];
 
/* Object list */
Object objects[MAX_OBJECTS];
int    object_count = 0;
 
/* ============================================================
 *  FUNCTION PROTOTYPES
 * ============================================================ */
 
/* Canvas */
void clear_canvas(void);
void display_canvas(void);
void render_all(void);
 
/* Drawing primitives */
void set_cell(int r, int c, char ch);
void draw_line(int r0, int c0, int r1, int c1, char ch);
void draw_circle(int cr, int cc, int radius);
void draw_rect(int r, int c, int height, int width);
void draw_triangle(int r0, int c0, int r1, int c1, int r2, int c2);
 
/* Object management */
int  add_object(void);
void delete_object(int id);
void modify_object(int id);
void toggle_visibility(int id);
void list_objects(void);
void print_shape_info(Object *obj, int id);
 
/* Menu helpers */
void menu(void);
void menu_add(void);
void menu_delete(void);
void menu_modify(void);
int  get_int(const char *prompt);
 
/* ============================================================
 *  CANVAS FUNCTIONS
 * ============================================================ */
 
/*
 * clear_canvas()
 * Fills every cell of the 2D array with EMPTY_CHAR.
 * Uses nested for-loops — a core 1st-year concept.
 */
void clear_canvas(void)
{
    int r, c;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            canvas[r][c] = EMPTY_CHAR;
}
 
/*
 * display_canvas()
 * Prints the 2D character array to the terminal,
 * surrounded by a border made of dashes and pipes.
 */
void display_canvas(void)
{
    int r, c;
 
    /* Top border */
    printf("  +");
    for (c = 0; c < COLS; c++) printf("-");
    printf("+\n");
 
    /* Rows with row numbers and side borders */
    for (r = 0; r < ROWS; r++) {
        printf("%2d|", r);               /* row label */
        for (c = 0; c < COLS; c++)
            putchar(canvas[r][c]);
        printf("|\n");
    }
 
    /* Bottom border */
    printf("  +");
    for (c = 0; c < COLS; c++) printf("-");
    printf("+\n");
 
    /* Column ruler (every 10 columns) */
    printf("   ");
    for (c = 0; c < COLS; c++)
        printf("%c", c % 10 == 0 ? ('0' + (c / 10) % 10) : ' ');
    printf("\n   ");
    for (c = 0; c < COLS; c++)
        printf("%d", c % 10);
    printf("\n");
}
 
/*
 * render_all()
 * Clears canvas, then redraws every active+visible object.
 * This is the "display function" that uses the 2D array.
 */
void render_all(void)
{
    int i;
    clear_canvas();
 
    for (i = 0; i < object_count; i++) {
        Object *o = &objects[i];
        if (!o->active || !o->visible) continue;
 
        switch (o->type) {
            case SHAPE_CIRCLE:
                draw_circle(o->r, o->c, o->radius);
                break;
            case SHAPE_RECT:
                draw_rect(o->r, o->c, o->height, o->width);
                break;
            case SHAPE_LINE:
                draw_line(o->r0, o->c0, o->r1, o->c1, FILL_CHAR);
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(o->r0, o->c0, o->r1, o->c1, o->r2, o->c2);
                break;
        }
    }
}
 
/* ============================================================
 *  DRAWING PRIMITIVES
 * ============================================================ */
 
/*
 * set_cell()
 * Safely sets canvas[r][c] only if within bounds.
 * Boundary checking prevents array overflow — important!
 */
void set_cell(int r, int c, char ch)
{
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = ch;
}
 
/*
 * draw_line()
 * Bresenham's Line Algorithm — integer-only, no floats needed.
 *
 * Idea: for a line from (r0,c0) to (r1,c1), decide each step
 *       whether to move in r or c using an error accumulator.
 *
 *   dr = |r1 - r0|,  dc = |c1 - c0|
 *   err = dr - dc
 *   Each step: if err > 0, move in r-direction; else in c-direction.
 */
void draw_line(int r0, int c0, int r1, int c1, char ch)
{
    int dr = abs(r1 - r0);
    int dc = abs(c1 - c0);
    int sr = (r0 < r1) ? 1 : -1;  /* step direction for rows */
    int sc = (c0 < c1) ? 1 : -1;  /* step direction for cols */
    int err = dr - dc;
    int e2;
 
    while (1) {
        set_cell(r0, c0, ch);
        if (r0 == r1 && c0 == c1) break;  /* reached end point */
 
        e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r0 += sr; }
        if (e2 <  dr) { err += dr; c0 += sc; }
    }
}
 
/*
 * draw_circle()
 * Midpoint Circle Algorithm (integer arithmetic, no sqrt).
 *
 * Idea: start at the top of the circle (x=0, y=radius).
 *       Use a decision variable 'd' to choose the next pixel.
 *       The 8-way symmetry of a circle lets us plot 8 points
 *       for every (x,y) we compute — very efficient!
 *
 *   d = 1 - radius  (initial value)
 *   Each step: if d < 0,  d += 2x + 3
 *              if d >= 0, d += 2(x-y) + 5, y--
 *              x++
 */
void draw_circle(int cr, int cc, int radius)
{
    int x = 0, y = radius;
    int d = 1 - radius;
 
    /* Helper macro: plot all 8 symmetric points */
    #define PLOT8(dx, dy) \
        set_cell(cr+(dy), cc+(dx), FILL_CHAR); \
        set_cell(cr-(dy), cc+(dx), FILL_CHAR); \
        set_cell(cr+(dy), cc-(dx), FILL_CHAR); \
        set_cell(cr-(dy), cc-(dx), FILL_CHAR); \
        set_cell(cr+(dx), cc+(dy), FILL_CHAR); \
        set_cell(cr-(dx), cc+(dy), FILL_CHAR); \
        set_cell(cr+(dx), cc-(dy), FILL_CHAR); \
        set_cell(cr-(dx), cc-(dy), FILL_CHAR);
 
    PLOT8(x, y);
 
    while (x < y) {
        x++;
        if (d < 0)
            d += 2 * x + 1;
        else {
            y--;
            d += 2 * (x - y) + 1;
        }
        PLOT8(x, y);
    }
    #undef PLOT8
}
 
/*
 * draw_rect()
 * Draws a hollow rectangle.
 * Top/bottom edges: EDGE_CHAR ('_')
 * Left/right edges: FILL_CHAR ('*')
 *
 * (r,c) is the top-left corner.
 * height = number of rows, width = number of columns.
 */
void draw_rect(int r, int c, int height, int width)
{
    int i;
 
    /* Top and bottom edges */
    for (i = 0; i < width; i++) {
        set_cell(r,            c + i, EDGE_CHAR);
        set_cell(r + height-1, c + i, EDGE_CHAR);
    }
 
    /* Left and right edges (skip corners already drawn) */
    for (i = 1; i < height - 1; i++) {
        set_cell(r + i, c,           FILL_CHAR);
        set_cell(r + i, c + width-1, FILL_CHAR);
    }
}
 
/*
 * draw_triangle()
 * Draws a triangle by connecting 3 vertices with lines.
 * Vertices: (r0,c0), (r1,c1), (r2,c2)
 */
void draw_triangle(int r0, int c0, int r1, int c1, int r2, int c2)
{
    draw_line(r0, c0, r1, c1, FILL_CHAR);  /* side 1 */
    draw_line(r1, c1, r2, c2, FILL_CHAR);  /* side 2 */
    draw_line(r2, c2, r0, c0, FILL_CHAR);  /* side 3 */
}
 
/* ============================================================
 *  OBJECT MANAGEMENT
 * ============================================================ */
 
/*
 * list_objects()
 * Prints a table of all objects with their parameters.
 */
void list_objects(void)
{
    int i, any = 0;
 
    printf("\n  ID  TYPE       VISIBLE  PARAMETERS\n");
    printf("  --  ---------  -------  --------------------\n");
 
    for (i = 0; i < object_count; i++) {
        if (!objects[i].active) continue;
        print_shape_info(&objects[i], i);
        any = 1;
    }
 
    if (!any) printf("  (no objects)\n");
    printf("\n");
}
 
void print_shape_info(Object *obj, int id)
{
    const char *name;
    const char *vis = obj->visible ? "YES" : "NO ";
 
    switch (obj->type) {
        case SHAPE_CIRCLE:   name = "circle   "; break;
        case SHAPE_RECT:     name = "rectangle"; break;
        case SHAPE_LINE:     name = "line     "; break;
        case SHAPE_TRIANGLE: name = "triangle "; break;
        default:             name = "unknown  "; break;
    }
 
    printf("  %2d  %s  %s  ", id, name, vis);
 
    switch (obj->type) {
        case SHAPE_CIRCLE:
            printf("center=(%d,%d)  radius=%d", obj->r, obj->c, obj->radius);
            break;
        case SHAPE_RECT:
            printf("top-left=(%d,%d)  h=%d  w=%d",
                   obj->r, obj->c, obj->height, obj->width);
            break;
        case SHAPE_LINE:
            printf("(%d,%d) -> (%d,%d)", obj->r0, obj->c0, obj->r1, obj->c1);
            break;
        case SHAPE_TRIANGLE:
            printf("(%d,%d) (%d,%d) (%d,%d)",
                   obj->r0, obj->c0, obj->r1, obj->c1, obj->r2, obj->c2);
            break;
    }
    printf("\n");
}
 
/*
 * add_object()
 * Asks the user to choose a shape type and enter parameters.
 * Returns the new object's ID, or -1 if the list is full.
 */
int add_object(void)
{
    int type, id;
    Object *o;
 
    if (object_count >= MAX_OBJECTS) {
        printf("  ERROR: maximum %d objects reached.\n", MAX_OBJECTS);
        return -1;
    }
 
    printf("\n  Shape type:\n");
    printf("  1. Circle\n  2. Rectangle\n  3. Line\n  4. Triangle\n");
    type = get_int("  Choose [1-4]: ");
 
    if (type < 1 || type > 4) {
        printf("  Invalid type.\n");
        return -1;
    }
 
    /* Find the next slot (could be reused after a delete) */
    id = object_count;
    object_count++;
 
    o          = &objects[id];
    o->type    = type;
    o->active  = 1;
    o->visible = 1;
 
    switch (type) {
        case SHAPE_CIRCLE:
            o->r      = get_int("  Center row    : ");
            o->c      = get_int("  Center col    : ");
            o->radius = get_int("  Radius        : ");
            break;
 
        case SHAPE_RECT:
            o->r      = get_int("  Top-left row  : ");
            o->c      = get_int("  Top-left col  : ");
            o->height = get_int("  Height        : ");
            o->width  = get_int("  Width         : ");
            break;
 
        case SHAPE_LINE:
            o->r0 = get_int("  Start row     : ");
            o->c0 = get_int("  Start col     : ");
            o->r1 = get_int("  End row       : ");
            o->c1 = get_int("  End col       : ");
            break;
 
        case SHAPE_TRIANGLE:
            printf("  Vertex 1 —\n");
            o->r0 = get_int("    row: "); o->c0 = get_int("    col: ");
            printf("  Vertex 2 —\n");
            o->r1 = get_int("    row: "); o->c1 = get_int("    col: ");
            printf("  Vertex 3 —\n");
            o->r2 = get_int("    row: "); o->c2 = get_int("    col: ");
            break;
    }
 
    printf("  Object %d added.\n", id);
    return id;
}
 
/*
 * delete_object()
 * Marks an object as inactive (logical delete).
 * The slot is not reused in this simple implementation.
 */
void delete_object(int id)
{
    if (id < 0 || id >= object_count || !objects[id].active) {
        printf("  ERROR: no active object with ID %d.\n", id);
        return;
    }
    objects[id].active = 0;
    printf("  Object %d deleted.\n", id);
}
 
/*
 * modify_object()
 * Lets the user change any parameter of an existing object.
 */
void modify_object(int id)
{
    Object *o;
    int choice;
 
    if (id < 0 || id >= object_count || !objects[id].active) {
        printf("  ERROR: no active object with ID %d.\n", id);
        return;
    }
 
    o = &objects[id];
 
    printf("\n  Modifying object %d (%s).\n", id,
           o->type == SHAPE_CIRCLE   ? "circle"    :
           o->type == SHAPE_RECT     ? "rectangle" :
           o->type == SHAPE_LINE     ? "line"      : "triangle");
 
    switch (o->type) {
        case SHAPE_CIRCLE:
            printf("  1. Center row (%d)\n", o->r);
            printf("  2. Center col (%d)\n", o->c);
            printf("  3. Radius     (%d)\n", o->radius);
            choice = get_int("  Field to change [1-3]: ");
            if      (choice == 1) o->r      = get_int("  New center row : ");
            else if (choice == 2) o->c      = get_int("  New center col : ");
            else if (choice == 3) o->radius = get_int("  New radius     : ");
            break;
 
        case SHAPE_RECT:
            printf("  1. Top-left row (%d)\n", o->r);
            printf("  2. Top-left col (%d)\n", o->c);
            printf("  3. Height       (%d)\n", o->height);
            printf("  4. Width        (%d)\n", o->width);
            choice = get_int("  Field to change [1-4]: ");
            if      (choice == 1) o->r      = get_int("  New row    : ");
            else if (choice == 2) o->c      = get_int("  New col    : ");
            else if (choice == 3) o->height = get_int("  New height : ");
            else if (choice == 4) o->width  = get_int("  New width  : ");
            break;
 
        case SHAPE_LINE:
            printf("  1. Start row (%d)\n", o->r0);
            printf("  2. Start col (%d)\n", o->c0);
            printf("  3. End row   (%d)\n", o->r1);
            printf("  4. End col   (%d)\n", o->c1);
            choice = get_int("  Field to change [1-4]: ");
            if      (choice == 1) o->r0 = get_int("  New start row : ");
            else if (choice == 2) o->c0 = get_int("  New start col : ");
            else if (choice == 3) o->r1 = get_int("  New end row   : ");
            else if (choice == 4) o->c1 = get_int("  New end col   : ");
            break;
 
        case SHAPE_TRIANGLE:
            printf("  1. Vertex 1 row (%d)\n", o->r0);
            printf("  2. Vertex 1 col (%d)\n", o->c0);
            printf("  3. Vertex 2 row (%d)\n", o->r1);
            printf("  4. Vertex 2 col (%d)\n", o->c1);
            printf("  5. Vertex 3 row (%d)\n", o->r2);
            printf("  6. Vertex 3 col (%d)\n", o->c2);
            choice = get_int("  Field to change [1-6]: ");
            if      (choice == 1) o->r0 = get_int("  New value : ");
            else if (choice == 2) o->c0 = get_int("  New value : ");
            else if (choice == 3) o->r1 = get_int("  New value : ");
            else if (choice == 4) o->c1 = get_int("  New value : ");
            else if (choice == 5) o->r2 = get_int("  New value : ");
            else if (choice == 6) o->c2 = get_int("  New value : ");
            break;
    }
 
    printf("  Object %d updated.\n", id);
}
 
/*
 * toggle_visibility()
 * Hides a visible object or shows a hidden one.
 */
void toggle_visibility(int id)
{
    if (id < 0 || id >= object_count || !objects[id].active) {
        printf("  ERROR: no active object with ID %d.\n", id);
        return;
    }
    objects[id].visible = !objects[id].visible;
    printf("  Object %d is now %s.\n", id,
           objects[id].visible ? "visible" : "hidden");
}
 
/* ============================================================
 *  MENU & HELPERS
 * ============================================================ */
 
/*
 * get_int()
 * Prints a prompt and reads one integer from the user.
 */
int get_int(const char *prompt)
{
    int val;
    printf("%s", prompt);
    scanf("%d", &val);
    return val;
}
 
void menu(void)
{
    int choice, id;
 
    while (1) {
        /* Render and display the current canvas */
        render_all();
        display_canvas();
        list_objects();
 
        printf("=== MENU ===================\n");
        printf("  1. Add object\n");
        printf("  2. Delete object\n");
        printf("  3. Modify object\n");
        printf("  4. Toggle visibility\n");
        printf("  5. Clear all objects\n");
        printf("  0. Quit\n");
        printf("============================\n");
 
        choice = get_int("  Choice: ");
 
        switch (choice) {
            case 1:
                add_object();
                break;
 
            case 2:
                id = get_int("  Object ID to delete: ");
                delete_object(id);
                break;
 
            case 3:
                id = get_int("  Object ID to modify: ");
                modify_object(id);
                break;
 
            case 4:
                id = get_int("  Object ID to toggle: ");
                toggle_visibility(id);
                break;
 
            case 5:
                object_count = 0;
                printf("  All objects cleared.\n");
                break;
 
            case 0:
                printf("  Goodbye!\n");
                return;
 
            default:
                printf("  Invalid choice.\n");
                break;
        }
 
        printf("\n  Press Enter to continue...");
        getchar(); getchar();   /* consume newline */
    }
}
 
/* ============================================================
 *  MAIN
 * ============================================================ */
 
int main(void)
{
    printf("\n");
    printf("  ****  2D ASCII GRAPHICS EDITOR  ****\n");
    printf("  Grid: %d rows x %d cols\n", ROWS, COLS);
    printf("  Chars: '%c' (fill)  '%c' (edges)\n\n", FILL_CHAR, EDGE_CHAR);
 
    /* Initialise object list */
    memset(objects, 0, sizeof(objects));
    object_count = 0;
 
    /* Start with a demo scene */
    printf("  Loading demo scene...\n");
 
    /* Rectangle */
    objects[0].type = SHAPE_RECT; objects[0].active = objects[0].visible = 1;
    objects[0].r = 1; objects[0].c = 1; objects[0].height = 8; objects[0].width = 18;
 
    /* Circle */
    objects[1].type = SHAPE_CIRCLE; objects[1].active = objects[1].visible = 1;
    objects[1].r = 12; objects[1].c = 30; objects[1].radius = 8;
 
    /* Line */
    objects[2].type = SHAPE_LINE; objects[2].active = objects[2].visible = 1;
    objects[2].r0 = 0; objects[2].c0 = 40; objects[2].r1 = 15; objects[2].c1 = 58;
 
    /* Triangle */
    objects[3].type = SHAPE_TRIANGLE; objects[3].active = objects[3].visible = 1;
    objects[3].r0 = 1; objects[3].c0 = 30;
    objects[3].r1 = 8; objects[3].c1 = 20;
    objects[3].r2 = 8; objects[3].c2 = 40;
 
    object_count = 4;
 
    menu();
    return 0;
}