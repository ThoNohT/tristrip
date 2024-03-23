#ifndef NOH_H_
#define NOH_H_

#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
   #include <direct.h>
#else
    #include <sys/stat.h>
#endif // _WIN32


///////////////////////// Number definitions /////////////////////////

#ifndef int8
#define int8 signed char
#endif

#ifndef uint8
#define uint8 unsigned char
#endif

#ifndef int16
#define int16 short
#endif

#ifndef uint16
#define uint16 unsigned short
#endif

#ifndef uint
#define uint unsigned int
#endif

#ifndef _WIN32
#ifndef int64
#define int64 long
#endif
#endif 

#ifndef uint64
#define uint64 unsigned long
#endif

#ifndef KB
#define KB << 10
#endif

#ifndef MB
#define MB << 20
#endif

#ifndef GB
#define GB << 30
#endif

///////////////////////// Core stuff /////////////////////////

#define noh_array_len(array) (sizeof(array)/sizeof(array[0]))
#define noh_array_get(array, index) \
    (noh_assert(index >= 0), assert(index < noh_array_get(array)), array[index])

// Allows returning a file after performing some deferred code.
// Usage:
// Define a result variable before the first call of this macro.
// Place a defer label at the end of the function where the work is done.
// Return result at the end of the deferred work.
#define noh_return_defer(value) do { result = (value); goto defer; } while(0)

void* noh_realloc_check_(void *target, size_t size);

// Reallocates some memory and crashes if it failed.
#define noh_realloc_check(target, size) noh_realloc_check_((void*)(target), (size))

// Returns the next argument as a c-string, moves the argv pointer to the next argument and decreases argc.
char *noh_shift_args(int *argc, char ***argv);

///////////////////////// Time /////////////////////////

// Returns the result of subtracting the second timespec from the first timespec, in milliseconds.
// There is no absolute compare function, since it is assumed that a higher precision than milliseconds will not be
// needed, and cannot really be expected to be reliable.
long noh_diff_timespec_ms(const struct timespec *time1, const struct timespec *time2);

// Returns a timespec that represents the local time with the specified number of second and milliseconds added.
// Negative values will lead to a time in the past.
struct timespec noh_get_time_in(int seconds, long milliseconds);

// Adds the specified number of seconds and milliseconds to a timespec.
void noh_time_add(struct timespec *time, int seconds, long milliseconds);

///////////////////////// Logging /////////////////////////

// An assert macro that outputs a better format for use with vim's make command.
#define noh_assert(condition) {                                                    \
    if (!(condition)) {                                                            \
        printf("%s:%i: Assertion failed: %s. \n", __FILE__, __LINE__, #condition); \
        exit(1);                                                                   \
    }                                                                              \
}                                                                                  \

// Possible log levels.
typedef enum {
    NOH_INFO,
    NOH_WARNING,
    NOH_ERROR,
} Noh_Log_Level;

// Writes a formatted log message to stderr with the provided log level.
void noh_log(Noh_Log_Level level, const char *fmt, ...);

///////////////////////// Dynamic array /////////////////////////

#define NOH_DA_INIT_CAP 256

// Appends an element to a dynamic array, allocates more memory and moves all elements to newly allocated memory
// if needed.
#define noh_da_append(da, elem)                                                              \
do {                                                                                         \
    if ((da)->count >= (da)->capacity) {                                                     \
        (da)->capacity = (da)->capacity == 0 ? NOH_DA_INIT_CAP : (da)->capacity * 2;         \
        (da)->elems = noh_realloc_check((da)->elems, (da)->capacity * sizeof(*(da)->elems)); \
    }                                                                                        \
                                                                                             \
    (da)->elems[(da)->count++] = (elem);                                                     \
} while(0)

