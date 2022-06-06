//chat client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#define MAXLEN 100
#define FAIL -1
#define SUCCESS 0

int sd, len;
char buff[MAXLEN];
int i = 0;
struct sockaddr_in client_addr;

void usage() {
	printf("syntax : ./client <ip address> <port number> <nickname>\n");
	printf("sample : ./client 127.0.0.1 8080 User1\n");
}

int send_msg(int cd, char* buff, int maxlen)
{
        fgets(buff, maxlen, stdin);
	int len = strlen(buff) - 1;
	send(cd, buff, len, 0);
        return strncmp("QUIT", buff, (len > 4? len: 4));
}


void thread_func(void* arg)
{
	char buff[MAXLEN] = {0};
	while (1) {
		if (recv(sd, buff, sizeof(buff), 0) < 0) continue;
		if(*buff != 0) puts(buff);
		memset(buff, 0, MAXLEN);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
		usage();
		return FAIL;
	}

	//create socket
	if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket creation error"); //socket creation error
		return FAIL;
	}

	//assign IP and PORT
	char* ip = argv[1];
	uint16_t port = atoi(argv[2]);
	char* nickname = argv[3];
	in_addr_t ip_addr = inet_addr(ip);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	client_addr.sin_addr.s_addr = ip_addr;

	//connect socket
	if (connect(sd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
		perror("socket connect error"); //socket connect error
		return FAIL;
	}

	printf("%s is connected\n", nickname);

	//chat
	pthread_t tid;
	pthread_create(&tid, NULL, (void*(*)(void*))thread_func, NULL);
	send(sd, nickname, strlen(nickname), 0);
	while (1) {
		if(send_msg(sd, buff, sizeof(buff)) == 0) {
			printf("%s is disconnected\n", nickname);
			break;
		}
		memset(buff, 0, MAXLEN);
	}
	pthread_cancel(tid);

	//close socket
	close(sd);
	return SUCCESS;
}
