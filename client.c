#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h> //인터넷 프로토콜
#include <sys/socket.h> //소켓 통신 함수
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
	
#define BUF_SIZE 100 // 100바이트 버퍼 크기 사용
#define NAME_SIZE 20 
#define NORMAL_SIZE 20

char name[NAME_SIZE] = "[DEFAULT]"; //이름을 저장하는 문자열
char msg[BUF_SIZE]; // 메시지를 저장하는 문자열
char serv_time[NORMAL_SIZE]; // server time
char serv_port[NORMAL_SIZE]; // server port number
char clnt_ip[NORMAL_SIZE]; // client ip address
	
void * sendMsg(void * arg);
void * rcvMsg(void * arg);
void errorHandling(char * msg); //에러 처리 함수 (계속 재사용)
	
	
int main(int argc, char *argv[]) {
	pthread_t snd_t, rcv_t; // 쓰레드 선언 (쓰레드 송신, 쓰레드 수신)
	struct sockaddr_in serv_addr; //서버용 소켓(accept용)
	void * return_t;
	int sock;

	//input error (ip, port, nickname 순서대로 입력해야 함) 프로그램 잘못 실행 시 사용법 출력
	if (argc != 4) {
		printf("사용법 : %s [서버주소] [프트번호] [닉네임]\n", argv[0]);
		exit(1); // 비정상 종료 처리
	 }
	
	struct tm *t;
	time_t timer = time(NULL);
	t = localtime(&timer);
	printf("%d-%d-%d %d:%d\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min);
	
	sprintf(name, "[%s]", argv[3]); // “[%s]”로 서식을 지정하여 name(argv[3])에 저장
	//create socket
	sock=socket(PF_INET, SOCK_STREAM, 0); //IPv4, TCP 소켓 생성 후 SOCK에 저장
	memset(&serv_addr, 0, sizeof(serv_addr)); // 서버 구조체를 0으로 채움
	serv_addr.sin_family=AF_INET; // 서버 주소 체계를 인터넷 계열로 설정
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]); 
	serv_addr.sin_port=htons(atoi(argv[2])); // 서버가 사용할 포트 번호
	
	//error (TCP 통신용 서버 소켓 생성 실패)
	if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1) //서버 주소 정보를 기반으로 연결 요청, 이때 비로소 클라이언트 소켓이 됨 
  {
		errorHandling("connecting error occurred");
	}
	
	//create threads (쓰레드 생성)
	pthread_create(&snd_t, NULL, sendMsg, (void*)&sock); //메시지 보내는 쓰레드 생성 
	pthread_create(&rcv_t, NULL, rcvMsg, (void*)&sock); // 메시지 받는 쓰레드 생성
	pthread_join(snd_t, &return_t); //snd_t 쓰레드가 종료된 후 실행
	pthread_join(rcv_t, &return_t); //rcv_t 쓰레드가 종료된 후 실행

	close(sock);  // 클라이언트 소켓 연결 종료
	return 0;
}
	
// sending thread (sendMsg 쓰레드가 생성되자마자 실행) 송신
void * sendMsg(void * arg) { 

	//클라이언트의 파일 디스크립터
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];

	printf(" ========== join!!! Hello~ ========== \n");
	printf("Hello~ %s!!!! \n", name);
	
	write(sock, name, strlen(name)); //null 문자 제외하고 서버로 문자열 보냄

	while(1) {
		fgets(msg, BUF_SIZE, stdin); // 콘솔에서 문자열 입력 받음
		//exit
		if(!strcmp(msg,"exit\n") || !strcmp(msg,"goodbye\n")) { 
			close(sock); //클라이언트 소켓 종료
			exit(0); //프로그램 종료
		}
		if(!strcmp(msg, "change@\n")) {
			char nameTemp[100];
			printf("\nChange nickname -> ");
			scanf("%s", nameTemp);
			sprintf(name, "[%s]", nameTemp);
			printf("\nComplete. \n");
			continue;
		}
		if(!strcmp(msg, "clear@\n")) {
			system(“clear”);
		}
		
		int isPos = 0;
		for(int i = 0; i < BUF_SIZE; i++){
			if(msg[i] == '@'){ //msg에 @ 있으면 isPos에 1 저장
				isPos = 1;
				break;
			}
		}
		if(isPos == 0){ // msg에 @ 없으면 보낼 사람 적으라는 알림
			printf("Target user not found!\n");
			continue;
		}

		sprintf(name_msg,"%s %s", name, msg); // 클라이언트 이름과 msg 합침
		//send message to server (null 문자 제외하고 서버로 문자열 보냄)
		write(sock, name_msg, strlen(name_msg));
	}
	return NULL;
}

//reading thread (수신)
void* rcvMsg(void * arg) {

	//클라이언트의 파일 디스크립터
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	int str_len;

	while(1) {
		//get message from server
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);

		if (str_len == -1) { //read 실패시
			return (void*)-1;
		}
		
		char cur_name[NAME_SIZE];
		int clnt_name_size = 0;
		int idx = 0;
		
		//get receiver's name (받는사람의 이름을 가져옴)
		while(idx < BUF_SIZE){ //BUF_SIZE는 메시지 배열 크기
			if(name_msg[idx] == '@'){ //name_msg를 [0]부터 하나씩 가져와서 ‘@’만나면 break로 빠져나옴 
				idx++;
				break;
			}
			idx++;
		}
		for(int j = 0; name_msg[idx + j] != 32; j++){ //name_msg[idx]는 @, name_msg가 space를 만나기 전까지 돌아감
			cur_name[j] = name_msg[idx + j]; //받는 사람을 cur_name에 저장
			clnt_name_size++;
		}
		
		//check if this user is receiver
		int isPos = 1;
		for(int j = 0; j < clnt_name_size; j++){ //clnt_name_size는 받는 사람 크기
			if(name[j + 1] != cur_name[j]){ //name에는 @도 있으므로 1부터 시작, 두 값이 틀리면 isPos = 0
				isPos = 0;
				break;
			}
		}
		// cur_name이 ‘all’일 경우 isPos에 1 저장
		if(cur_name[0] == 'a' && cur_name[1] == 'l' && cur_name[2] == 'l'){
			isPos = 1;
		}
		
		//print message if this user is rigth receiver (누군가에게 보내거나 all일 때)
		if(isPos == 1){
			name_msg[str_len] = 0;
			
			char fnl_msg[BUF_SIZE + NAME_SIZE];
			int cur = 0;
			
			//get sender's name and message
			for(int j = 0; j < str_len;j++){ //str_len은 name_msg 크기
				if(name_msg[j] == '@'){
					while(name_msg[j] != 32){ //띄어쓰기 나올 때까지 j값 증가
						j++;
					}
					j++;
				}
				fnl_msg[cur++] = name_msg[j]; //fnl_msg는 name_msg에서 msg만 가져옴
			}
			fnl_msg[cur] = 0; //msg 마지막 값은 0 저장
			//print (콘솔에 출력)
			fputs(fnl_msg, stdout);
		}
	}

	return NULL;
}

//error handling (에러 처리)
void errorHandling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