// Appends multiple elements to a dynamic array. Allocates more memory and moves all elements to newly allocated memory
// if needed.
#define noh_da_append_multiple(da, new_elems, new_elems_count)                               \
do {                                                                                         \
    if ((da)->count + new_elems_count > (da)->capacity) {                                    \
        if ((da)->capacity == 0) (da)->capacity = NOH_DA_INIT_CAP;                           \
        while ((da)->count + new_elems_count > (da)->capacity) (da)->capacity *= 2;          \
        (da)->elems = noh_realloc_check((da)->elems, (da)->capacity * sizeof(*(da)->elems)); \
    }                                                                                        \
                                                                                             \
    memcpy((da)->elems + (da)->count, new_elems, new_elems_count * sizeof(*(da)->elems));    \
    (da)->count += new_elems_count;                                                          \
} while (0)

// Removes the element at the specified location.
#define noh_da_remove_at(da, index)                              \
do {                                                             \
    noh_assert((index) < (da)->count && "Index out of bounds."); \
    (da)->count -= 1;                                            \
    if ((index) < (da)->count) {                                 \
        size_t elem_size = sizeof(*(da)->elems);                 \
        memmove(                                                 \
            (void*)(da)->elems + (index) * elem_size,            \
            (void*)(da)->elems + ((index) + 1) * elem_size,      \
            ((da)->count - (index)) * elem_size);                \
    }                                                            \
} while (0)

// Frees the elements in a dynamic array, and resets the count and capacity.
#define noh_da_free(da)       \
do {                          \
    if ((da)->capacity > 0) { \
        (da)->count = 0;      \
        (da)->capacity = 0;   \
        free((da)->elems);    \
    }                         \
} while (0)

// Resets the count of a dynamic array to 0.
#define noh_da_reset(da) \
do {                    \
    (da)->count = 0;    \
} while (0)

///////////////////////// Circular buffer /////////////////////////  

// Initializes a circular buffer, similar to a dynamic array, but adding elements should be done with noh_cb_insert.
// This call should be the only one to allocate memory to hold the data and set the capacity.
#define noh_cb_initialize(da, size) {                                                               \
    noh_assert((da)->capacity == 0 && "Cannot initialize an already initialized circular buffer."); \
    noh_assert((size) > 0 && "Cannot initialize an empty circular buffer.");                        \
                                                                                                    \
    (da)->capacity = size;                                                                          \
    (da)->elems = noh_realloc_check((da)->elems, (da)->capacity * sizeof(*(da)->elems));            \
    (da)->start = 0;                                                                                \
    (da)->count = 0;                                                                                \
}                                                                                                   \

// Inserts an element in a dynamic array as if it is a circular buffer, will not extend beyond the capacity of the
// dynamic array but instead overwrite the oldest element.
#define noh_cb_insert(da, elem)                                              \
do {                                                                         \
    noh_assert((da)->capacity > 0 && "Circular buffer is not initialized."); \
                                                                             \
    if ((da)->count < (da)->capacity) {                                      \
        (da)->elems[(da)->count++] = (elem);                                 \
    } else {                                                                 \
        (da)->elems[(da)->start] = (elem);                                   \
        (da)->start = ((da)->start + 1) % (da)->count;                       \
    }                                                                        \
} while(0)

///////////////////////// Arena /////////////////////////  

#define NOH_ARENA_INIT_CAP 1<<10

// Checkpoints in an arena.
typedef struct {
    size_t block_id;
    size_t offset_in_block;
} Noh_Arena_Checkpoint;

typedef struct {
    Noh_Arena_Checkpoint *elems;
    size_t count;
    size_t capacity;
} Noh_Arena_Checkpoints;

// Data blocks in an arena.
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} Noh_Arena_Data_Block;

typedef struct {
    Noh_Arena_Data_Block *elems;
    size_t count;
    size_t capacity;
} Noh_Arena_Data_Blocks;

// An arena for storing temporary data.
typedef struct {
    Noh_Arena_Data_Blocks blocks; // Blocks are always in order of increasing capacity.
    Noh_Arena_Checkpoints checkpoints;
    size_t active_block; // The index of the block up to which data has been allocated.
} Noh_Arena;

// Initialize an empty arena with the specified capacity. A checkpoint is also saved at the empty arena.
Noh_Arena noh_arena_init(size_t capacity);

