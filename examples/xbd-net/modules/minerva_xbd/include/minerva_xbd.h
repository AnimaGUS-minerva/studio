/*
 * Copyright (C) 2023 ANIMA Minerva toolkit
 */

#ifndef MINERVA_XBD_H
#define MINERVA_XBD_H

#include <shell.h>
#include "minerva_gcoap.h"
#include "minerva_libcoap.h"

#ifdef __cplusplus
extern "C" {
#endif

void xbd_usleep(uint32_t delay);
void xbd_ztimer_msleep(uint32_t delay, bool debug);
void xbd_ztimer_set(uint32_t delay, void (*cb_handler)(void *), void *arg_ptr, void **timeout_pp);

const shell_command_t * xbd_shell_get_commands(void);
void xbd_shell_start(const shell_command_t *shell_commands);

void xbd_async_shell_on_char(char ch);
size_t xbd_async_shell_bufsize(void);
void xbd_async_shell_prompt(void);

void xbd_gcoap_req_send(char *addr, char *uri, uint8_t method, uint8_t *payload, size_t payload_len, bool blockwise, size_t blockwise_state_index, void *context, gcoap_resp_handler_t resp_handler);
uint8_t xbd_resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t* pdu, const sock_udp_ep_t *remote,
                         uint8_t **payload, size_t *payload_len, void **context);

ssize_t riot_board_handler_fill(
        coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx,
        const char *board);

#ifdef __cplusplus
}
#endif

#endif /* MINERVA_XBD_H */