#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 100
#define MAX_CLNT 20
#define NAME_SIZE 20

void errorHandling(char *msg);

int main(int argc, char *argv[]) {
	struct sockaddr_in serv_addr; // 서버 주소, 포트번호 저장
	struct sockaddr_in clnt_addr; // 클라이언트 주소, 포트번호 저장
	struct timeval timeout; // delay 할 수 있는 함수
	int serv_sock, clnt_sock;
	fd_set fds, cpy_fds; //fd_set 구조체 사용을 하기 위한 변수 선언
	
	int clnt_socks[MAX_CLNT];
	int clnt_cnt = 0;
	char clnt_names[MAX_CLNT][NAME_SIZE];
	int clnt_checks[MAX_CLNT];
	for(int i = 0; i < MAX_CLNT; i++){
		clnt_checks[i] = 0;
	}

	char buf[BUF_SIZE];
	int fd_max, str_len, fd_num;
	socklen_t addr_size;
	
	//input error (port 입력해야 함)
	if(argc != 2) {
		printf("사용법 : %s <포트번호>\n", argv[0]);
		exit(1);
	}
	
	//create socket
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //TCP 소켓 생성
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //IPv4를 쓰고
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //서버 ip 지정
	serv_addr.sin_port = htons(atoi(argv[1])); // 서버 포트 지정
	
	//error (성공 시 0 리턴, 실패시 1 리턴)
	if (bind(serv_sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1) // 파일 디스크립터 serv_sock의 소켓에 주소 할당 (ip주소와 포트번호 할당) 
	{
		errorHandling("binding error occurred");
	}
	if (listen(serv_sock, 5) == -1) { //소켓을 연결 요청을 받을 수 있는 대기상태로 만듦
		errorHandling("listening error occurred");
	}

	//set up io multiplexing (하나의 통신 채널에 2개 이상의 데이터 전송)
	FD_ZERO(&fds); //fd_set으로 선언된 변수 초기화
	//serv_sock is changed if client call connect function
	FD_SET(serv_sock, &fds); //서버 소켓을 관리대상으로 지정
	fd_max = serv_sock; // 최대 파일 디스크립터의 값

	while(1) {
		cpy_fds = fds; //원본 fd_set 복사
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000; //타임아웃 설정

		//error case (아직 서버 소켓만 있으므로 connect 요청시 서버소켓에 데이터가 들어오게 됨)
		if((fd_num = select(fd_max + 1, &cpy_fds, 0, 0, &timeout)) == -1){
			break;
		}
		//no change (타임아웃 시 continue)
		if(fd_num == 0) {
			continue;
		}
		
		//check every fd (소켓에 따른 구분)
		for(int i = 0; i < fd_max + 1; i++) { //fd_set의 인덱스를 하나씩 돌리면서 변화가 있는 인덱스를 찾아냄
			if (!FD_ISSET(i, &cpy_fds)) {
				continue;
			}

			// connection request (만약 변화가 일어난 소켓이 서버 소켓이면 connect 요청인 경우임)
			if(serv_sock == i) {		
				addr_size = sizeof(clnt_addr);
				clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &addr_size);
				
				clnt_socks[clnt_cnt++] = clnt_sock;
				FD_SET(clnt_sock, &fds);
				if(fd_max < clnt_sock)
					fd_max = clnt_sock;
			}
			// read message (다른 소켓이면 데이터 read)
			else {
				str_len = read(i, buf, BUF_SIZE);
				// close request
				if(str_len == 0) {
					FD_CLR(i, &fds);
					close(i);
					
					printf("@%s has left the chat\n", clnt_names[i]);
					
					clnt_checks[i] = 0;
					for(int j = 0; j < NAME_SIZE; j++){
						clnt_names[i][j] = 0;
					}
					
				}
				//get client name after connect request
				else if(clnt_checks[i] == 0){
					//set clnt_name as 0
					for(int j = 0; j < NAME_SIZE; j++){
						clnt_names[i][j] = 0;
					}
					//get client name
					for(int j = 1; buf[j] != ']'; j++){
						clnt_names[i][j - 1] = buf[j];
					}
					printf("User @%s has joined the chat\n", clnt_names[i]);
					clnt_checks[i] = 1;
				}
				//send message to all clients
				else {  				
					for(int j = 0; j < clnt_cnt; j++) {
						write(clnt_socks[j], buf, str_len);
					}
				}
				//set buf as 0
				for(int j = 0; j < BUF_SIZE; j++){
					buf[j] = 0;
				}
			}
		}
	}
	close(serv_sock);
	return 0;
}

//error handling
void errorHandling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
