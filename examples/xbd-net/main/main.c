#include <stdio.h>
#include <msg.h>

#include "minerva_border_router.h"
#include "minerva_xbd.h"
#include "rustmod.h"

//-------- -------- ^^ TODO refactor into 'modules/minerva_gcoap'
#include "fs/constfs.h"

//--#include <stdio.h>
//--#include "kernel_defines.h"
#include "net/gcoap.h"
#include "net/gcoap/fileserver.h"
//--#include "shell.h"
#include "vfs_default.h"

/* CoAP resources. Must be sorted by path (ASCII order). */
#if 0//====
static const coap_resource_t _resources[] = {
    { "/vfs",
      COAP_GET |
#if IS_USED(MODULE_GCOAP_FILESERVER_PUT)
      COAP_PUT |
#endif
#if IS_USED(MODULE_GCOAP_FILESERVER_DELETE)
      COAP_DELETE |
#endif
      COAP_MATCH_SUBTREE,
      gcoap_fileserver_handler, VFS_DEFAULT_DATA },
};
#else//==== 'tests/gcoap_fileserver' code
extern ssize_t xbd_riot_fileserver_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static const coap_resource_t _resources[] = {
    {
        .path = "/const",
        .methods = COAP_GET | COAP_MATCH_SUBTREE,
        .handler = xbd_riot_fileserver_handler,
        .context = "/const"
    },
    {
        .path = "/vfs",
        .methods = COAP_GET | COAP_PUT | COAP_MATCH_SUBTREE,
        .handler = xbd_riot_fileserver_handler,
        .context = VFS_DEFAULT_DATA
    },
};
#endif//====


static gcoap_listener_t _listener = {
    .resources = _resources,
    .resources_len = ARRAY_SIZE(_resources),
};

//--------^^ 'tests/gcoap_fileserver' code
static const char song[] =
    "Join us now and share the software;\n"
    "You'll be free, hackers, you'll be free.\n"
    "Join us now and share the software;\n"
    "You'll be free, hackers, you'll be free.\n"
    "\n"
    "Hoarders can get piles of money,\n"
    "That is true, hackers, that is true.\n"
    "But they cannot help their neighbors;\n"
    "That's not good, hackers, that's not good.\n"
    "\n"
    "When we have enough free software\n"
    "At our call, hackers, at our call,\n"
    "We'll kick out those dirty licenses\n"
    "Ever more, hackers, ever more.\n"
    "\n"
    "Join us now and share the software;\n"
    "You'll be free, hackers, you'll be free.\n"
    "Join us now and share the software;\n"
    "You'll be free, hackers, you'll be free.\n";

/* this defines two const files in the constfs */
static constfs_file_t constfs_files[] = {
    {
        .path = "/song.txt",
        .size = sizeof(song),
        .data = song,
    },
};

/* this is the constfs specific descriptor */
static constfs_t constfs_desc = {
    .nfiles = ARRAY_SIZE(constfs_files),
    .files = constfs_files,
};

/* constfs mount point, as for previous example, it needs a file system driver,
 * a mount point and private_data as a pointer to the constfs descriptor */
static vfs_mount_t const_mount = {
    .fs = &constfs_file_system,
    .mount_point = "/const",
    .private_data = &constfs_desc,
};
//--------$$ 'tests/gcoap_fileserver' code

static void _event_cb(gcoap_fileserver_event_t event, gcoap_fileserver_event_ctx_t *ctx)
{
    switch (event) {
    case GCOAP_FILESERVER_GET_FILE_START:
        printf("gcoap fileserver: Download started: %s\n", ctx->path);
        break;
    case GCOAP_FILESERVER_GET_FILE_END:
        printf("gcoap fileserver: Download finished: %s\n", ctx->path);
        break;
    case GCOAP_FILESERVER_PUT_FILE_START:
        printf("gcoap fileserver: Upload started: %s\n", ctx->path);
        break;
    case GCOAP_FILESERVER_PUT_FILE_END:
        printf("gcoap fileserver: Upload finished: %s\n", ctx->path);
        break;
    case GCOAP_FILESERVER_DELETE_FILE:
        printf("gcoap fileserver: Delete %s\n", ctx->path);
        break;
    }
}

// NG **** instread
// !!!! ../../RIOT/sys/shell/cmds/vfs.c
//static int test_vfs_cmd() {
//    return _vfs_handler(0, NULL);
//}

