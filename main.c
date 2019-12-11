#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#define _CRT_SECURE_NO_WARNINGS   
#define BLKSIZE 1024

int playBowling();
void rank(); 
void rank_remove();
ssize_t r_write();
ssize_t r_read();
 
int copyfile(int fromfd, int tofd) {
   char buf[BLKSIZE]; 
   int bytesread, byteswritten;
   int totalbytes = 0;

   for (  ;  ;  ) {
      if ((bytesread = r_read(fromfd, buf, BLKSIZE)) <= 0)
         break;     
      if ((byteswritten = r_write(tofd, buf, bytesread)) == -1)
         break;
      totalbytes += byteswritten;
   }
   return totalbytes;
}

int r_close(int fd) {
   int retval;

   while (retval = close(fd), retval == -1 && errno == EINTR) ;
   return retval;
}


void timer()
{
    r_write(STDOUT_FILENO, "\n=== 광고: 한아름볼링장 GS마트 옆 위치, 건국대학교 학생 대상 할인 ===\n", strlen("\n=== 광고: 한아름볼링장 GS마트 옆 위치, 건국대학교 학생 대상 할인 ===\n"));
}
 
int createTimer(timer_t *timerID, int sec, int msec)  
{      
    int sigNo = SIGALRM;  
    struct sigevent te;  
    struct itimerspec its;  
    struct sigaction sa;  
     
    sa.sa_flags = SA_SIGINFO;  
    sa.sa_sigaction = timer;      
    sigemptyset(&sa.sa_mask);  
  
    if (sigaction(sigNo, &sa, NULL) == -1)  
    {  
        printf("sigaction error\n");
        return -1;  
    }  
   
    te.sigev_notify = SIGEV_SIGNAL;  
    te.sigev_signo = sigNo;  
    te.sigev_value.sival_ptr = timerID;  
    timer_create(CLOCK_REALTIME, &te, timerID); 
   
    its.it_interval.tv_sec = sec;
    its.it_interval.tv_nsec = msec * 1000000;  
    its.it_value.tv_sec = sec;
    
    its.it_value.tv_nsec = msec * 1000000;
    timer_settime(*timerID, 0, &its, NULL);  
   
    return 0;  
}

void *copyfilemalloc(void *arg)  { /* copy infd to outfd with return value */
   int *bytesp;
   int infd;
   int outfd;
 
   infd = *((int *)(arg));
   outfd = *((int *)(arg) + 1);
   if ((bytesp = (int *)malloc(sizeof(int))) == NULL)
      return NULL;
   *bytesp = copyfile(infd, outfd);
   r_close(infd);
   r_close(outfd);
   return bytesp; 
}