// Resets the size of an arena to 0, keeping the data reserved. Any checkpoints are removed and one is saved at the
// start of the arena. Requires that the arena is initialized with noh_arena_init.
void noh_arena_reset(Noh_Arena *arena);

// Frees all data in an arena. Any checkpoints are removed. The arena is no longer initialized, and cannot be used
// anymore.
void noh_arena_free(Noh_Arena *arena);

// Ensures that there is room available for the requested size of data. Does not return a pointer to the data to the
// caller. Used if you want to pre-allocate a larger set of data that will later be filled by multiple allocations,
// keeping it in a single block.
void noh_arena_reserve(Noh_Arena *arena, size_t size);

// Allocates data in an arena of the requested size, returns the start of the data.
// Requires at least one checkpoint, either from noh_arena_init, noh_arena_reset or noh_arena_save.
void *noh_arena_alloc(Noh_Arena *arena, size_t size);

// Saves the current position in of the arena in a checkpoint. Requires that the arena is initialized with
// noh_arena_init.
void noh_arena_save(Noh_Arena *arena);

// Rewinds an arena to the last saved checkpoint. Requires at least one checkpoint.
void noh_arena_rewind(Noh_Arena *arena);

// Copies a c-string to the arena.
char *noh_arena_strdup(Noh_Arena *arena, const char *cstr);

// Prints the specified formatted string to the arena.
char *noh_arena_sprintf(Noh_Arena *arena, const char *format, ...);

///////////////////////// Strings /////////////////////////  

// Defines a string that can be extended.
typedef struct {
    char *elems;
    size_t count;
    size_t capacity;
} Noh_String;

// Appends a null-terminated string into a Noh_String.
void noh_string_append_cstr(Noh_String *string, const char *cstr);

// Appends null into a Noh_String.
void noh_string_append_null(Noh_String *string);

// Frees a Noh_String, freeing the memory used and settings the count and capacity to 0.
#define noh_string_free(string) noh_da_free(string)

// Resets a Noh_String, setting the count to 0.
#define noh_string_reset(string) noh_da_reset(string)

// Reads the contents of a file into a Noh_String.
bool noh_string_read_file(Noh_String *string, const char *filename);

///////////////////////// String view /////////////////////////  

// A view of a string, that does not own the data.
typedef struct {
    size_t count;
    const char *elems;
} Noh_String_View;

// Finds the first occurrence of the specified delimiter in a string view and returns the part of the string until 
// that delimiter. The string view itself is shrunk to start after the delimiter.
Noh_String_View noh_sv_chop_by_delim(Noh_String_View *sv, char delim);

// Trims the left part of a string view, until the provided function no longer holds on the current character.
void noh_sv_trim_left(Noh_String_View *sv, bool (*do_trim)(char));

// Trims the right part of a string view, until the provided function no longer holds on the current character.
void noh_sv_trim_right(Noh_String_View *sv, bool (*do_trim)(char));

// Trims both sides of a string view, until the provided function no longer holds on the current character.
void noh_sv_trim(Noh_String_View *sv, bool (*do_trim)(char));

// Trims spaces from the left part of a string view.
void noh_sv_trim_space_left(Noh_String_View *sv);

// Trims spaces from the right part of a string view.
void noh_sv_trim_space_right(Noh_String_View *sv);

// Trims spaces from both sides of a string view.
void noh_sv_space_trim(Noh_String_View *sv);

// Creates a string view from a c-string.
Noh_String_View noh_sv_from_cstr(const char *cstr);

// Checks whether to string views contain the same string.
bool noh_sv_eq(Noh_String_View a, Noh_String_View b);

// Checks whether to string views contain the same string, ignoring the case.
bool noh_sv_eq_ci(Noh_String_View a, Noh_String_View b);

// Checks whether the first string view starts with the elements from second string view.
bool noh_sv_starts_with(Noh_String_View a, Noh_String_View b);

// Checks whether the first string view starts with the elements from second string view, ignoring the case.
bool noh_sv_starts_with_ci(Noh_String_View a, Noh_String_View b);

// Checks whether the first string view contains the elements from the second string view.
bool noh_sv_contains(Noh_String_View a, Noh_String_View b);

