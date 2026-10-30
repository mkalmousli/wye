/* A Pico's name, colour and notes: the payload, cleaning, and storing them over the link. */
#include "sim.h"
#include "test.h"

#include <stdio.h>

static struct sim_device g_sim;

TEST(profile_codec)
{
    struct wye_profile f = {3, "K\xc3\xbc" "che Desk", "under the left monitor"}, back;
    uint8_t buf[WYE_PROFILE_PACKED_MAX];
    size_t n = wye_profile_pack(&f, buf);
    CHECK(wye_profile_unpack(buf, n, &back));
    CHECK_EQ(back.color, 3);
    CHECK(strcmp(back.name, f.name) == 0 && strcmp(back.notes, f.notes) == 0);
    CHECK(!wye_profile_unpack(buf, n - 1, &back)); /* truncated */
    CHECK(!wye_profile_unpack(buf, 0, &back));
    buf[0] = 200;
    CHECK(wye_profile_unpack(buf, n, &back));
    CHECK_EQ(back.color, 0); /* an unknown colour means the usual one */
    buf[1] = WYE_PROFILE_NAME_MAX + 1;
    CHECK(!wye_profile_unpack(buf, n, &back)); /* longer than a name can be */

    memset(&f, 'x', sizeof(f)); /* the longest there can be fits the buffer */
    f.color = 1;
    f.name[WYE_PROFILE_NAME_MAX] = f.notes[WYE_PROFILE_NOTES_MAX] = '\0';
    CHECK_EQ(wye_profile_pack(&f, buf), WYE_PROFILE_PACKED_MAX);
    CHECK(WYE_PROFILE_PACKED_MAX <= WYE_MAX_PAYLOAD);
}

TEST(profile_text_cleaning)
{
    char s[128];
    snprintf(s, sizeof(s), "  a\tb\nc  ");
    wye_profile_clean(s, WYE_PROFILE_NAME_MAX);
    CHECK(strcmp(s, "a b c") == 0);
    snprintf(s, sizeof(s), "12345678901234567890123\xc3\xa4z"); /* the second byte would pass the limit */
    wye_profile_clean(s, WYE_PROFILE_NAME_MAX);
    CHECK(strcmp(s, "12345678901234567890123") == 0);
    snprintf(s, sizeof(s), "12345678901234567890123 zz");
    wye_profile_clean(s, WYE_PROFILE_NAME_MAX);
    CHECK(strcmp(s, "12345678901234567890123") == 0);
    snprintf(s, sizeof(s), "   ");
    wye_profile_clean(s, WYE_PROFILE_NAME_MAX);
    CHECK_EQ(s[0], '\0');
}

TEST(profile_is_stored_on_the_board_without_a_restart)
{
    struct sim_device *s = &g_sim;
    struct wye_profile f = {4, "Rack", "second shelf"};
    uint8_t buf[WYE_PROFILE_PACKED_MAX];
    sim_init(s);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_PROFILE_SET, buf, wye_profile_pack(&f, buf));
    CHECK(strcmp(s->profile.name, "Rack") == 0 && s->profile.color == 4);
    CHECK(strcmp(s->profile.notes, "second shelf") == 0);
    sim_advance(s, WYE_NODE_ACT_DELAY_MS + 1);
    CHECK_EQ(s->reboots, 0);
}
