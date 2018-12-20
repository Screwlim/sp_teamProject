#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXLINE     1000
#define NAME_SIZE    20

char *EXIT = "exit";
int tcp_connect(int af, char *servip, unsigned short port);
void err_exit(char *mesg) { perror(mesg); exit(1); }

int main(int argc, char *argv[]) {
	char bufname[NAME_SIZE];	//name buf
	char bufmsg[MAXLINE];	//massage buf
	char bufall[MAXLINE + NAME_SIZE];
	int max_sock;
	int s;		//sock
	int admin = 1;  //관리자권한 구분 변수 0: 관리자 1: 학생
	fd_set read_f;
	time_t ct;
	struct tm tm;

	if (argc != 4) {
		printf("사용법 : %s sever_ip  port name \n", argv[0]);
		exit(0);
	}
	if(!strcmp(argv[3],"admin")){
		printf("admin 닉네임 사용불가\n");
		exit(1);
	}
	s = tcp_connect(AF_INET, argv[1], atoi(argv[2]));
	if (s == -1)
		err_exit("tcp_connect fail");

	puts("connect server");
	max_sock = s + 1;
	FD_ZERO(&read_f);

	while (1) {
		FD_SET(0, &read_f);
		FD_SET(s, &read_f);
		if (select(max_sock, &read_f, NULL, NULL, NULL) < 0)
			err_exit("select fail");
		if (FD_ISSET(s, &read_f)) {
			int nbyte;
			if ((nbyte = recv(s, bufmsg, MAXLINE, 0)) > 0) {
				bufmsg[nbyte] = 0;
				write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
				//admin 권한변경,server로 전송받은 name과 client의 name 비교
				if(!strcmp(bufmsg,argv[3])){
					if(admin == 1){
						admin =0;//조교권한
						strcpy(argv[3],"admin");
					}
					continue;
				}
				printf("%s", bufmsg);		//print massage
				fprintf(stderr, "\033[1;32m");	//글자색 = 녹색
				fprintf(stderr, "%s%d>",argv[3],admin);//내 닉네임 출력//뒤에정수는권한

			}
		}
		if (FD_ISSET(0, &read_f)) {
			if (fgets(bufmsg, MAXLINE, stdin)) {
				fprintf(stderr, "\033[1;33m"); //글자색 = 노란색
				fprintf(stderr, "\033[1A"); //cur y = y -1
				ct = time(NULL);
				tm = *localtime(&ct);
				sprintf(bufall, "[%02d:%02d:%02d]%s>%s", tm.tm_hour, tm.tm_min, tm.tm_sec, argv[3], bufmsg);
				if (send(s, bufall, strlen(bufall), 0) < 0)
					puts("Error : Write error on socket.");
				if (strstr(bufmsg, EXIT) != NULL) {
					puts("Good bye.");
					close(s);
					exit(0);
				}
			}
		}
	} // end of while
}

int tcp_connect(int af, char *servip, unsigned short port) {
	struct sockaddr_in server_add;
	int  s;
	//create socket
	if ((s = socket(af, SOCK_STREAM, 0)) < 0)
		return -1;
	//chating server sock struct seting
	bzero((char *)&server_add, sizeof(server_add));
	server_add.sin_family = af;
	inet_pton(AF_INET, servip, &server_add.sin_addr);
	server_add.sin_port = htons(port);

	//server connect
	if (connect(s, (struct sockaddr *)&server_add, sizeof(server_add))
		< 0)
		return -1;
	return s;
}
