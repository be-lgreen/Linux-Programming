#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define BUFF_SIZE 512

#define PROC_OPT_F 0x0001
#define PROC_OPT_T 0x0002 
#define PROC_OPT_C 0x0004
#define PROC_OPT_N 0x0008
#define PROC_OPT_M 0x0010
#define PROC_OPT_K 0x0020
#define PROC_OPT_W 0x0040
#define PROC_OPT_E 0x0080
#define PROC_OPT_L 0x0100
#define PROC_OPT_V 0x0200
#define PROC_OPT_R 0x0400
#define PROC_OPT_S 0x0800
#define PROC_OPT_O 0x1000

//need for count number of variable factor and find my_argv[] array index
#define LOCATION_F 0
#define LOCATION_T 1
#define LOCATION_C 2
#define LOCATION_N 3
#define LOCATION_M 4
#define LOCATION_K 5
#define LOCATION_S 6
#define LOCATION_O 7

//need for sequential print in -s option
#define S_FILEDES 0x01
#define S_CMDLINE 0x02
#define S_IO 0x04
#define S_STAT 0x08
#define S_ENVIRON 0x10
/*
   20152387 신연수

 */
void func_f(int pid,int pid_num); // /proc/[pid]/fd
void func_t(int pid,int pid_num); // /proc/[pid]/status
void func_c(int pid,int pid_num); // /proc/[pid]/cmdline
void func_n(int pid,int pid_num); // /proc/[pid]/io
void func_m(int pid,int pid_num); // /proc/[pid]/environ
void func_w(int pid_num); // /proc/interrupts
void func_e(int pid_num); // /proc/filesystem
void func_l(int pid_num); // /proc/uptime
void func_v(int pid_num); // /proc/vesion
void func_s(int uid);
int same_uid(char *d_name,char *next_path,int uid);

int arrange_option(int argc, char *argv[]);
void ascendingsort();
void my_ascendingsort(int num, int start);
void alphabetsort();

int flags; //for all option
int s_flags; //for -s option

char my_argv[BUFF_SIZE][BUFF_SIZE]; //sorted argv
char l_argv[9][2] = {0}; //location of option in argv

