/* Maintenance messages: reboot and firmware update (identity: node_ident.c). */
#include "node.h"

void wye_node_schedule(struct wye_node *n, uint8_t kind, uint8_t mode)
{
    n->pending = kind;
    n->pending_mode = mode;
    n->pending_at = n->hooks->now_ms(n->hooks->ctx) + WYE_NODE_ACT_DELAY_MS;
}

static void fw_ack(struct wye_node *n, uint8_t seq, uint8_t status, uint32_t next)
{
    uint8_t p[5];
    p[0] = status;
    wye_put_u32(&p[1], next);
    wye_node_send(n, WYE_MSG_FW_ACK, seq, p, sizeof(p));
}

static void handle_fw(struct wye_node *n, const struct wye_packet *pkt)
{
    const struct wye_flash_ops *ops = n->hooks->flash;
    uint32_t next = n->fw.next;
    uint8_t st = WYE_FW_ERR_STATE;
    if (pkt->type == WYE_MSG_FW_BEGIN && pkt->len >= 8) {
        st = wye_fwupdate_begin(&n->fw, ops, wye_get_u32(pkt->payload), wye_get_u32(pkt->payload + 4));
        next = 0;
    } else if (pkt->type == WYE_MSG_FW_DATA && pkt->len > 4 && ops) {
        st = wye_fwupdate_data(&n->fw, ops, wye_get_u32(pkt->payload), pkt->payload + 4,
                               (size_t)pkt->len - 4u, &next);
    } else if (pkt->type == WYE_MSG_FW_COMMIT && ops) {
        st = wye_fwupdate_commit(&n->fw, ops);
        if (st == WYE_FW_OK) {
            wye_node_schedule(n, WYE_PENDING_INSTALL, 0);
        }
    }
    fw_ack(n, pkt->seq, st, next);
}

bool wye_node_handle_maint(struct wye_node *n, const struct wye_packet *pkt)
{
    switch (pkt->type) {
    case WYE_MSG_REBOOT:
        wye_node_schedule(n, WYE_PENDING_REBOOT, pkt->len ? pkt->payload[0] : WYE_REBOOT_NORMAL);
        wye_node_send(n, WYE_MSG_REBOOT, pkt->seq, NULL, 0);
        return true;
    case WYE_MSG_FW_BEGIN:
    case WYE_MSG_FW_DATA:
    case WYE_MSG_FW_COMMIT:
        handle_fw(n, pkt);
        return true;
    default:
        return wye_node_handle_ident(n, pkt);
    }
}
