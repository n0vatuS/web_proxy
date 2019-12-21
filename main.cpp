#include <stdio.h> // for perror
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <stdlib.h> // for exit
#include <pthread.h>
#include <netdb.h>

#include <set>
#include <vector>
#include <thread>

using namespace std;

set<int> Clients;

bool compare_method(char * packet) {
	const char * method[6] = {"GET", "POST", "HEAD", "PUT", "DELETE", "OPTIONS"};
	for(int i = 0; i < 6; i++) {
		if(!memcmp(packet, method[i], strlen(method[i])) && packet[strlen(method[i])] == 0x20)
            return true;
	}
	return false;
}

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
		printf("%s\n", buf);
        if(!compare_method(buf)) {
            break;
        }
        char host[50];
        const char * str = "Host: ";
        char * tmp = strstr((char *)buf, str);
        if(tmp == NULL) {
            return;
        }
        int i = 0;
        while(tmp[i + strlen(str)] != 0x0d) {
            host[i] = tmp[i+strlen(str)];
            i++;
        }
        printf("host : %s\n", host);

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(80);
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
        struct hostent * host_entry = gethostbyname(host); 
        char * ip = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
        if(!inet_aton(ip, &addr.sin_addr)) {
            printf("Conversion Error!\n");
            exit(1);
        }
        int res = connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
        if(res < 0) {
            printf("can't connect\n");
            return;
        }

        ssize_t sent = send(sockfd, buf, strlen(buf), 0);
        char buf2[BUFSIZE];

		ssize_t received2 = recv(sockfd, buf2, BUFSIZE - 1, 0);
		if (received2 == 0 || received2 == -1) {
			break;
		}
		printf("%s\n", buf2);
        
        ssize_t sent2 = send(childfd, buf2, strlen(buf2), 0);
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
