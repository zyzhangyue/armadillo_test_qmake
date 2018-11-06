#include "module_csirawdatafetcher.h"
#include "iwl_connector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>

static int sock_fd = -1;
/* Local variables */
static struct sockaddr_nl proc_addr, kern_addr;	// addrs for recv, send, bind
static struct cn_msg *cmsg;
static char buf[4096];

static void exit_program_err()
{
    if (sock_fd != -1)
    {
        close(sock_fd);
        sock_fd = -1;
    }
}

bool raw_csi_driver_init(void)
{
    /* Setup the socket */
    sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);

    if (sock_fd == -1) {
        fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
        return false;
    }

    /* Initialize the address structs */
    memset(&proc_addr, 0, sizeof(struct sockaddr_nl));
    proc_addr.nl_family = AF_NETLINK;
    proc_addr.nl_pid = getpid();			// this process' PID
    proc_addr.nl_groups = CN_IDX_IWLAGN;
    memset(&kern_addr, 0, sizeof(struct sockaddr_nl));
    kern_addr.nl_family = AF_NETLINK;
    kern_addr.nl_pid = 0;					// kernel
    kern_addr.nl_groups = CN_IDX_IWLAGN;

    /* Now bind the socket */
    if (bind(sock_fd, (struct sockaddr *)&proc_addr, sizeof(struct sockaddr_nl)) < 0) {
        fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
        exit_program_err();
        return false;
    }

    /* And subscribe to netlink group */
    {
        int on = proc_addr.nl_groups;
        if (setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &on, sizeof(on)) < 0) {
            fprintf(stderr, "Error setsocketopt: %s\n", strerror(errno));
            exit_program_err();
            return false;
        }
    }

    /* RT scheduler */
    struct sched_param param;
    param.sched_priority = 1;
    if (sched_setscheduler( getpid(), SCHED_FIFO, &param) < 0) {
        fprintf(stderr, "Error setting scheduler: %s\n", strerror(errno));
        exit_program_err();
        return false;
    }
    return true;
}

bool get_raw_csi_from_driver(void *buffer)
{
    if (recv(sock_fd, buf, sizeof(buf), 0) < 0) {
        exit_program_err();
        return false;
    }
    cmsg = (struct cn_msg*)NLMSG_DATA(buf);
    unsigned short len = cmsg->len;
    memcpy(buffer, &len, 2);
    memcpy((char*)buffer+2, cmsg->data, len);
    return true;
}

void clean_on_exit(void)
{
    exit_program_err();
}
