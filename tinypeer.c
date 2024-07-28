#include <btc/btc.h>
#include <btc/buffer.h>
#include <btc/cstr.h>
#include <btc/protocol.h>
#include <btc/net.h>
#include <btc/chainparams.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define REGTEST_PORT "18444"

int main() {
	int sockfd;
	struct addrinfo hints, *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int rv = getaddrinfo("127.0.0.1", REGTEST_PORT, &hints, &result);
	if (rv != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            exit(EXIT_FAILURE);
	}

	struct addrinfo *rp;
	// getaddrinfo returns list of addresses in result
	// loop throught list and to try to connect until success
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		// first need to create socket
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) {
			continue;
		} 

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
	}

	printf("connected to peer\n");

	// if rp is null at this point, it could not connect
	if (rp == NULL) {
		fprintf(stderr, "unable to connect to peer");
		exit(EXIT_FAILURE);
	}
	
	// socket addr to peer addr
	btc_p2p_address fromAddr;
	btc_p2p_address_init(&fromAddr);

	btc_p2p_address toAddr;
	btc_p2p_address_init(&toAddr);
	btc_addr_to_p2paddr(result->ai_addr, &toAddr);

	freeaddrinfo(result);

	// string buffer for serialized message
	cstring *version_msg_cstr = cstr_new_sz(256);
 
	// create version message
	btc_p2p_version_msg version_msg;
	memset(&version_msg, 0, sizeof(version_msg));

	btc_p2p_msg_version_init(&version_msg, &fromAddr, &toAddr, "70016", false);
	btc_p2p_msg_version_ser(&version_msg, version_msg_cstr);

	cstring *p2p_version_msg = btc_p2p_message_new(btc_chainparams_regtest.netmagic, BTC_MSG_VERSION, version_msg_cstr->str, version_msg_cstr->len);

	if (send(sockfd, p2p_version_msg->str, p2p_version_msg->len, 0) == -1) {
		perror("error sending version message");
		exit(EXIT_FAILURE);
	}
	printf("sent version message to peer\n");

	cstr_free(version_msg_cstr, true);
	cstr_free(p2p_version_msg, true);

	for (;;) {
		cstring *rec_buf = cstr_new_sz(BTC_P2P_MESSAGE_CHUNK_SIZE);
		int rs = recv(sockfd, rec_buf->str, BTC_P2P_MESSAGE_CHUNK_SIZE, 0);
		if (rs == -1) {
			perror("error recv");
		}

		btc_p2p_msg_hdr msg_header;
		struct const_buffer header_buf = {rec_buf->str, BTC_P2P_HDRSZ};

		btc_p2p_deser_msghdr(&msg_header, &header_buf);
		if (strcmp(msg_header.command, BTC_MSG_VERACK) == 0) {
			printf("received verack message from peer\n");
		}

		if (strcmp(msg_header.command, BTC_MSG_VERSION) == 0) {
			printf("received version message from peer\n");
		}

		cstr_free(rec_buf, true);
	}
}

