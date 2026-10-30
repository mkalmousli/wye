/* Pico names: found by board, kept in the file, cleaned, and limited. */
#include "config/pico_names.h"
#include "test.h"

#include <stdio.h>

static const uint8_t A[8] = {1, 2, 3, 4, 5, 6, 7, 8}, B[8] = {9, 9, 9, 9, 9, 9, 9, 9}, NONE[8];

TEST(pico_names_follow_the_board)
{
    struct wye_pico_names n = {0};
    CHECK(wye_pico_names_find(&n, A) == NULL);
    CHECK(wye_pico_names_set(&n, A, "Desk", 3));
    CHECK(wye_pico_names_set(&n, B, "Rack", 0));
    CHECK(strcmp(wye_pico_names_find(&n, A)->name, "Desk") == 0);
    CHECK_EQ(wye_pico_names_find(&n, A)->color, 3);
    CHECK(strcmp(wye_pico_names_find(&n, B)->name, "Rack") == 0);
    CHECK(wye_pico_names_set(&n, A, "Desk 2", 1)); /* renaming replaces */
    CHECK_EQ(n.count, 2);
    CHECK(strcmp(wye_pico_names_find(&n, A)->name, "Desk 2") == 0);
    CHECK(wye_pico_names_set(&n, A, "", 4));       /* a colour alone is kept */
    CHECK(wye_pico_names_find(&n, A) && !wye_pico_names_find(&n, A)->name[0]);
    CHECK(wye_pico_names_set(&n, A, "  ", 0));     /* nothing left: forgotten */
    CHECK(wye_pico_names_find(&n, A) == NULL);
    CHECK_EQ(n.count, 1);
    CHECK(!wye_pico_names_set(&n, NONE, "x", 0));  /* a board with no ID cannot be told apart */
    CHECK(wye_pico_names_find(&n, NONE) == NULL);
    CHECK(wye_pico_names_set(&n, A, "x", 99));     /* an unknown colour means the usual one */
    CHECK_EQ(wye_pico_names_find(&n, A)->color, 0);
}

TEST(pico_name_cleaning)
{
    char s[128];
    snprintf(s, sizeof(s), "  a\tb\nc  ");
    wye_pico_name_clean(s);
    CHECK(strcmp(s, "a b c") == 0);
    snprintf(s, sizeof(s), "12345678901234567890123\xc3\xa4z"); /* 'ä' would straddle the limit */
    wye_pico_name_clean(s);
    CHECK(strcmp(s, "12345678901234567890123") == 0);
    snprintf(s, sizeof(s), "12345678901234567890123 zz");
    wye_pico_name_clean(s);
    CHECK(strcmp(s, "12345678901234567890123") == 0);
}

TEST(pico_names_survive_a_restart)
{
    struct wye_pico_names n = {0}, back;
    wye_pico_names_set(&n, A, "K\xc3\xbc" "che 2", 5);
    wye_pico_names_set(&n, B, "", 2);
    CHECK(wye_pico_names_save(&n));
    wye_pico_names_load(&back);
    CHECK_EQ(back.count, 2);
    CHECK(strcmp(wye_pico_names_find(&back, A)->name, "K\xc3\xbc" "che 2") == 0);
    CHECK_EQ(wye_pico_names_find(&back, A)->color, 5);
    CHECK_EQ(wye_pico_names_find(&back, B)->color, 2);
}

TEST(pico_names_forget_the_oldest_when_full)
{
    struct wye_pico_names n = {0};
    uint8_t uid[8] = {1};
    for (int i = 0; i < WYE_PICO_NAMES + 3; ++i) {
        uid[7] = (uint8_t)i;
        CHECK(wye_pico_names_set(&n, uid, "board", 0));
    }
    CHECK_EQ(n.count, WYE_PICO_NAMES);
    uid[7] = 0;
    CHECK(wye_pico_names_find(&n, uid) == NULL);
    uid[7] = WYE_PICO_NAMES + 2;
    CHECK(wye_pico_names_find(&n, uid) != NULL);
}