int main()
{
    int bowling_score;
    char name[20];
    char menu_c[2];
    int menu;
    FILE *fp;    
    int fd;
    int *bytesptr;
    int error; 
    int fds[2];
    pthread_t tid;
    sigset_t newsigset;    
    timer_t timerID;

    if (sigemptyset(&newsigset) == -1) {
        perror("Failed to initialize the signal set");
    }

    createTimer(&timerID, 10, 0);

    do {        
        r_write(STDOUT_FILENO, "[1] 볼링게임 [2] 랭킹확인 [3] 랭킹백업 [4] 백업불러오기 [5] 초기화 후 종료 [6] 저장 후 종료\n", strlen("[1] 볼링게임 [2] 랭킹확인 [3] 랭킹백업 [4] 백업불러오기 [5] 초기화 후 종료 [6] 저장 후 종료\n"));
        r_read(STDIN_FILENO, menu_c, 2);
        menu = atoi(menu_c);
        if (menu == 1) {
            r_write(STDOUT_FILENO, "이름을 입력하세요: ", strlen("이름을 입력하세요: "));
            r_read(STDIN_FILENO, name, 20);
            bowling_score = playBowling();
            if (bowling_score == 0) {
                r_write(STDOUT_FILENO, "0점을 기록하셨습니다. 점수가 등록되지 않습니다.\n", strlen("0점을 기록하셨습니다. 점수가 등록되지 않습니다.\n"));
            }
            else {
                fp = fopen("rank.txt", "a");     
                fprintf(fp, "%s %d\n", name, bowling_score);   // 서식을 지정하여 문자열을 파일에 저장
                fclose(fp);    // 파일 포인터 닫기
            }
        }
        else if (menu == 2) {
            fd = open("rank.txt", (O_RDWR | O_CREAT), (S_IRWXU | S_IRWXG| S_IRWXO));  
            close(fd); 
            rank();
        }
        else if (menu == 3) {
            fds[0] = open("rank.txt", (O_RDWR | O_CREAT), (S_IRWXU | S_IRWXG| S_IRWXO));
            fds[1] = open("rank_backup.txt", (O_RDWR | O_CREAT), (S_IRWXU | S_IRWXG| S_IRWXO));
            if (error = pthread_create(&tid, NULL, copyfilemalloc, fds)) {
                fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
                return 1;
            }
            if (error = pthread_join(tid, (void **)&bytesptr)) {
                fprintf(stderr, "Failed to join thread: %s\n", strerror(error));
                return 1;
            }
            printf("Number of bytes copied: %d\n", *bytesptr);
        }
        else if (menu == 4) {
            fds[1] = open("rank.txt", (O_RDWR | O_CREAT), (S_IRWXU | S_IRWXG| S_IRWXO));
            fds[0] = open("rank_backup.txt", (O_RDWR | O_CREAT), (S_IRWXU | S_IRWXG| S_IRWXO));
            if (error = pthread_create(&tid, NULL, copyfilemalloc, fds)) {
                fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
                return 1;
            }
            if (error = pthread_join(tid, (void **)&bytesptr)) {
                fprintf(stderr, "Failed to join thread: %s\n", strerror(error));
                return 1;
            }
            printf("Number of bytes copied: %d\n", *bytesptr);
        }
        else if (menu == 5) {
            atexit(rank_remove);
            printf("기록을 초기화하고 게임을 종료합니다.\n");
            exit(0);
        }
        else if (menu == 6) {
            printf("기록을 저장하고 게임을 종료합니다.\n");
        }
        else if (menu == 9) {
            if (sigismember(&newsigset, SIGALRM)) {
                if(sigprocmask(SIG_UNBLOCK, &newsigset, NULL) == -1) {
                    perror("Failed to unblock SIGALRM");
                } 
                sigdelset(&newsigset, SIGALRM);
                r_write(STDOUT_FILENO, "광고부착완료\n", strlen("광고부착완료\n"));
            } else {     
                sigaddset(&newsigset, SIGALRM);       
                if(sigprocmask(SIG_BLOCK, &newsigset, NULL) == -1) {
                    perror("Failed to block SIGALRM");
                }    
                r_write(STDOUT_FILENO, "광고제거완료\n", strlen("광고제거완료\n"));
            }            
        }
    } while (menu != 6);
 
    return 0;
}

ssize_t r_write(int fd, void *buf, size_t size) {
   char *bufp;
   size_t bytestowrite;
   ssize_t byteswritten;
   size_t totalbytes;

   for (bufp = buf, bytestowrite = size, totalbytes = 0;
        bytestowrite > 0;
        bufp += byteswritten, bytestowrite -= byteswritten) {
      byteswritten = write(fd, bufp, bytestowrite);
      if ((byteswritten == -1) && (errno != EINTR))
         return -1;
      if (byteswritten == -1)
         byteswritten = 0;
      totalbytes += byteswritten;
   }
   return totalbytes;
}

ssize_t r_read(int fd, void *buf, size_t size) {
   ssize_t retval;

   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR);
   return retval;
}   

void rank_remove() {
    remove("rank.txt");
}

void rank() {
    FILE *fp;
 
    char rank_name[100][10];
    int rank_score[100] = { 0 };
    char name[10];
    int bowling_score;
    int file_index = 0;
    int i, j;
    int temp;
    char temp_name[10];
 
    fp = fopen("rank.txt", "r");    // rank.txt 파일을 읽기 모드(r)로 열기.
 
    while (!feof(fp)) {
        fscanf(fp, "%s %d", name, &bowling_score);   // 서식을 지정하여 파일에서 문자열 읽기                
        if (!feof(fp)) {
            strcpy(rank_name[file_index], name);
            rank_score[file_index] = bowling_score;
            file_index++;
        }
    }
    file_index = 0;
    fclose(fp);    // 파일 포인터 닫기
 
    for (i = 0; rank_score[i] != 0; i++) {
        for (j = i; rank_score[j] != 0; j++) {
            if (rank_score[i] < rank_score[j]) {
                temp = rank_score[i];
                rank_score[i] = rank_score[j];
                rank_score[j] = temp;
                strcpy(temp_name, rank_name[i]);
                strcpy(rank_name[i], rank_name[j]);
                strcpy(rank_name[j], temp_name);
            }
        }
    }
 
    printf("----------------------\n");
    for (i = 0; rank_score[i] != 0; i++) {
        printf("(%d등) %s %d\n", i + 1, rank_name[i], rank_score[i]);
    }
    printf("----------------------\n");
}

