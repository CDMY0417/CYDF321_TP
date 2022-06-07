//chat server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define MAXLEN 100
#define QUEUELIMIT 5
#define MAXCLIENT 5
#define FAIL -1
#define SUCCESS 0

int client_num = 0;
int sd, cd, len;
int sdarr[MAXCLIENT] = {-1, -1, -1, -1, -1};
char buff[MAXLEN];
int i = 0;
struct sockaddr_in server_addr, client_addr;
pthread_t tid[MAXCLIENT];
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

typedef struct _thread_arg
{
	int idx;
	struct sockaddr_in client_addr;
}thread_arg;

void usage() {
	printf("syntax : ./server <port number>\n");
	printf("sample : ./server 8080\n");
}

void broadcast(int idx, char* str, int len)
{
	pthread_mutex_lock(&mut);
	for(i = 0; i < MAXCLIENT; i++) {
		if(i == idx || sdarr[i] < 0) continue;
		send(sdarr[i], str, len, 0);
	}
	pthread_mutex_unlock(&mut);
}

int recv_msg(int cd, char* buff, int maxlen)
{
        recv(cd, buff, maxlen, 0);
	int len = strlen(buff) - 1;
        return strncmp("QUIT", buff, (len > 4? len: 4));
}

void thread_func(void* arg)
{
	char nickname[MAXLEN] = {0};
	char buff[MAXLEN] = {0};
	thread_arg ta = *(thread_arg*)arg;
	printf("Connection from %s:%d\n", inet_ntoa(ta.client_addr.sin_addr), ntohs(ta.client_addr.sin_port));
	recv(sdarr[ta.idx], nickname, MAXLEN, 0);
	snprintf(buff, MAXLEN, "%s is connected", nickname);
	broadcast(ta.idx, buff, strlen(buff));
	puts(buff);

	int res;
	while (1) {
		memset(buff, 0, MAXLEN);
		res = recv_msg(sdarr[ta.idx], buff, MAXLEN);
		if(res == 0) snprintf(buff, MAXLEN, "%s is disconnected", nickname);
		else
		{
			char* tmp = strdup(buff);
			snprintf(buff, MAXLEN, "%s: %s", nickname, tmp);
		}
		broadcast(ta.idx, buff, strlen(buff));
		puts(buff);
		if(res == 0) {
			pthread_mutex_lock(&mut);
			close(sdarr[ta.idx]);
			client_num--;
			sdarr[ta.idx] = -1;
			pthread_mutex_unlock(&mut);
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		usage();
		return FAIL;
	}

	uint16_t port = atoi(argv[1]);

	//create socket
	if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket creation error"); //socket creation error
		return FAIL;
	}

	//assign IP and PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket
	if (bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("socket bind error"); //socket bind error
		return FAIL;
	}

	//listen
	if (listen(sd, QUEUELIMIT) < 0) {
		perror("server listen error"); //server listen error
		return FAIL;
	}
	len = sizeof(client_addr);

	//accept and broadcast
	thread_arg ta;
	while (1) {
		pthread_mutex_lock(&mut);
		if (client_num >= MAXCLIENT) {
			pthread_mutex_unlock(&mut);
			continue;
		} else client_num++;
		pthread_mutex_unlock(&mut);
		cd = accept(sd, (struct sockaddr*)&client_addr, &len);
		pthread_mutex_lock(&mut);
		if (cd < 0) {
			client_num--;
			pthread_mutex_unlock(&mut);
			continue;
		}
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(sdarr[i] == -1){
				ta.idx = i;
				sdarr[i] = cd;
				break;
			}
		}
		pthread_mutex_unlock(&mut);
		ta.client_addr = client_addr;
		pthread_create(tid + ta.idx, NULL, (void*(*)(void*))thread_func, (void*)&ta);
	}
}
