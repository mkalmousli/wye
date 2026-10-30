/* Target settings (drive switch and name) over the link, and the payload codec. */
#include "sim.h"
#include "test.h"

static struct sim_device g_sim;

TEST(functions_round_trip_and_reboot)
{
    struct sim_device *s = &g_sim;
    struct wye_functions f = {WYE_FN_DRIVE, "ACME", "Team Share"}, back;
    uint8_t buf[1 + 2 * WYE_FN_NAME_MAX];
    size_t n = wye_functions_pack(&f, buf);
    CHECK(wye_functions_unpack(buf, n, &back));
    CHECK(strcmp(back.drive_product, "Team Share") == 0 && back.flags == WYE_FN_DRIVE);
    CHECK(!wye_functions_unpack(buf, 3, &back)); /* truncated */
    sim_init(s);
    f.flags = WYE_FN_AUDIO; /* drive off, audio on */
    n = wye_functions_pack(&f, buf);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_FUNCS_SET, buf, n);
    CHECK(strcmp(s->functions.drive_vendor, "ACME") == 0 && s->functions.flags == WYE_FN_AUDIO);
    CHECK_EQ(s->reboots, 0);
    sim_advance(s, WYE_NODE_ACT_DELAY_MS + 1);
    CHECK_EQ(s->reboots, 1); /* re-enumerates with the new parts */
}