// Checks whether the first string view contains the elements from the second string view, ignoring the case.
bool noh_sv_contains_ci(Noh_String_View a, Noh_String_View b);

// Creates a cstring in an arena from a string view.
const char *noh_sv_to_arena_cstr(Noh_Arena *arena, Noh_String_View sv);


// printf macros for Noh_String_View
#define Nsv_Fmt "%.*s"
#define Nsv_Arg(sv) (int) (sv).count, (sv).data
// USAGE:
//   Noh_String_View name = ...;
//   printf("Name: "Nsv_Fmt"\n", Nsv_Arg(name));

///////////////////////// Files and directories /////////////////////////

// File paths.
typedef struct {
    char **elems;
    size_t count;
    size_t capacity;
} Noh_File_Paths;

// Creates the path at the specified directory if it does not exist.
// Does not create any missing parent directories.
bool noh_mkdir_if_needed(const char *path);

// Renames a file.
bool noh_rename(const char *path, const char *new_path);

// Removes a file.
bool noh_remove(const char *path);

#endif // NOH_H_

#ifdef NOH_IMPLEMENTATION

///////////////////////// Core stuff /////////////////////////  

void* noh_realloc_check_(void *target, size_t size) {
    target = realloc(target, size);
    noh_assert(target != NULL && "Could not allocate enough memory");
    return target;
}

char *noh_shift_args(int *argc, char ***argv) {
    noh_assert(*argc > 0 && "No more arguments");

    char *result = **argv;
    (*argv)++;
    (*argc)--;

    return result;
}

///////////////////////// Time /////////////////////////  

long noh_diff_timespec_ms(const struct timespec *time1, const struct timespec *time2) {
    noh_assert(time1);
    noh_assert(time2);

    long res = 0;
    // Every second adds 1000 milliseconds difference.
    res += (time1->tv_sec - time2->tv_sec) * 1000;
    // Every 1000 * 1000 nanoseconds add 1 millisecond difference.
    res += (time1->tv_nsec - time2->tv_nsec) / 1000 / 1000;

    return res;
}

struct timespec noh_get_time_in(int seconds, long milliseconds) {
    struct timespec time;
    if (clock_gettime(CLOCK_REALTIME, &time) == -1)
    {
        noh_log(NOH_ERROR, "Unable to get the current time: %s", strerror(errno));
        exit(1);
    }

    noh_time_add(&time, seconds, milliseconds);
    return time;
}

void noh_time_add(struct timespec *time, int seconds, long milliseconds) {
    static long ns_per_ms = 1000 * 1000;
    time->tv_sec += seconds;
    time->tv_nsec += milliseconds * ns_per_ms;

    // Fix any overflow.
    if (time->tv_nsec >= 1000 * ns_per_ms) {
        time->tv_sec += time->tv_nsec / (1000 * ns_per_ms);
        time->tv_nsec %= 1000 * ns_per_ms;
    }
}

///////////////////////// Logging /////////////////////////  