int main(int argc, char *argv[])
{
	char *start_string = ">: ssu_lsproc start. :<\n";
	char *terminate_string = ">: ssu_lsproc terminated. :<\n";

	int i=0 ,j=0;
	int plus_argv_num =0;

	//save option in flags 
	//save option location in l_argv[][]
	int success_parsing=0;
	success_parsing = arrange_option(argc,argv);

	//if parsing is wrong then print message
	if(success_parsing == 1)
	{
		printf("you can use -f -t -c -n -m -k -w -e -l -v -r -s -o option\n");
		printf("you can't use filename start with '-' in option -o\n");
		printf("please try again!!!\n");
		exit(1);
	}
	//check optin -o used correctly
	if(l_argv[LOCATION_O][1]-l_argv[LOCATION_O][0] >1)
	{
		printf("you can input only one filename after option -o\n");
		exit(1);
	}
	if((flags & PROC_OPT_O) && (l_argv[LOCATION_O][1]-l_argv[LOCATION_O][0]) == 0 )
	{
		printf("you should input filename after option -o\n");
		exit(1);
	}
	//check option -m and option -k used correctly
	if(flags & PROC_OPT_K)
	{
		if(! (flags & PROC_OPT_M))
		{
			printf("you can't use option -k without option -m\n");
			exit(1);
		}

		if((l_argv[LOCATION_M][1] + 1 )!=(l_argv[LOCATION_K][0]))
		{
			printf("you can use option -k right after option -m\n");
			exit(1);
		}
	}

	//move argv to my_argv
	for(i=0; i<argc; i++)
	{
		for(j=0; j<strlen(argv[i]); j++)
			my_argv[i][j] = argv[i][j];
	}
	//sort [PID] or [KEY] if there is a option -r
	if(flags & PROC_OPT_R)
	{
		ascendingsort();
		if(flags & PROC_OPT_K)
			alphabetsort();
	}

	//if there is option -s, set s_flags
	if(flags & PROC_OPT_S)
	{
		for(i=0; i<(l_argv[LOCATION_S][1]-l_argv[LOCATION_S][0]); i++)
		{
			if(!strcmp(my_argv[l_argv[LOCATION_S][0]+1+i],"FILEDES"))
				s_flags |= S_FILEDES;
			else if(!strcmp(my_argv[l_argv[LOCATION_S][0]+1+i],"CMDLINE"))
				s_flags |= S_CMDLINE;
			else if(!strcmp(my_argv[l_argv[LOCATION_S][0]+1+i],"IO"))
				s_flags |= S_IO;
			else if(!strcmp(my_argv[l_argv[LOCATION_S][0]+1+i],"STAT"))
				s_flags |= S_STAT;
			else if(!strcmp(my_argv[l_argv[LOCATION_S][0]+1+i],"ENVIRON"))
				s_flags |= S_ENVIRON;
		}
	}

	int pid, status;
	//print start_string
	printf("%s",start_string);

	//option -o
	int temp_fd;
	if(flags & PROC_OPT_O)
	{
		int fd;
		temp_fd = dup(1);
		if((fd = open(my_argv[l_argv[LOCATION_O][1]],O_WRONLY | O_CREAT | O_TRUNC,0666)) <0)
		{
			perror("open");
			exit(1);
		}
		dup2(fd,1);
		printf("!--Successfully Redirected : %s--!\n",my_argv[l_argv[LOCATION_O][1]]);
	}
	

	//do option -f
	if(flags & PROC_OPT_F){

		pid = fork();
		if(pid<0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if((l_argv[LOCATION_F][1] - l_argv[LOCATION_F][0]) == 0 ) //if there is no variable factor
			{

				if(flags == PROC_OPT_F || flags == PROC_OPT_F + PROC_OPT_R || flags == PROC_OPT_F + PROC_OPT_O || flags == PROC_OPT_F + PROC_OPT_R + PROC_OPT_O)
					func_f(getpid(),0);
				else
					func_f(getpid(),1);
			}
			else if((l_argv[LOCATION_F][1] - l_argv[LOCATION_F][0]) == 1 ) //if there is one variable factor
			{
				if(flags == PROC_OPT_F || flags == PROC_OPT_F + PROC_OPT_R || flags == PROC_OPT_F + PROC_OPT_O || flags == PROC_OPT_F + PROC_OPT_R + PROC_OPT_O)
					func_f(atoi(my_argv[l_argv[LOCATION_F][1]]),0);
				else
					func_f(atoi(my_argv[l_argv[LOCATION_F][1]]),1);
			}
			else{ 
				plus_argv_num = l_argv[LOCATION_F][1] - l_argv[LOCATION_F][0];
				if(plus_argv_num > 16) //if variable factors exceed 16
				{
					for(i=17; i<= plus_argv_num; i++) 
						printf("Maximun Number of Argument Exceeded. :: %s\n",my_argv[l_argv[LOCATION_F][0]+i]);

					for(i=0; i<16; i++)
						func_f(atoi(my_argv[l_argv[LOCATION_F][0]+i+1]),1);
				}
				else{ //if variable factors not exceed 16
					for(i=0; i<(l_argv[LOCATION_F][1]-l_argv[LOCATION_F][0]); i++)
						func_f(atoi(my_argv[l_argv[LOCATION_F][0]+i+1]),1);
				}
				plus_argv_num = 0;
			}
			exit(0); 

		}else{
			wait(&status);
		}
	}

	//do option -t
	if(flags & PROC_OPT_T){
		pid = fork();
		if(pid <0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if((l_argv[LOCATION_T][1] - l_argv[LOCATION_T][0]) == 0 ){ //if there is no variable factor
				if(flags == PROC_OPT_T || flags == PROC_OPT_T + PROC_OPT_R || flags == PROC_OPT_T + PROC_OPT_O || flags == PROC_OPT_T + PROC_OPT_R + PROC_OPT_O)
					func_t(getpid(),0);
				else
					func_t(getpid(),1);
			}else if((l_argv[LOCATION_T][1] - l_argv[LOCATION_T][0]) == 1 ){ //if there is one variable factor
				if(flags == PROC_OPT_T || flags == PROC_OPT_T + PROC_OPT_R || flags == PROC_OPT_T + PROC_OPT_O || flags == PROC_OPT_T + PROC_OPT_R + PROC_OPT_O)
					func_t(atoi(my_argv[l_argv[LOCATION_T][1]]),0);
				else
					func_t(atoi(my_argv[l_argv[LOCATION_T][1]]),1);
			}else{
				plus_argv_num = l_argv[LOCATION_T][1] - l_argv[LOCATION_T][0];
				if(plus_argv_num > 16) //if variable factors exceed 16
				{
					for(i=17; i<= plus_argv_num; i++)
						printf("Maximun Number of Argument Exceeded. :: %s\n",my_argv[l_argv[LOCATION_T][0]+i]);

					for(i=0; i<16; i++)
						func_t(atoi(my_argv[l_argv[LOCATION_T][0]+i+1]),1);
				}else{ //if variable factors not exceed 16
					for(i=0; i<(l_argv[LOCATION_T][1]-l_argv[LOCATION_T][0]); i++)
						func_t(atoi(my_argv[l_argv[LOCATION_T][0]+i+1]),1);
				}
				plus_argv_num = 0;
			}
			exit(0);
		}else{
			wait(&status);
		}

	}

	//do option -c
	if(flags & PROC_OPT_C){
		pid = fork();
		if(pid<0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if((l_argv[LOCATION_C][1] - l_argv[LOCATION_C][0]) == 0 ){ //if there is no variable factor
				if(flags == PROC_OPT_C || flags == PROC_OPT_C + PROC_OPT_R || flags == PROC_OPT_C+PROC_OPT_O || flags == PROC_OPT_C + PROC_OPT_R + PROC_OPT_O)
					func_c(getpid(),0);
				else
					func_c(getpid(),1);
			}else if((l_argv[LOCATION_C][1] - l_argv[LOCATION_C][0]) == 1 ){ //if there is one variable factor
				if(flags == PROC_OPT_C || flags == PROC_OPT_C + PROC_OPT_R || flags == PROC_OPT_C + PROC_OPT_O || flags == PROC_OPT_C + PROC_OPT_R + PROC_OPT_O)
					func_c(atoi(my_argv[l_argv[LOCATION_C][1]]),0);
				else
					func_c(atoi(my_argv[l_argv[LOCATION_C][1]]),1);
			}else{
				plus_argv_num = l_argv[LOCATION_C][1] - l_argv[LOCATION_C][0];
				if(plus_argv_num > 16) //if variable factors exceed 16
				{
					for(i=17; i<= plus_argv_num; i++)
						printf("Maximun Number of Argument Exceeded. :: %s\n",my_argv[l_argv[LOCATION_C][0]+i]);

					for(i=0; i<16; i++)
						func_c(atoi(my_argv[l_argv[LOCATION_C][0]+i+1]),1);
				}else{ //if variable factors not exceed 16
					for(i=0; i<(l_argv[LOCATION_C][1]-l_argv[LOCATION_C][0]); i++)
						func_c(atoi(my_argv[l_argv[LOCATION_C][0]+i+1]),1);
				}
				plus_argv_num = 0;
			}
			exit(0);
		}else{
			wait(&status);
		}
	}

	//do option n
	if(flags & PROC_OPT_N){
		pid = fork();
		if(pid <0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if((l_argv[LOCATION_N][1] - l_argv[LOCATION_N][0]) == 0 ){ //if there is no variable factor
				if(flags == PROC_OPT_N || flags == PROC_OPT_N + PROC_OPT_R || flags == PROC_OPT_N + PROC_OPT_O || flags == PROC_OPT_N + PROC_OPT_R + PROC_OPT_O)
					func_n(getpid(),0);
				else
					func_n(getpid(),1);
			}else if((l_argv[LOCATION_N][1] - l_argv[LOCATION_N][0]) == 1 ){ //if there is one variable factor
				if(flags == PROC_OPT_N || flags == PROC_OPT_N + PROC_OPT_R || flags == PROC_OPT_N + PROC_OPT_O || flags == PROC_OPT_N + PROC_OPT_R + PROC_OPT_O)
					func_n(atoi(my_argv[l_argv[LOCATION_N][1]]),0);
				else
					func_n(atoi(my_argv[l_argv[LOCATION_N][1]]),1);
			}else{
				plus_argv_num = l_argv[LOCATION_N][1] - l_argv[LOCATION_N][0];
				if(plus_argv_num > 16) //if variable factors exceed 16
				{
					for(i=17; i<= plus_argv_num; i++)
						printf("Maximun Number of Argument Exceeded. :: %s\n",my_argv[l_argv[LOCATION_N][0]+i]);

					for(i=0; i<16; i++)
						func_n(atoi(my_argv[l_argv[LOCATION_N][0]+i+1]),1);
				}else{ //if variable factors not exceed 16
					for(i=0; i<(l_argv[LOCATION_N][1]-l_argv[LOCATION_N][0]); i++)
						func_n(atoi(my_argv[l_argv[LOCATION_N][0]+i+1]),1);

				}
				plus_argv_num = 0;
			}
			exit(0);
		}else{
			wait(&status);
		}
	}

	//option m
	if(flags & PROC_OPT_M){
		pid = fork();
		if(pid <0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if((l_argv[LOCATION_M][1] - l_argv[LOCATION_M][0]) == 0 ){ //if there is no variable factor
				if(flags == PROC_OPT_M || flags == PROC_OPT_M + PROC_OPT_O ||flags == PROC_OPT_M + PROC_OPT_R ||flags == PROC_OPT_M + PROC_OPT_O + PROC_OPT_R || flags == PROC_OPT_M+PROC_OPT_K || flags == PROC_OPT_M + PROC_OPT_K + PROC_OPT_R || flags == PROC_OPT_M + PROC_OPT_K + PROC_OPT_O || flags == PROC_OPT_M + PROC_OPT_K + PROC_OPT_R + PROC_OPT_O)
					func_m(getpid(),0);
				else
					func_m(getpid(),1);
			}else if((l_argv[LOCATION_M][1] - l_argv[LOCATION_M][0]) == 1 ){ //if there is one variable factor
				if(flags == PROC_OPT_M || flags == PROC_OPT_M+PROC_OPT_K || flags == PROC_OPT_M + PROC_OPT_K + PROC_OPT_R
						||flags == PROC_OPT_M + PROC_OPT_K + PROC_OPT_R + PROC_OPT_O)
					func_m(atoi(my_argv[l_argv[LOCATION_M][1]]),0);
				else
					func_m(atoi(my_argv[l_argv[LOCATION_M][1]]),1);
			}else{
				plus_argv_num = l_argv[LOCATION_M][1] - l_argv[LOCATION_M][0];
				if(plus_argv_num > 16) //if variable factors exceed 16
				{
					for(i=17; i<= plus_argv_num; i++)
						printf("Maximun Number of Argument Exceeded. :: %s\n",my_argv[l_argv[LOCATION_M][0]+i]);

					for(i=0; i<16; i++)
						func_m(atoi(my_argv[l_argv[LOCATION_M][0]+i+1]),1);
				}else{ //if variable factors not exceed 16
					for(i=0; i<(l_argv[LOCATION_M][1]-l_argv[LOCATION_M][0]); i++)
						func_m(atoi(my_argv[l_argv[LOCATION_M][0]+i+1]),1);

				}
				plus_argv_num = 0;
			}
			exit(0);
		}else{
			wait(&status);
		}
	}
	//do option -w
	if(flags & PROC_OPT_W){
		pid = fork();
		if(pid<0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if(flags == PROC_OPT_W || flags == PROC_OPT_W + PROC_OPT_R ||flags == PROC_OPT_W + PROC_OPT_O|| flags == PROC_OPT_W + PROC_OPT_R + PROC_OPT_O)
				func_w(0);
			else
				func_w(1);
			exit(0);
		}else{
			wait(&status);
		}
	}
	//do option -e
	if(flags & PROC_OPT_E){
		pid = fork();
		if(pid <0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if(flags == PROC_OPT_E || flags == PROC_OPT_E + PROC_OPT_R || flags == PROC_OPT_E + PROC_OPT_O || flags == PROC_OPT_E + PROC_OPT_R + PROC_OPT_O)
				func_e(0);
			else 
				func_e(1);
			exit(0);
		}else{
			wait(&status);
		}
	}
	//do option -l
	if(flags & PROC_OPT_L){
		pid = fork();
		if(pid <0){
			perror("fork");
			exit(1);
		}else if(pid ==0){
			if(flags == PROC_OPT_L || flags == PROC_OPT_L + PROC_OPT_R || flags == PROC_OPT_L + PROC_OPT_O || flags == PROC_OPT_L + PROC_OPT_R + PROC_OPT_O)
				func_l(0);
			else
				func_l(1);
			exit(0);
		}else{
			wait(&status);
		}
	}
	//do option -v
	if(flags & PROC_OPT_V){
		pid = fork();
		if(pid < 0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			if(flags == PROC_OPT_V || flags == PROC_OPT_V + PROC_OPT_R || flags == PROC_OPT_V + PROC_OPT_O || flags == PROC_OPT_V + PROC_OPT_R + PROC_OPT_O)
				func_v(0);
			else
				func_v(1);
			exit(0);
		}else{
			wait(&status);
		}
	}
	//do option -s
	if(flags & PROC_OPT_S){

		uid_t uid;
		uid = getuid();

		pid = fork();
		if(pid <0){
			perror("fork");
			exit(1);
		}else if(pid == 0){
			func_s(uid);			
			exit(0);
		}else{
			wait(&status);
		}
	}

	if(flags & PROC_OPT_O)
	{
		dup2(temp_fd,1);
	}
	//terminate
	printf("%s",terminate_string);
}
//check program can access to file , return 0 or 1
int can_access(char *path)
{
	int can=0; //can :0 can't: 1
	if(access(path,F_OK)==0)
	{
		if(access(path,R_OK) < 0) //if file can't be read then print message
		{
			can = 1;
			printf("%s can't be read.\n",path);
		}
	}else{ //if file doesn't exist then print message
		can = 1;
		printf("%s doesn't exist.\n",path);
	}

	return can;
}
//function for option -f
void func_f(int pid, int pid_num)
{
	char path[100]= {0};
	char link_path[100] = {0};
	sprintf(path,"/proc/%d/fd",pid);

	if(pid_num == 1)
		//	if(flags != PROC_OPT_S)
		printf("([%s])\n",path);

	if(pid_num == 3){ //if user use only -s option
		if((flags == PROC_OPT_S) || (flags == PROC_OPT_S + PROC_OPT_O))
			printf("## Attribute : FILEDES, Target Process ID : %d ##\n",pid);
		else{ //if user use option -s with other options
			printf("([%s])\n",path);
			printf("## Attribute : FILEDES, Target Process ID : %d ##\n",pid);
		}
	}

	int can; //check access permission
	can = can_access(path);
	if(can == 1)
		return;

	DIR *dirp = NULL;
	struct dirent *dentry;
	char link_buf[BUFF_SIZE] = {0};

	if((dirp = opendir(path)) == NULL)
	{
		perror("opendir");
		exit(1);
	}
	while((dentry = readdir(dirp)) != NULL)
	{
		if(strcmp(".",dentry -> d_name) == 0 || strcmp("..",dentry -> d_name) == 0)
			continue;

		printf("File Discriptor number: %s,",dentry -> d_name);
		sprintf(link_path,"/proc/%d/fd/%s",pid,dentry -> d_name);
		readlink(link_path,link_buf,sizeof(link_buf));
		printf("Opened File: %s\n",link_buf);

		memset(link_path,'\0',sizeof(link_path));
		memset(link_buf,'\0',sizeof(link_buf));
	}
	closedir(dirp);
}
//function for optoin -t
void func_t(int pid, int pid_num)
{
	FILE *fp;
	char read_buf[BUFF_SIZE] = {0};
	char path[100]={0};
	sprintf(path,"/proc/%d/status",pid);

	if(pid_num == 1) //check plus argv is 1 or more
		printf("([%s])\n",path);	

	if(pid_num == 3){
		if((flags == PROC_OPT_S) || (flags == PROC_OPT_S + PROC_OPT_O))
			printf("## Attribute : STAT, Target Process ID : %d ##\n",pid);
		else
		{
			printf("([%s])\n",path);	
			printf("## Attribute : STAT, Target Process ID : %d ##\n",pid);
		}
	}

	int can; //check access permission
	can = can_access(path);
	if(can == 1)
		return;

	if((fp = fopen(path, "r")) == NULL)
	{
		perror("open");
		exit(1);
	}
	int tab_cnt=0;
	int i,j;
	char tab_buf[100] = {0};
	for(i =0; i<10; i++)
	{
		fgets(read_buf,BUFF_SIZE,fp);
		if(i == 1)
			continue;
		if(i == 2){
			if(!strncmp(read_buf+7,"R",1))
				printf("State:  Running\n");
			else if(!strncmp(read_buf+7,"S",1))
				printf("State:  Sleeping in an interruptible wait\n");
			else if(!strncmp(read_buf+7,"D",1))
				printf("State:  Waiting in uninterruptible disk sleep\n");
			else if(!strncmp(read_buf+7,"Z",1))
				printf("State:  Zombie\n");
			else if(!strncmp(read_buf+7,"T",1))
				printf("State:  Stopped or trace stopped\n");
			else if(!strncmp(read_buf+7,"t",1))
				printf("State:  Tracing stop\n");
			else if(!strncmp(read_buf+7,"X",1))
				printf("State:  Dead\n");
		}else if(i == 8 || i == 9){
			tab_cnt=0;
			j=0;
			while(tab_cnt <= 1)
			{
				if(*(read_buf+j) =='\t')
					tab_cnt++;

				tab_buf[j] = *(read_buf + j);
				j++;
			}
			printf("%s\n",tab_buf);
			memset(tab_buf,'\0',sizeof(tab_buf));
		}else{
			printf("%s",read_buf);
		}
	}
}
//function for option -c
void func_c(int pid, int pid_num)
{
	FILE *fp;
	int fd;
	char read_buf[BUFF_SIZE] = {0};
	char path[100]={0};
	sprintf(path,"/proc/%d/cmdline",pid);

	if(pid_num == 1) //print path if pid_num == 1
		printf("([%s])\n",path);

	if(pid_num == 3)
	{
		if((flags == PROC_OPT_S) || (flags == PROC_OPT_S + PROC_OPT_O))
			printf("## Attribute : CMDLINE, Target Process ID : %d ##\n",pid);
		else
		{
			printf("([%s])\n",path);
			printf("## Attribute : CMDLINE, Target Process ID : %d ##\n",pid);
		}
	}
	int can; //check access permission
	can = can_access(path);
	if(can == 1)
		return;

	if((fd = open(path,O_RDONLY)) <0)
	{
		perror("open");
		exit(1);
	}
	int n;
	n = read(fd,read_buf,sizeof(read_buf));

	printf("argv[0] = ");
	int i,j=1;
	for(i=0; i<n; i++)
	{
		if(read_buf[i] == '\0'){ // parsing by '\0'
			if(read_buf[i+1] != '\0') //not end of cmdline file
			{
				printf("\n");
				printf("argv[%d] = ",j++);
			}
		}else
			printf("%c",read_buf[i]);

	}
	printf("\n");
	close(fd);
}
//function for option -n
void func_n(int pid, int pid_num)
{
	FILE *fp;
	char read_buf[BUFF_SIZE] = {0};
	char path[100]={0};
	sprintf(path,"/proc/%d/io",pid);

	if(pid_num == 1) //printf path if pid_num == 1
		printf("([%s])\n",path);

	if(pid_num ==3 ){
		if((flags == PROC_OPT_S) || (flags == PROC_OPT_S + PROC_OPT_O))
			printf("## Attribute : IO, Target Process ID : %d ##\n",pid);
		else
		{
			printf("([%s])\n",path);
			printf("## Attribute : IO, Target Process ID : %d ##\n",pid);
		}
	}
	int can; //check access permission
	can = can_access(path);
	if(can == 1)
		return;

	if((fp = fopen(path, "r")) == NULL)
	{
		perror("open");
		exit(1);
	}
	int i;
	for(i =0; i<7; i++)
	{
		fgets(read_buf,BUFF_SIZE,fp);
		if(i == 0) //print rchar
			printf("Characters read : %s",read_buf+7);
		else if(i == 1) //print wchar
			printf("Characters written : %s",read_buf+7);
		else if(i == 2) //print syscr
			printf("Read syscalls : %s",read_buf+7);
		else if(i == 3) //print syscw
			printf("Write syscalls : %s",read_buf+7);
		else if(i == 4) //print read_bytes
			printf("Byetes read : %s",read_buf+12);
		else if(i == 5) //print write_bytes
			printf("Byetes written : %s",read_buf+13);
		else if(i == 6) //print cancelled_write_bytes
			printf("Cancelled write bytes : %s",read_buf+23);
	}
	fclose(fp);
}
//function for option -m
void func_m(int pid, int pid_num)
{
	int fd;
	char read_buf[4096] = {0};
	char path[100]={0};
	sprintf(path,"/proc/%d/environ",pid);

	if(pid_num == 1) //print path if pid_num == 1
		printf("([%s])\n",path);

	if(pid_num == 3){
		if((flags == PROC_OPT_S) || (flags == PROC_OPT_S + PROC_OPT_O))
			printf("## Attribute : ENVIRON, Target Process ID : %d ##\n",pid);
		else
		{
			printf("([%s])\n",path);
			printf("## Attribute : ENVIRON, Target Process ID : %d ##\n",pid);
		}
	}

	int can; //check access permission
	can = can_access(path);
	if(can == 1)
		return;

	if((fd = open(path,O_RDONLY)) <0)
	{
		perror("open");
		exit(1);
	}
	int n;
	n = read(fd,read_buf,sizeof(read_buf));

	int i,j,k=0;
	if((pid_num != 3)&&(flags & PROC_OPT_K )&&(l_argv[LOCATION_K][1]-l_argv[LOCATION_K][0])>0){ //if there is -k option
		for(j=0; j<(l_argv[LOCATION_K][1]-l_argv[LOCATION_K][0]); j++)
		{
			for(i=0; i<n; i++)
			{
				if(!strncmp(my_argv[l_argv[LOCATION_K][0]+1+j],read_buf+i,strlen(my_argv[l_argv[LOCATION_K][0]+1+j])))
				{
					if((read_buf[i-1]) != '\0' || read_buf[i+strlen(my_argv[l_argv[LOCATION_K][0]+1+j])] != '=')
						continue;
					else
					{
						while(read_buf[i] != '\0') //print before meet '\0'
						{
							printf("%c",read_buf[i]);
							i++;
						}
						printf("\n");
					}
				}
			}
		}
	}else{ //if there is no -k option
		for(i=0; i<n; i++)
		{
			if(read_buf[i] == '\0') //parsing by '\0'
				printf("\n");
			else
				printf("%c",read_buf[i]);
		}
	}
	close(fd);
}
//function for option -w
void func_w(int pid_num)
{
	FILE *fp;
	char read_buf[1024] = {0};
	char path[100] = {0};
	sprintf(path,"/proc/interrupts");

	int cpu_num = 0;
	float sum = 0;
	char num[BUFF_SIZE] = {0};
	char title[10] = {0};

	if(pid_num == 1) //print path if pid_num == 1
		printf("([%s])\n",path);

	if((fp = fopen(path,"r")) == NULL)
	{
		perror("open");
		exit(1);
	}
	fgets(read_buf,1024,fp); //read cpu number
	int i=0;
	while(1)
	{
		if(*(read_buf+i) == ' ')
			i++;
		else if(*(read_buf+i)== 'C') //if there is 'C' increase cpu_num
		{
			cpu_num++;
			i++;
		}
		else if(*(read_buf+i)== '\n')
			break;
		else 
			i++;
	}
	printf("---Number of CPUs : %d---\n",cpu_num);
	printf("%15s : %s\n","[Average]","[Description]");
	int j=0;
	memset(read_buf,'\0',1024);
	while(1)
	{
		memset(read_buf,'\0',1024);
		if(fgets(read_buf,1024,fp) == NULL)
			break;

		sum =0;
		memset(num,'\0',BUFF_SIZE);
		memset(title,'\0',10);
		if( *read_buf >= 65 && *read_buf<=90) // if capital letter
		{
			i=0;
			while(*(read_buf + i) != ' ')
			{
				title[i] = *(read_buf + i);
				i++;
			}
			while(*(read_buf + i) != '\n')
			{
				if(*(read_buf + i) == ' ') //if blank, continue.
					i++;
				else if(*(read_buf + i) >= 48 && *(read_buf + i)<=57) //if digit, 
				{
					j=0;
					while(*(read_buf+i)!=' ')
					{
						num[j] = *(read_buf+i);
						i++;
						j++;
					}
					sum = sum + atoi(num);
					if((!strncmp(title,"ERR",3)) || (!strncmp(title,"MIS",3)))
					{
						printf("%15.3f : <%s> \n",sum/cpu_num,title); //divide sum by cpu_num
						break;
					}
				}else if((*(read_buf + i) >= 65 && *(read_buf +i ) <= 90) ||( *(read_buf + i) >= 97 && *(read_buf + i) <= 122)){
					printf("%15.3f : <%s> %s",sum/cpu_num,title,read_buf+i); //divide sum by cpu_num
					break;
				}
			}
		}
	}
	fclose(fp);
}
//function for option -e
void func_e(int pid_num)
{
	int i,check_five =0;
	FILE *fp;
	char read_buf[BUFF_SIZE] = {0};
	char path[100] = {0};
	sprintf(path, "/proc/filesystems");

	if(pid_num == 1) //print path if pid_num == 1
		printf("([%s])\n",path);

	if((fp  = fopen(path, "r")) == NULL)
	{
		perror("open");
		exit(1);
	}
	printf("<<Supported Filesystems>>\n");
	while(fgets(read_buf,BUFF_SIZE,fp) != NULL)
	{ 
		if(!strncmp(read_buf,"nodev",5)) //if it is nodev, continue
			continue;
		else
		{
			if(check_five != 0) //if check_five is not 5
				printf(", ");

			read_buf[strlen(read_buf)-1] = '\0';
			printf("%s",read_buf+1);
			check_five++;

			if(check_five == 5) //if check_five is 5 change line
			{
				printf("\n");
				check_five = 0;
			}			
		}
		memset(read_buf,'\0',BUFF_SIZE);
	}
	printf("\n");
}
//function for option -l
void func_l(int pid_num)
{
	int fd;
	char read_buf[BUFF_SIZE] = {0};
	char path[100] = {0};
	sprintf(path,"/proc/uptime");

	if(pid_num == 1)
		printf("([%s])\n",path);

	if((fd=open(path,O_RDONLY)) < 0)
	{
		perror("open");
		exit(1);
	}
	read(fd,read_buf,sizeof(read_buf));

	int check = 0;
	char *ptr;
	ptr = strtok(read_buf," ");
	while(ptr != NULL)
	{
		if(check == 0)
			printf("Process worked time : ");
		else if(check == 1)
		{
			printf("Process idle time : ");
			ptr[strlen(ptr)-1] = '\0';
		}

		printf("%s(sec)\n",ptr);
		ptr = strtok(NULL," ");
		check++;
	}
}
//function for option -v
void func_v(int pid_num)
{
	int fd;
	char read_buf[BUFF_SIZE] = {0};
	char path[100] = {0};
	sprintf(path,"/proc/version");

	if(pid_num == 1)
		printf("([%s])\n",path);

	if((fd = open(path, O_RDONLY))<0)
	{
		perror("open");
		exit(1);
	}
	read(fd,read_buf,sizeof(read_buf));
	read_buf[strlen(read_buf)-1]='\0';
	printf("%s\n",read_buf);
}
//function for option -s
void func_s(int uid)
{
	DIR *dirp = NULL;
	struct dirent *dentry;
	char path[100] = {0};
	char next_path[100] = {0};
	int i;
	sprintf(path,"/proc");

	if((dirp = opendir(path)) == NULL || chdir(path) == -1)
	{
		perror("opendir");
		exit(1);
	}
	while((dentry = readdir(dirp)) != NULL)
	{
		if(strcmp(".",dentry -> d_name) == 0 || strcmp("..",dentry -> d_name) == 0)
			continue;

		if(*(dentry -> d_name) >= 48 && *(dentry -> d_name) <= 57) //if filename start with number
		{
			memset(next_path, '\0', 100);
			sprintf(next_path,"/proc/%s/status",dentry -> d_name);
			if(access(next_path,R_OK) == 0) //if file is readable
			{
				if((same_uid(dentry -> d_name, next_path,uid)) == 0) //if uids are same
				{
					if(s_flags & S_FILEDES){
						func_f(atoi(dentry->d_name),3); //call func_f
					}if(s_flags & S_CMDLINE){
						func_c(atoi(dentry->d_name),3); //call func_c
					}if(s_flags & S_IO){
						func_n(atoi(dentry->d_name),3); //call func_n
					}if(s_flags & S_STAT){
						func_t(atoi(dentry->d_name),3); //call func_t
					}if(s_flags & S_ENVIRON){
						func_m(atoi(dentry->d_name),3); //call func_m
					}
				}else
					continue;
			}else
			{
				printf("continue\n");
				continue;
			}
		}
	}
	closedir(dirp);
}
int same_uid(char *d_name,char *next_path, int uid) //if uids are same return 0 else return 1
{
	FILE *fp;
	char read_buf[BUFF_SIZE] = {0};
	char uid_arr[100] = {0};

	if((fp = fopen(next_path,"r")) == NULL)
	{
		perror("open");
		exit(1);
	}
	int i,j=0;
	for(i =0; i<9; i++)
	{
		fgets(read_buf,BUFF_SIZE,fp);
		if(i == 8)
		{
			while(*(read_buf+5+j) != '\t')
			{
				uid_arr[j] = *(read_buf+5+j);
				j++;
			}
		}
	}

	fclose(fp);

	if(atoi(uid_arr) == uid) //if uids are same
		return 0;
	else
		return 1;
}
int arrange_option(int argc, char *argv[])
{
	int i=1;
	while((i<argc) == 1)
	{
		if(!strcmp(argv[i],"-f")){ //option -f
			flags |= PROC_OPT_F; //add to flags
			l_argv[LOCATION_F][0] = i; //remember option -f index in l_argv array
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_F][1]= i-1; //remember end index of variable factor
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_F][1]= i-1; //remember end index of variable factor
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-t")){ //option -t
			flags |= PROC_OPT_T;
			l_argv[LOCATION_T][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_T][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_T][1]= i-1;
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-c")){ //option -c
			flags |= PROC_OPT_C;
			l_argv[LOCATION_C][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_C][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_C][1]= i-1;
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-n")){ //option -n
			flags |= PROC_OPT_N;
			l_argv[LOCATION_N][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_N][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_N][1]= i-1;
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-m")){ //option -m
			flags |= PROC_OPT_M;
			l_argv[LOCATION_M][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_M][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_M][1]= i-1;
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-k")){ //option -k
			flags |= PROC_OPT_K;
			l_argv[LOCATION_K][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_K][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_K][1]= i-1;
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-w")){ //option -w
			flags |= PROC_OPT_W;
			i++;
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-e")){ //option -e
			flags |= PROC_OPT_E;
			i++;
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-l")){ //option -l
			flags |= PROC_OPT_L;
			i++;
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-v")){ //option -v
			flags |= PROC_OPT_V;
			i++;
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-r")){ //option -r
			flags |= PROC_OPT_R;
			i++;
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-s")){ //option -s
			flags |= PROC_OPT_S;
			l_argv[LOCATION_S][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_S][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_S][1]= i-1;
					break;
				}
				i++;
			}
			if((i<argc) == 0)
				break;
		}else if(!strcmp(argv[i],"-o")){ //option -o
			flags |= PROC_OPT_O;
			l_argv[LOCATION_O][0] = i;
			i++;
			while(1)
			{
				if((i<argc) == 0){
					l_argv[LOCATION_O][1]= i-1;
					break;
				}else if(!strncmp(argv[i],"-",1)){
					l_argv[LOCATION_O][1]= i-1;
					break;
				}
				i++;
			}
			//	l_argv[LOCATION_O][1] = i;
			//	i++;
			if((i<argc) == 0)
				break;
		}else //wrong option
			return 1;
	}
	return 0;

}
void ascendingsort()
{
	if(flags & PROC_OPT_F){ //if there is option -f
		if((l_argv[LOCATION_F][1] - l_argv[LOCATION_F][0]) > 1)
			my_ascendingsort((l_argv[LOCATION_F][1]-l_argv[LOCATION_F][0]),l_argv[LOCATION_F][0]+1);
	}
	if(flags & PROC_OPT_T){ //if there is option -t
		if((l_argv[LOCATION_T][1] - l_argv[LOCATION_T][0]) > 1)
			my_ascendingsort((l_argv[LOCATION_T][1]-l_argv[LOCATION_T][0]),l_argv[LOCATION_T][0]+1);
	}
	if(flags & PROC_OPT_C){ //if there is option -c
		if((l_argv[LOCATION_C][1] - l_argv[LOCATION_C][0]) > 1)
			my_ascendingsort((l_argv[LOCATION_C][1]-l_argv[LOCATION_C][0]),l_argv[LOCATION_C][0]+1);
	}
	if(flags & PROC_OPT_N){ //if there is option -n
		if((l_argv[LOCATION_N][1] - l_argv[LOCATION_N][0]) > 1)
			my_ascendingsort((l_argv[LOCATION_N][1]-l_argv[LOCATION_N][0]),l_argv[LOCATION_N][0]+1);
	}
	if(flags & PROC_OPT_M){ //if there is option -m
		if((l_argv[LOCATION_M][1] - l_argv[LOCATION_M][0]) > 1)
			my_ascendingsort((l_argv[LOCATION_M][1]-l_argv[LOCATION_M][0]),l_argv[LOCATION_M][0]+1);
	}

}
void my_ascendingsort(int num,int start) //num: number of sort, start: sort start point of my_argv
{
	int i,j;
	char temp_pid[100];
	if(num > 16)
		num = 16;

	for(i=0; i<num; i++)
	{
		for(j=start; j<start+num-1; j++)
		{
			if(atoi(my_argv[j]) > atoi(my_argv[j+1]))
			{
				strcpy(temp_pid,my_argv[j]);

				memset(my_argv[j],'\0',strlen(my_argv[j]));
				strcpy(my_argv[j],my_argv[j+1]);

				memset(my_argv[j+1],'\0',strlen(my_argv[j+1]));
				strcpy(my_argv[j+1],temp_pid);
				memset(temp_pid,'\0',100);
			}
		}
	}

}
void alphabetsort()
{
	int i,j;
	int num = l_argv[LOCATION_K][1] - l_argv[LOCATION_K][0];
	int start = l_argv[LOCATION_K][0] +1;
	char temp[100] = {0};
	for(i=0; i<num; i++)
	{
		for(j=start; j< start+num-1 ; j++)
		{
			if(strcmp(my_argv[j],my_argv[j+1]) > 0) //front < back
			{
				strcpy(temp,my_argv[j]);

				memset(my_argv[j],'\0',strlen(my_argv[j]));
				strcpy(my_argv[j],my_argv[j+1]);

				memset(my_argv[j+1],'\0',strlen(my_argv[j+1]));
				strcpy(my_argv[j+1],temp);

				memset(temp,'\0',100);
			}
		}

	}
}