void init_gcoap_fileserver(void) {
    vfs_mount(&const_mount); // <<
    /* todo - dump on start
> vfs df
vfs df
Mountpoint              Total         Used    Available     Use%
/nvm0                   8 MiB        8 KiB     8184 KiB       0%
/const                  599 B        599 B          0 B     100%  <<
> vfs ls /const
vfs ls /const
song.txt	599 B
total 1 files
> vfs ls /nvm0
vfs ls /nvm0
./
../
total 0 files
    */

    //--msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    gcoap_register_listener(&_listener);

    if (IS_USED(MODULE_GCOAP_FILESERVER_CALLBACK)) {
        gcoap_fileserver_set_event_cb(_event_cb, NULL);
    }

    //++char line_buf[SHELL_DEFAULT_BUFSIZE];
    //++shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
}

//-------- -------- $$ TODO refactor into 'modules/minerva_gcoap'

//

#if defined(MINERVA_DEBUG_ETH_MINIMAL)
#include "netdev_eth_minimal.h"
#define MINERVA_NETDEV_ETH_INIT minerva_netdev_eth_minimal_init
#elif defined(MINERVA_DEBUG_ETH_MANUAL)
#include "minerva_esp_eth.h"
#define MINERVA_NETDEV_ETH_INIT minerva_gnrc_esp_eth_init
#endif

#if defined(MINERVA_DEBUG_ETH_MINIMAL) || defined(MINERVA_DEBUG_ETH_MANUAL)
#include "esp_eth_netdev.h"
extern esp_eth_netdev_t _esp_eth_dev;
extern void esp_eth_setup(esp_eth_netdev_t* dev);
static int debug_esp32_eth_init(void) {
    esp_eth_setup(&_esp_eth_dev);
    return MINERVA_NETDEV_ETH_INIT(&_esp_eth_dev.netdev);
}
#endif

//

#ifdef MINERVA_BOARD_NATIVE
#define IP6_FIXTURE_SERVER "fe80::20be:cdff:fe0e:44a1" // IP6_FIXTURE_TAP1
#else
#define IP6_FIXTURE_SERVER "fe80::a00:27ff:fefd:b6f8" // IP6_FIXTURE_BR0
#endif

//

static const xbd_fn_t xbd_fns[] = {
    { "xbd_usleep", (xbd_fn_ptr_t)xbd_usleep },
    { "xbd_ztimer_msleep", (xbd_fn_ptr_t)xbd_ztimer_msleep },
    { "xbd_ztimer_set", (xbd_fn_ptr_t)xbd_ztimer_set },
    { "xbd_gcoap_req_send", (xbd_fn_ptr_t)xbd_gcoap_req_send },
};

static const size_t xbd_fns_sz = sizeof(xbd_fns) / sizeof(xbd_fns[0]);

//

static void goto_sync_shell(void) {
    xbd_shell_start(xbd_shell_get_commands());
}

//

static msg_t main_msg_queue[16];
static gnrc_netif_t *outer_interface = NULL;
static gnrc_netif_t *inner_interface = NULL;