void noh_log(Noh_Log_Level level, const char *fmt, ...)
{
    switch (level) {
        case NOH_INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case NOH_WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case NOH_ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        default:
            noh_assert(false && "Invalid log level");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

///////////////////////// Arena /////////////////////////  

// Alin a size such that it is a multiple of 8, keeping blocks of 64 bits.
size_t align_size(size_t size) {
    return size + (size % 8);
}

Noh_Arena noh_arena_init(size_t size) {
    Noh_Arena arena = {0};

    Noh_Arena_Data_Blocks blocks = {0};
    arena.blocks = blocks;

    Noh_Arena_Checkpoints checkpoints = {0};
    arena.checkpoints = checkpoints;
    arena.active_block = 0;

    Noh_Arena_Data_Block block = {0};
    block.capacity = align_size(size);
    block.data = noh_realloc_check(block.data, block.capacity);
    block.size = 0;
    noh_da_append(&arena.blocks, block);

    // Nice to have a checkpoint at the start.
    noh_arena_save(&arena);

    return arena;
}

void noh_arena_reset(Noh_Arena *arena) {
    // We need to load a block and save it in the checkpoint, so at least one block needs to be allocated.
    noh_assert(arena->blocks.count > 0 && "Please ensure that the arena is inintialized.");

    // Reset checkpoints.
    noh_da_reset(&arena->checkpoints);

    // Insert a checkpoint at the start so we can rewind to this checkpoint.
    Noh_Arena_Checkpoint start_checkpoint = {0};
    start_checkpoint.block_id = 0;
    start_checkpoint.offset_in_block = 0;
    noh_da_append(&arena->checkpoints, start_checkpoint);

    // Rewind to the checkpoint, and place the checkpoint back in such that there is again a checkpoint at the start.
    noh_arena_rewind(arena);
    arena->checkpoints.count += 1;
}

void noh_arena_free(Noh_Arena *arena) {
    // Remove checkpoints.
    noh_da_free(&arena->checkpoints);

    // Free all blocks.
    for (size_t i = 0; i < arena->blocks.count; i++) {
        Noh_Arena_Data_Block *block = &arena->blocks.elems[i];
        free(block->data);
    }

    // Remove blocks.
    noh_da_free(&arena->blocks);

    arena->active_block = 0;
}

void *noh_arena_alloc(Noh_Arena *arena, size_t size) {
    // This is technically not needed, but it is nice to be consistent and ensure that there is always a checkpoint
    // at the beginning, either from noh_arena_init, noh_arena_reset or noh_arena_save.
    noh_assert(arena->checkpoints.count > 0 && "Please ensure that there is at least one checkpoint before allocating.");

    // Reserve will ensure that we have the required space available. Then we just need to find the block where we can
    // allocate the requested space.
    noh_arena_reserve(arena, size);

    // Find the block that fits the requested size.
    size_t current_block = arena->active_block;
    Noh_Arena_Data_Block *block = &arena->blocks.elems[current_block];
    while (block->capacity - block->size < size && current_block < arena->blocks.count) {
        current_block += 1;
        block = &arena->blocks.elems[current_block];
    }

    noh_assert(block->capacity - block->size >= size && "Reserve should have provided a large enough block.");

    arena->active_block = current_block;

    // Allocate data in the block and return a pointer to the start.
    void *result = &block->data[block->size];
    block->size += size;
    return result;
}

void noh_arena_reserve(Noh_Arena *arena, size_t size) {
    noh_assert(arena->blocks.count > 0 && "Please ensure that the arena is initialized.");

    size_t requested_size = align_size(size);
    
    while (arena->active_block < arena->blocks.count) {
        Noh_Arena_Data_Block *block = &arena->blocks.elems[arena->active_block];
        // If the requested size fits into the current block, use it and return.
        if (block->capacity - block->size >= requested_size) {
            return;
        }

        // If it doesn't, free the block if it was empty. Note that all but the current block will be empty, since
        // rewinding sets the sizes of later blocks to 0. Current block may be empty.
        if (block->size == 0) {
            free(block->data);

            // This reduces arena->blocks.count, thus ensuring termination of the loop.
            noh_da_remove_at(&arena->blocks, arena->active_block);
        } else {
            // If we're not cleaning this block up, move the pointer.
            arena->active_block += 1;
        }
    }

    // If no block was found that fits, create a new one that is at least as big as the requested size, and twice the
    // size of the current block.
    // arena->active_block will now point to just beyond the last existing block. We can get the previous capacity
    // only if we didn't just delete the first block.
    size_t prev_cap = 0;
    if (arena->active_block > 1) prev_cap = arena->blocks.elems[arena->active_block - 1].capacity;

    size_t new_cap = NOH_ARENA_INIT_CAP;
    // If not big enough to double the previous cap, set to double the previous capacity.
    if (prev_cap * 2 > new_cap) new_cap = prev_cap * 2;
    // Keep doubling until the requested size fits.
    while (requested_size > new_cap) new_cap *= 2;

    Noh_Arena_Data_Block new_block = {0};
    new_block.data = noh_realloc_check(new_block.data, new_cap);
    new_block.capacity = new_cap;
    new_block.size = 0;

    // After adding this block, arena->active_block will point to this new block.
    noh_da_append(&(arena->blocks), new_block);
}

void noh_arena_save(Noh_Arena *arena) {
    // We need to load a block and save it in the checkpoint, so at least one block needs to be allocated.
    noh_assert(arena->blocks.count > 0 && "Please ensure that the arena is inintialized.");

    Noh_Arena_Checkpoint checkpoint = {0};
    checkpoint.block_id = arena->active_block;

    Noh_Arena_Data_Block *block = &arena->blocks.elems[arena->active_block];
    checkpoint.offset_in_block = block->size;

    noh_da_append(&(arena->checkpoints), checkpoint);
}

void noh_arena_rewind(Noh_Arena *arena) {
    noh_assert(arena->checkpoints.count > 0 && "No history to rewind");

    // Restore to block from checkpoint.
    Noh_Arena_Checkpoint *checkpoint = &arena->checkpoints.elems[arena->checkpoints.count - 1];
    arena->active_block = checkpoint->block_id;

    // Rewind all blocks from the active block to the end.
    for (size_t i = arena->active_block; i < arena->blocks.count; i++) {
        Noh_Arena_Data_Block *block = &arena->blocks.elems[i];
        if (i == arena->active_block) block->size = checkpoint->offset_in_block;
        else block->size = 0;

    }

    // Remove checkpoint.
    arena->checkpoints.count -= 1;
}

char *noh_arena_strdup(Noh_Arena *arena, const char *cstr) {
    size_t len = strlen(cstr);
    char *result = noh_arena_alloc(arena, len + 1);
    memcpy(result, cstr, len);
    result[len] = '\0';
    return result;
}

char *noh_arena_sprintf(Noh_Arena *arena, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);

    noh_assert(n >= 0);
    char *result = noh_arena_alloc(arena, n + 1);
    va_start(args, format);
    vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

///////////////////////// Strings /////////////////////////

void noh_string_append_cstr(Noh_String *string, const char *cstr) {
    size_t len = strlen(cstr);
    noh_da_append_multiple(string, cstr, len);
}

void noh_string_append_null(Noh_String *string) {
    noh_da_append(string, '\0');
}

bool noh_string_read_file(Noh_String *string, const char *filename) {
    bool result = true;
    size_t buf_size = 32*1024;
    char *buf = NULL;
    buf = noh_realloc_check(buf, buf_size);

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        noh_log(NOH_ERROR, "Could not open file %s: %s.\n", filename, strerror(errno));
        noh_return_defer(false);
    }

    size_t n = fread(buf, 1, buf_size, f);
    while (n > 0) {
        noh_da_append_multiple(string, buf, n);
        n = fread(buf, 1, buf_size, f);
    }

    if (ferror(f)) {
        noh_log(NOH_ERROR, "Could not read file %s: %s.\n", filename, strerror(errno));
        noh_return_defer(false);
    }

defer:
    free(buf);
    if (f) fclose(f);
    return result;
}

///////////////////////// String view /////////////////////////  

static void increase_sv_position(Noh_String_View *sv, size_t distance) {
    if (distance < sv->count) {
        sv->count -= distance;
        sv->elems += distance;
    } else {
        sv->count = 0;
        sv->elems += sv->count;
    }
}

Noh_String_View noh_sv_chop_by_delim(Noh_String_View *sv, char delim) {
    size_t i = 0;
    // Find the character, or the end of the string view.
    while (i < sv->count && sv->elems[i] != delim) i++;

    // The data until the delimiter is returned.
    Noh_String_View result = { .count = i, .elems = sv->elems };

    // Update the current string view beyond the delimiter.
    increase_sv_position(sv, i + 1);

    return result;
}

void noh_sv_trim_left(Noh_String_View *sv, bool (*do_trim)(char)) {
    size_t i = 0;
    while (i < sv->count && (*do_trim)(sv->elems[i])) i++;
    increase_sv_position(sv, i);
}

void noh_sv_trim_right(Noh_String_View *sv, bool (*do_trim)(char)) {
    size_t i = sv->count;
    while (i > 0 && (*do_trim)(sv->elems[i-1])) i--;
    sv->count = i;
}

void noh_sv_trim(Noh_String_View *sv, bool (*do_trim)(char)) {
    noh_sv_trim_left(sv, do_trim);
    noh_sv_trim_right(sv, do_trim);
}

bool is_space(char c) {
    return isspace(c) > 0;
}

void noh_sv_trim_space_left(Noh_String_View *sv) {
    noh_sv_trim_left(sv, &is_space);
}

void noh_sv_trim_space_right(Noh_String_View *sv) {
    noh_sv_trim_right(sv, &is_space);
}

void noh_sv_trim_space(Noh_String_View *sv) {
    noh_sv_trim(sv, &is_space);
}

Noh_String_View noh_sv_from_cstr(const char *cstr) {
    Noh_String_View result = {0};
    result.elems = cstr;
    result.count = strlen(cstr);
    return result;
}

bool noh_sv_eq(Noh_String_View a, Noh_String_View b) {
    if (a.count != b.count) return false;

    for (size_t i = 0; i < a.count; i++) {
        if (a.elems[i] != b.elems[i]) return false;
    }

    return true;
}

// Check two characters case insensitively.
bool char_eq_ci(char a, char b) {
    // FUTURE: Unicode support?
    // A=65, Z=90 - a=97, z=122
    if (a >= 65 && a <= 90) a +=32;
    if (b >= 65 && b <= 90) b +=32;

    return a == b;
}

bool noh_sv_eq_ci(Noh_String_View a, Noh_String_View b) {
    if (a.count != b.count) return false;

    for (size_t i = 0; i < a.count; i++) {
        if (!char_eq_ci(a.elems[i], b.elems[i])) return false;
    }

    return true;
}

bool noh_sv_starts_with(Noh_String_View a, Noh_String_View b) {
    if (a.count < b.count) return false;
    a.count = b.count;
    return noh_sv_eq(a, b);
}

bool noh_sv_starts_with_ci(Noh_String_View a, Noh_String_View b) {
    if (a.count < b.count) return false;
    a.count = b.count;
    return noh_sv_eq_ci(a, b);
}

bool noh_sv_contains(Noh_String_View a, Noh_String_View b) {
    while (a.count >= b.count) {
        if (noh_sv_starts_with(a, b)) return true;
        increase_sv_position(&a, 1);
    }

    return false;
}

bool noh_sv_contains_ci(Noh_String_View a, Noh_String_View b) {
    while (a.count >= b.count) {
        if (noh_sv_starts_with_ci(a, b)) return true;
        increase_sv_position(&a, 1);
    }

    return false;
}

const char *noh_sv_to_arena_cstr(Noh_Arena *arena, Noh_String_View sv)
{
    char *result = noh_arena_alloc(arena, sv.count + 1);
    memcpy(result, sv.elems, sv.count);
    result[sv.count] = '\0';
    return result;
}

///////////////////////// Files and directories /////////////////////////

bool noh_mkdir_if_needed(const char *path) {
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif // _WIN32

    if (result == 0) {
        noh_log(NOH_INFO, "Created directory '%s'.", path);
        return true;
    }

    if (errno == EEXIST) {
        noh_log(NOH_INFO, "Directory '%s' already exists.", path);
        return true;
    }

    noh_log(NOH_ERROR, "Could not create directory '%s': %s", path, strerror(errno));
    return false;
}

bool noh_rename(const char *path, const char *new_path)
{
    noh_log(NOH_INFO, "Renaming '%s' to '%s'.", path, new_path);
    if (rename(path, new_path) < 0) {
        noh_log(NOH_ERROR, "Rename failed: %s", strerror(errno));
        return false;
    }

    return true;
}

bool noh_remove(const char *path) {
    noh_log(NOH_INFO, "Removing '%s'.", path);
    if (remove(path) < 0) { 
        noh_log(NOH_ERROR, "Remove failed: %s", strerror(errno));
        return false;
    }

    return true;
}

#endif // NOH_IMPLEMENTATION