int playBowling()
{
    int now_hit;       // 현재 맞춘 점수 입력
    int frame_num = 0; // 몇번째 프레임인지 확인
    int i;
    int sum_score = 0;
    int bonus = 0;    

    int first_tern[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int second_tern[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int final_score[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, -1};

    int strike_check[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int spare_check[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    char now_hit_s[3];

    for (; frame_num < 9; frame_num++)
    {
        printf("[%d번째 프레임]\n", frame_num + 1);
        r_write(STDOUT_FILENO, "첫번째 턴, 점수를 입력하세요: ", strlen("첫번째 턴, 점수를 입력하세요: "));
        r_read(1, now_hit_s, 3);
        now_hit = atoi(now_hit_s);
        if (now_hit > 10)
        {
            printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
            frame_num--;
            continue;
        }
        first_tern[frame_num] = now_hit;
        if (first_tern[frame_num] == 10)
        {
            strike_check[frame_num] = 1;
            printf("축하합니다. 스트라이크입니다.\n");
            final_score[frame_num] = 10;
            continue;
        }
        r_write(STDOUT_FILENO, "두번째 턴, 점수를 입력하세요: ", strlen("두번째 턴, 점수를 입력하세요: "));
        r_read(1, now_hit_s, 3);
        now_hit = atoi(now_hit_s);
        if (first_tern[frame_num] + now_hit > 10)
        {
            printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
            frame_num--;
            continue;
        }
        second_tern[frame_num] = now_hit;
        if (first_tern[frame_num] + second_tern[frame_num] == 10)
        {
            spare_check[frame_num] = 1;
            printf("축하합니다. 스페어입니다.\n");
            final_score[frame_num] = 10;
            continue;
        }
        final_score[frame_num] = first_tern[frame_num] + second_tern[frame_num];
    }

    while (final_score[9] == -1)
    {
        printf("[%d번째 프레임]\n", frame_num + 1);
        r_write(STDOUT_FILENO, "첫번째 턴, 점수를 입력하세요: ", strlen("첫번째 턴, 점수를 입력하세요: "));
        r_read(1, now_hit_s, 3);
        now_hit = atoi(now_hit_s);
        if (now_hit > 10)
        {
            printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
            continue;
        }
        first_tern[frame_num] = now_hit;
        if (first_tern[frame_num] == 10)
        {
            strike_check[frame_num] = 1;
            printf("축하합니다. 스트라이크입니다.\n");
            r_write(STDOUT_FILENO, "두번째 턴, 점수를 입력하세요: ", strlen("두번째 턴, 점수를 입력하세요: "));
            r_read(1, now_hit_s, 3);
            now_hit = atoi(now_hit_s);
            if (now_hit > 10)
            {
                printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
                continue;
            }
            second_tern[frame_num] = now_hit;
            if (second_tern[frame_num] == 10)
            {
                strike_check[frame_num + 1] = 1;
                printf("축하합니다. 스트라이크입니다.\n");
                r_write(STDOUT_FILENO, "보너스 턴, 점수를 입력하세요: ", strlen("보너스 턴, 점수를 입력하세요: "));
                r_read(1, now_hit_s, 3);
                now_hit = atoi(now_hit_s);
                if (now_hit > 10)
                {
                    printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
                    continue;
                }
                bonus = now_hit;
                if (bonus == 10)
                {
                    strike_check[frame_num + 2] = 1;
                    printf("축하합니다. 스트라이크입니다.\n");
                }
                final_score[frame_num] = first_tern[frame_num] + second_tern[frame_num] + bonus;
                break;
            }
            r_write(STDOUT_FILENO, "보너스 턴, 점수를 입력하세요: ", strlen("보너스 턴, 점수를 입력하세요: "));
            r_read(1, now_hit_s, 3);
            now_hit = atoi(now_hit_s);
            if (second_tern[frame_num] + now_hit > 10)
            {
                printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
                continue;
            }
            bonus = now_hit;
            if (bonus == 10)
            {
                strike_check[frame_num + 2] = 1;
                printf("축하합니다. 스트라이크입니다.\n");
            }
            final_score[frame_num] = first_tern[frame_num] + second_tern[frame_num] + bonus;
            break;
        }
        r_write(STDOUT_FILENO, "두번째 턴, 점수를 입력하세요: ", strlen("두번째 턴, 점수를 입력하세요: "));
        r_read(1, now_hit_s, 3);
        now_hit = atoi(now_hit_s);
        if (first_tern[frame_num] + now_hit > 10)
        {
            printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
            continue;
        }
        second_tern[frame_num] = now_hit;
        if (first_tern[frame_num] + second_tern[frame_num] == 10)
        {
            spare_check[frame_num] = 1;
            printf("축하합니다. 스페어입니다.\n");
            r_write(STDOUT_FILENO, "보너스 턴, 점수를 입력하세요: ", strlen("보너스 턴, 점수를 입력하세요: "));
            r_read(1, now_hit_s, 3);
            now_hit = atoi(now_hit_s);
            if (now_hit > 10)
            {
                printf("잘못된 입력입니다. 프레임을 재실행합니다. \n");
                continue;
            }
            bonus = now_hit;
            final_score[frame_num] = first_tern[frame_num] + second_tern[frame_num] + bonus;
            break;
        }
        final_score[frame_num] = first_tern[frame_num] + second_tern[frame_num] + bonus;
    }

    for (i = 0; i < 10; i++)
    { //스트라이크 점수처리
        if (strike_check[i] == 1 && strike_check[i + 1] != 1 && strike_check[i + 2] != 1)
        {
            if (i == 9)
            {
                final_score[i] = 10 + second_tern[9] + bonus;
            }
            else
            {
                final_score[i] = 10 + first_tern[i + 1] + second_tern[i + 1];
            }
        }
        else if (strike_check[i] == 1 && strike_check[i + 1] == 1 && strike_check[i + 2] != 1)
        {
            if (i == 9)
            {
                final_score[i] = 20 + bonus;
            }
            else if (i == 8)
            {
                final_score[i] = 20 + second_tern[9];
            }
            else
            {
                final_score[i] = 20 + first_tern[i + 2];
            }
        }
        else if (strike_check[i] == 1 && strike_check[i + 1] == 1 && strike_check[i + 2] == 1)
        {
            final_score[i] = 30;
        }
    }    

    for (i = 0; i < 10; i++)
    { //스페어 점수처리
        if (spare_check[i] == 1)
        {
            if (i == 9)
            {
                final_score[i] = 10 + bonus;
            }
            else
            {
                final_score[i] = 10 + first_tern[i + 1];
            }
        }
    }

    for (frame_num = 0; frame_num < 10; frame_num++)
    { // 점수 출력
        printf("[%5d]", strike_check[frame_num]);
    }
    printf("\n=================================================================================\n");
    for (frame_num = 0; frame_num < 10; frame_num++)
    { // 점수 출력
        printf("[%5d]", spare_check[frame_num]);
    }
    printf("\n=================================================================================\n");

    for (frame_num = 0; frame_num < 10; frame_num++)
    { // 점수 출력
        printf("%7d", frame_num + 1);
    }
    printf("\n=================================================================================\n");

    for (frame_num = 0; frame_num < 9; frame_num++)
    { // 점수 출력
        printf("[%2d/%2d]", first_tern[frame_num], second_tern[frame_num]);
    }
    printf("[%2d/%2d/%2d]", first_tern[frame_num], second_tern[frame_num], bonus);
    printf("\n---------------------------------------------------------------------------------\n");

    for (frame_num = 0; frame_num < 10; frame_num++)
    { // 점수 출력
        printf("[%5d]", final_score[frame_num]);
        sum_score += final_score[frame_num];
    }

    printf(" 총점 :%d\n", sum_score);

    

    return sum_score;
}