int main(void) {
    puts("@@ [xbd-net] main(): ^^");

    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(main_msg_queue, sizeof(main_msg_queue) / sizeof(main_msg_queue[0]));

#if defined(MINERVA_DEBUG_ETH_MINIMAL) || defined(MINERVA_DEBUG_ETH_MANUAL)
    if (debug_esp32_eth_init()) {
        puts("Error initializing eth devices");
        return 1;
    }
#endif

    find_ifces(&outer_interface, &inner_interface);
    set_ips(outer_interface, inner_interface);

    if (0) {
        if (outer_interface) {
            puts("@@ main(): initializing CoAP server (hint: check with `> coap info`)");
            server_init();

            // hit the internal server
//            test_gcoap_req("get", "[::1]:5683", "/.well-known/core");
            test_gcoap_req("ping", "[::1]:5683", NULL);

            // hit the external 'libcoap-minimal/server'
//            test_gcoap_req("get", "[" IP6_FIXTURE_SERVER "]:5683", "/hello");
            test_gcoap_req("ping", "[" IP6_FIXTURE_SERVER "]:5683", NULL);
            /*
gcoap_cli: sending msg ID 45090, 4 bytes
gcoap: @@ _on_sock_udp_evt(): sock: 0x809dc20 type: 16 arg: (nil)
@@ xbd_on_sock_udp_evt(): sock: 0x809dc20 type: 16 arg: 0x0
gcoap: received RST, expiring potentially existing memo
coap: received timeout message
gcoap: timeout for msg ID 45090
gcoap: Ignoring empty non-CON request
gcoap: @@ after _process_coap_pdu() via _on_sock_udp_evt()
             */
        }
    }

    if (0) { // !! test with alias='nns'
        puts("@@ main(): initializing CoAP server (hint: check with `> coap info`)");
        server_init();

        //----
        test_gcoap_req("get", "[::1]:5684", "/.well-known/core"); // ok <-- coap: authentication timed out
        // cf. [ ] "another" client code with DTLS - 'libcoap/examples/riot/examples_libcoap_client/main.c'
        //----
/*

---- linux
$ libcoap/local/bin/coap-client -m get coaps://[fe80::10ef:d5ff:fe61:c7c%tap1]/.well-known/core -k "secretPSK" -u "Client_identity"
</cli/stats>;ct=0;rt="count";obs,</riot/board>  <==== !! ok
$ libcoap/local/bin/coap-client -m get coaps://[fe80::78ec:5fff:febd:add9%tap1]/.well-known/core -k "secretPSK" -u "Client_identity"
^CApr 11 15:45:10.803 ERR  cannot send CoAP pdu  <==== ?? FIXME why failing ??
$ libcoap/local/bin/coap-client -m get coap://[fe80::10ef:d5ff:fe61:c7c%tap1]/.well-known/core
</cli/stats>;ct=0;rt="count";obs,</riot/board>
$ libcoap/local/bin/coap-client -m get coap://[fe80::78ec:5fff:febd:add9%tap1]/.well-known/core
</cli/stats>;ct=0;rt="count";obs,</riot/board>

---- riot
$ ./crates/RIOT--base/examples/gcoap_dtls/bin/native/gcoap_example.elf tap1  # cf. vanilla

$ nns  # ok
> ifconfig
ifconfig
Iface  9  HWaddr: 12:EF:D5:61:0C:7C
          L2-PDU:1500  MTU:1500  HL:64  RTR
          Source address length: 6
          Link type: wired
          inet6 addr: fe80::10ef:d5ff:fe61:c7c  scope: link  VAL
          inet6 addr: fe80::78ec:5fff:febd:add9  scope: link  VAL
          inet6 group: ff02::2
          inet6 group: ff02::1
          inet6 group: ff02::1:ff61:c7c
          inet6 group: ff02::1:ffbd:add9

> unsupported tls extension: 0
unsupported tls extension: 35
gcoap: @@ after _process_coap_pdu() via _on_sock_dtls_evt()

>
 */
        if (1) {
            init_gcoap_fileserver();
            /* !! ok
$ libcoap/local/bin/coap-client -m get coaps://[fe80::10ef:d5ff:fe61:c7c%tap1]/const/song.txt -k "secretPSK" -u "Client_identity"
Join us now and share the software;
...
You'll be free, hackers, you'll be free.
             */
        }

        goto_sync_shell();
    }

    //

    init_gcoap_fileserver(); // !!!! to refactor

    if (0) { // !! test with alias='nn'
        //----
//        test_gcoap_req("get", "[::1]:5683", "/const/song.txt"); // ok
        //test_gcoap_req("get", "[::1]:5683", "/const/song2.txt"); // ok, 4.04
        //assert(0); // ok
        //----
        /* ok
$ libcoap/local/bin/coap-server
$ libcoap/local/bin/coap-server -k "secretPSK"  # N/A `-u`

> coapc
coapc <uri>
> coapc coap://[fe80::902f:8cff:fe74:41ae]/.well-known/core
...
> coapc coaps://[fe80::902f:8cff:fe74:41ae]/.well-known/core
...
         */
        //----
//        test_libcoap_req("get", "coap://[fe80::902f:8cff:fe74:41ae]/.well-known/core");
//        test_libcoap_req("get", "coaps://[fe80::902f:8cff:fe74:41ae]/.well-known/core");
//        test_libcoap_req("get", "coap://[::1]/cli/stats");
        test_libcoap_req("get", "coap://[::1]/const/song.txt");
        //----

        goto_sync_shell();
    }

    if (1) {
        rustmod_start(xbd_fns, xbd_fns_sz);
    } else {
        goto_sync_shell();
    }

    /* should be never reached */
    return 0;
}