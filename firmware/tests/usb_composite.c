/* Native USB integration regression: enumerate CDC + audio, then handshake
   before DTR is set. Run with tools/test_usb_composite.py <pico-sdk>. */
#include "tusb.h"
#include "device/dcd.h"
#include "usb_audio.h"
#include "wye_msg.h"
#include "usb_common.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main wye_bridge_firmware_main
#include "firmware/bridge/main.c"
#undef main
void board_reboot(uint8_t mode) {(void)mode;}
static int stalled, controls, cdc_replies;
static uint8_t reply[WYE_MAX_ENCODED];
static uint16_t reply_len;
static uint8_t control_out[8];
static uint8_t *cdc_out;
static uint16_t cdc_out_size;
uint32_t board_now_ms(void) { return 0; }
void usb_fill_device_desc(tusb_desc_device_t *d) { (void)d; }
const uint16_t *tud_descriptor_string_cb(uint8_t i,uint16_t lang) { (void)i; (void)lang;return NULL; }
bool dcd_init(uint8_t port,const tusb_rhport_init_t *init) { (void)port;(void)init;return true; }
void dcd_int_handler(uint8_t p) { (void)p; }
void dcd_int_enable(uint8_t p) { (void)p; }
void dcd_int_disable(uint8_t p) { (void)p; }
void dcd_set_address(uint8_t p,uint8_t addr) { (void)p;(void)addr; }
void dcd_remote_wakeup(uint8_t p) { (void)p; }
void dcd_sof_enable(uint8_t p,bool en) { (void)p;(void)en; }
bool dcd_edpt_open(uint8_t p,const tusb_desc_endpoint_t *e) { (void)p; (void)e;return true; }
void dcd_edpt_close_all(uint8_t p) { (void)p; }
void dcd_edpt_close(uint8_t p,uint8_t ep) { (void)p;(void)ep; }
void dcd_edpt_stall(uint8_t p,uint8_t ep) { (void)p;printf("STALL %02x\n",ep);stalled++; }
void dcd_edpt_clear_stall(uint8_t p,uint8_t ep) { (void)p;(void)ep; }
bool dcd_edpt_xfer(uint8_t p,uint8_t ep,uint8_t *buf,uint16_t len) {

  if((ep&15)==0) {
    if(ep==0 && len) memcpy(buf,control_out,len);
    controls++;
    dcd_event_xfer_complete(p,ep,len,XFER_RESULT_SUCCESS,false);
  } else if(ep==2) { cdc_out=buf;cdc_out_size=len; } else if(ep==0x82) { cdc_replies++; memcpy(reply,buf,len); reply_len=len; }
  return true;
}
static void request(uint8_t type,uint8_t req,uint16_t value,uint16_t index,uint16_t length) {
  tusb_control_request_t r={.bmRequestType=type,.bRequest=req,.wValue=value,.wIndex=index,.wLength=length};

  dcd_event_setup_received(0,(const uint8_t*)&r,false);tud_task();assert(!stalled);
}
int main(void) {
  wye_usb_audio_init(2,true,NULL);
  assert(tusb_init());
  dcd_event_bus_reset(0,TUSB_SPEED_FULL,false);tud_task();
  request(0,9,1,0,0);assert(tud_mounted());
  request(0xa1,1,0x0100,0x0402,4); /* clock frequency */
  request(0xa1,2,0x0100,0x0402,14); /* frequency range */
  request(1,11,0,3,0); /* stop playback */
  request(1,11,0,4,0); /* stop microphone */
  wye_put_u32(control_out,115200);control_out[4]=0;control_out[5]=0;control_out[6]=8;
  request(0x21,0x20,0,0,7); /* CDC line coding */
  assert(!tud_cdc_connected());
  struct wye_hello_ack hello={.proto_version=WYE_PROTO_VERSION,.role=WYE_ROLE_BRIDGE,.caps=WYE_CAP_AUDIO};
  const struct wye_node_hooks hooks={.now_ms=now_ms,.send_host=send_host};
  const struct wye_bridge_io io={0};
  wye_bridge_init(&g_bridge,&hello,&hooks,&io);
  uint8_t version=WYE_PROTO_VERSION, packet[WYE_MAX_ENCODED];
  size_t n=wye_packet_encode(WYE_NODE_BRIDGE,WYE_MSG_HELLO,9,&version,1,packet,sizeof(packet));
  assert(n<=cdc_out_size);
  memcpy(cdc_out,packet,n);
  dcd_event_xfer_complete(0,2,n,XFER_RESULT_SUCCESS,false);tud_task();
  n=tud_cdc_read(packet,sizeof(packet));
  wye_bridge_host_rx(&g_bridge,packet,n);
  assert(cdc_replies==1); /* Real bridge must not silently discard handshake replies. */
  struct wye_decoder decoder;struct wye_packet ack;wye_decoder_init(&decoder);
  enum wye_decode decoded=WYE_DECODE_NONE;
  for(unsigned i=0;i<reply_len;i++) decoded=wye_decoder_feed(&decoder,reply[i],&ack);
  assert(decoded==WYE_DECODE_PACKET && ack.type==WYE_MSG_HELLO_ACK && ack.seq==9);
  dcd_event_xfer_complete(0,0x82,reply_len,XFER_RESULT_SUCCESS,false);tud_task();
  request(0x21,0x22,3,0,0); /* DTR */
  assert(tud_cdc_connected());
  assert(cdc_out && cdc_out_size>=8);
  memcpy(cdc_out,"hello",5);dcd_event_xfer_complete(0,2,5,XFER_RESULT_SUCCESS,false);tud_task();
  assert(tud_cdc_available()==5);
  uint8_t buf[8];assert(tud_cdc_read(buf,8)==5);assert(!memcmp(buf,"hello",5));
  assert(tud_cdc_write("reply",5)==5);tud_cdc_write_flush();
  puts("USB audio + CDC control and data path OK");
}

uint32_t tusb_time_millis_api(void) { static uint32_t t; return t++; }
