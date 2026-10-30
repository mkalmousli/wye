/* Identity messages: USB identity (both nodes) and function settings (target). */
#include "node.h"

static void send_identity(struct wye_node *n, uint8_t seq)
{
    struct wye_identity id;
    uint8_t buf[4 + 3 * WYE_ID_STR_MAX];
    if (!n->hooks->identity_get || !n->hooks->identity_get(n->hooks->ctx, &id)) {
        wye_node_error(n, seq, WYE_ERR_UNKNOWN_TYPE, WYE_MSG_IDENTITY_GET);
        return;
    }
    wye_node_send(n, WYE_MSG_IDENTITY, seq, buf, wye_identity_pack(&id, buf));
}

static void set_identity(struct wye_node *n, const struct wye_packet *pkt)
{
    struct wye_identity id;
    if (!wye_identity_unpack(pkt->payload, pkt->len, &id)) {
        wye_node_error(n, pkt->seq, WYE_ERR_BAD_LENGTH, pkt->type);
    } else if (!n->hooks->identity_set || !n->hooks->identity_set(n->hooks->ctx, &id)) {
        wye_node_error(n, pkt->seq, WYE_ERR_BUSY, pkt->type);
    } else {
        send_identity(n, pkt->seq);
        wye_node_schedule(n, WYE_PENDING_REBOOT, WYE_REBOOT_NORMAL); /* re-enumerate */
    }
}

static void send_functions(struct wye_node *n, uint8_t seq)
{
    struct wye_functions f;
    uint8_t buf[1 + 2 * WYE_FN_NAME_MAX];
    if (!n->hooks->functions_get || !n->hooks->functions_get(n->hooks->ctx, &f)) {
        wye_node_error(n, seq, WYE_ERR_UNKNOWN_TYPE, WYE_MSG_FUNCS_GET);
        return;
    }
    wye_node_send(n, WYE_MSG_FUNCS, seq, buf, wye_functions_pack(&f, buf));
}

static void set_functions(struct wye_node *n, const struct wye_packet *pkt)
{
    struct wye_functions f;
    if (!wye_functions_unpack(pkt->payload, pkt->len, &f)) {
        wye_node_error(n, pkt->seq, WYE_ERR_BAD_LENGTH, pkt->type);
    } else if (!n->hooks->functions_set || !n->hooks->functions_set(n->hooks->ctx, &f)) {
        wye_node_error(n, pkt->seq, WYE_ERR_BUSY, pkt->type);
    } else {
        send_functions(n, pkt->seq);
        wye_node_schedule(n, WYE_PENDING_REBOOT, WYE_REBOOT_NORMAL); /* re-enumerate with the new parts */
    }
}

bool wye_node_handle_ident(struct wye_node *n, const struct wye_packet *pkt)
{
    switch (pkt->type) {
    case WYE_MSG_IDENTITY_GET: send_identity(n, pkt->seq); return true;
    case WYE_MSG_IDENTITY_SET: set_identity(n, pkt); return true;
    case WYE_MSG_FUNCS_GET: send_functions(n, pkt->seq); return true;
    case WYE_MSG_FUNCS_SET: set_functions(n, pkt); return true;
    default: return false;
    }
}
