#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./editor.h"

static int failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures += 1; \
        } \
    } while (0)

static void write_file(const char *path, const char *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "could not create %s\n", path);
        exit(1);
    }
    fwrite(data, 1, size, f);
    fclose(f);
}

static bool file_equals(const char *path, const char *data, size_t size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return false;
    }
    char buf[4096];
    const size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    return n == size && memcmp(buf, data, size) == 0;
}

static bool editor_equals(const Editor *editor, const char *const *lines,
                           size_t count, bool trailing_newline)
{
    if (editor->size != count) {
        return false;
    }
    if (editor->trailing_newline != trailing_newline) {
        return false;
    }
    for (size_t row = 0; row < count; ++row) {
        if (editor->lines[row].size != strlen(lines[row])) {
            return false;
        }
        if (memcmp(editor->lines[row].chars, lines[row], editor->lines[row].size) != 0) {
            return false;
        }
    }
    return true;
}

static bool cursor_valid(const Editor *editor)
{
    if (editor->size == 0) {
        return editor->cursor_row == 0 && editor->cursor_col == 0;
    }
    return editor->cursor_row < editor->size &&
           editor->cursor_col <= editor->lines[editor->cursor_row].size;
}

int main(void)
{
    const char *path = "test_editor_tmp.txt";
    Editor editor = {0};

    write_file(path, "", 0);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, NULL, 0, false));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "", 0));

    const char *one[] = {"hello"};
    write_file(path, "hello", 5);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, one, 1, false));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "hello", 5));

    const char *many[] = {"a", "b", "c"};
    write_file(path, "a\nb\nc", 5);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, many, 3, false));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "a\nb\nc", 5));

    const char *blanks[] = {"a", "", "", "b"};
    write_file(path, "a\n\n\nb", 5);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, blanks, 4, false));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "a\n\n\nb", 5));

    const char *with_newline[] = {"a", "b"};
    write_file(path, "a\nb\n", 4);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, with_newline, 2, true));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "a\nb\n", 4));

    const char *without_newline[] = {"a", "b"};
    write_file(path, "a\nb", 3);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, without_newline, 2, false));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "a\nb", 3));

    const char *trailing_blank[] = {"a", ""};
    write_file(path, "a\n\n", 3);
    CHECK(editor_load_from_file(&editor, path));
    CHECK(editor_equals(&editor, trailing_blank, 2, true));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "a\n\n", 3));

    CHECK(!editor_load_from_file(&editor, "/nonexistent-dir-xyz/file.txt"));

    const char *edited[] = {"hello!", "world", "end"};
    write_file(path, "hello\nworld\n", 12);
    CHECK(editor_load_from_file(&editor, path));
    editor.cursor_row = 0;
    editor.cursor_col = 5;
    editor_insert_text_before_cursor(&editor, "!");
    editor.cursor_row = 1;
    editor.cursor_col = 5;
    editor_insert_new_line(&editor);
    editor_insert_text_before_cursor(&editor, "end");
    CHECK(editor_equals(&editor, edited, 3, false));
    CHECK(cursor_valid(&editor));
    editor_save_to_file(&editor, path);
    CHECK(file_equals(path, "hello!\nworld\nend", 16));

    remove(path);

    if (failures > 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
