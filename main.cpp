#include <stdio.h> // for perror
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <stdlib.h> // for exit
#include <pthread.h>

#include <set>
#include <vector>
#include <thread>

using namespace std;

set<int> Clients;

void echo(int childfd) {
	if (childfd < 0) {
		perror("ERROR on accept");
		return;
	}
	printf("connected  [Client %d]\n", childfd);

	while (true) {
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];

		ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			printf("recv failed to %d\n", childfd);
			Clients.erase(childfd);
			break;
		}
		snprintf(buf+received, 14, "  [Client %d]\0", childfd);
		printf("%s\n", buf);
        
        ssize_t sent = send(childfd, buf, strlen(buf), 0);
        if (sent == 0) {
            printf("send failed to %d", childfd);
            Clients.erase(childfd);
            break;
        }
	}
}

void usage() {
	printf("syntax : web_proxy <tcp port>\n");
	printf("sample : web_proxy 8080\n");
}

int main(int argc, char ** argv) {
	if(argc != 2) {
		usage();
		exit(1);
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = ::bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (res == -1) {
		perror("bind failed");
		return -1;
	}

	res = listen(sockfd, 2);
	if (res == -1) {
		perror("listen failed");
		return -1;
	}
	
	vector<thread> T;
	while (true) {
		struct sockaddr_in addr;
		socklen_t clientlen = sizeof(sockaddr);
    
		int childfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &clientlen);
		
		Clients.insert(childfd);
		T.push_back(thread(echo, childfd));
	}
	close(sockfd);
}
