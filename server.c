#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MAX_STRING  511
#define MAX 1024

char *EXIT = "exit";
char *START = "Connected chat\n";//print connected
int maxfd;			   //max sock
int n_user = 0;		           //total user
int n_chat = 0;		           //total chat num
int client_sock[MAX];		   //chating user sock number arr
char ip[MAX][20];		   //ip list arr
int listen_sock;                   //server listen  sock			
void add_Client(int s, struct sockaddr_in *new_client);//chat user input
int get_maxSock();		   //max sock number
void delete_Client(int s);	   //chat user output
int sock_listen(int host, int port, int backlog);//create sock and listen
void err_exit(char *mesg) {perror(mesg); exit(1);}

time_t c_time;
struct tm tm;

void *server_command(void *arg) { //command Thread
	int i;
	char name[MAX_STRING];
	printf("명령어 : help, num_user, num_chat, ip_list. admin\n");
	while (1) {
		char bufmsg[MAX_STRING + 1];
		printf("server>"); //print cur
		fgets(bufmsg, MAX_STRING, stdin); //input command
		if (!strcmp(bufmsg, "\n")) continue;   //enter pass
		else if (!strcmp(bufmsg, "help\n"))    //help print
			printf("help, num_user, num_chat, ip_list\n");
		else if (!strcmp(bufmsg, "num_user\n"))//total user number print
			printf("현재 참가자 수 = %d\n", n_user);
		else if (!strcmp(bufmsg, "num_chat\n"))//total chating number print
			printf("지금까지 오간 대화의 수 = %d\n", n_chat);
		else if (!strcmp(bufmsg, "ip_list\n")) //ip list print
			for (i = 0; i < n_user; i++)
				printf("%s\n", ip[i]);
		else if (!strcmp(bufmsg, "admin\n")){//admin 설정부분
			scanf("%s",&name);//admin 권한을 변경할 user name
			for (i = 0; i < n_user; i++)
				send(client_sock[i], name, MAX_STRING, 0);//pass name sock
		}
		else //input err
			printf("input err, enter help\n");
	}
}

int main(int argc, char *argv[]) {
	struct sockaddr_in client_add;
	char buf[MAX_STRING + 1]; //client message
	int i, j, num_b, accp_sock, addrlen = sizeof(struct
		sockaddr_in);
	fd_set read_f;	//read struct
	pthread_t new_thread;

	if (argc != 2) {
		printf("input :%s port\n", argv[0]);
		exit(0);
	}
	// sock_listen호출
	listen_sock = sock_listen(INADDR_ANY, atoi(argv[1]), 5);
	//create Thread
	pthread_create(&new_thread, NULL, server_command, (void *)NULL);
	while (1) {
		FD_ZERO(&read_f);
		FD_SET(listen_sock, &read_f);
		for (i = 0; i < n_user; i++)
			FD_SET(client_sock[i], &read_f);

		maxfd = get_maxSock() + 1;
		if (select(maxfd, &read_f, NULL, NULL, NULL) < 0)
			err_exit("select fail");

		if (FD_ISSET(listen_sock, &read_f)) {
			accp_sock = accept(listen_sock,
				(struct sockaddr*)&client_add, &addrlen);
			if (accp_sock == -1) err_exit("accept fail");
			add_Client(accp_sock, &client_add);
			send(accp_sock, START, strlen(START), 0);
			c_time = time(NULL);
			tm = *localtime(&c_time);
			write(1, "\033[0G", 4);		//cur x=0
			printf("[%02d:%02d:%02d]", tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("user input. user = %d\n", n_user);
			fprintf(stderr, "server>"); //cur print
		}

		//message print, all client
		for (i = 0; i < n_user; i++) {
			if (FD_ISSET(client_sock[i], &read_f)) {
				n_chat++;
				num_b = recv(client_sock[i], buf, MAX_STRING, 0);
				if (num_b <= 0) {
					delete_Client(i);//client delete
					continue;
				}
				buf[num_b] = 0;
				if (strstr(buf, EXIT) != NULL) {
					delete_Client(i);//client delete
					continue;
				}
				//message print, all client
				for (j = 0; j < n_user; j++)
					send(client_sock[j], buf, num_b, 0);
				printf("\033[0G");//cur x=0
				printf("%s", buf);//print massage
				fprintf(stderr, "server>"); //cur print
			}
		}

	}  // end of while

	return 0;
}

//new chating user
void add_Client(int s, struct sockaddr_in *new_client) {
	char buf[20];
	inet_ntop(AF_INET, &new_client->sin_addr, buf, sizeof(buf));
	write(1, "\033[0G", 4);//cur x=0
	printf("new client: %s\n", buf);//ip print
	client_sock[n_user] = s;
	strcpy(ip[n_user], buf);
	n_user++;
}

//chating user out
void delete_Client(int s) {
	close(client_sock[s]);
	if (s != n_user - 1) {
		client_sock[s] = client_sock[n_user - 1];
		strcpy(ip[s], ip[n_user - 1]);
	}
	n_user--;
	c_time = time(NULL);
	tm = *localtime(&c_time);
	write(1, "\033[0G", 4);	//cur x=0
	printf("[%02d:%02d:%02d]", tm.tm_hour, tm.tm_min, tm.tm_sec);
	printf("user out. user = %d\n", n_user);
	fprintf(stderr, "server>");
}

//max sock number
int get_maxSock() {
	int max_sock = listen_sock;
	int i;
	for (i = 0; i < n_user; i++)
		if (client_sock[i] > max_sock)
			max_sock = client_sock[i];
	return max_sock;
}

//create sock and listen
int  sock_listen(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in add_server;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket fail");
		exit(1);
	}
	//struct sock seting
	bzero((char *)&add_server, sizeof(add_server));
	add_server.sin_family = AF_INET;
	add_server.sin_addr.s_addr = htonl(host);
	add_server.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&add_server, sizeof(add_server)) < 0) {
		perror("bind fail");  exit(1);
	}
	//client listen
	listen(sd, backlog);
	return sd;
}
