#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"message.h"
#include"logging.c"
#include"logging.h"
#include<sys/wait.h>
#include<sys/poll.h>
#include<sys/time.h>
#ifdef SYSVR4
#define PIPE(a) pipe(a)
#else
#include<sys/socket.h>
#define PIPE(a) socketpair(AF_UNIX, SOCK_STREAM,PF_UNIX,(a))
#endif

int main(){

int p,r,t,k,no_of_bidders,start_bid,min_incr;
scanf("%d %d %d",&start_bid , &min_incr , &no_of_bidders);
char arr_bidders[no_of_bidders][150];
int no_arr_arg[no_of_bidders];
char listof_arg[no_of_bidders][80][80];
int child_arr[no_of_bidders];
char** updated;
for(int i = 0 ; i < no_of_bidders ; i++){
	scanf("%s %d", arr_bidders[i], &no_arr_arg[i]);
	for(int j = 0 ; j < no_arr_arg[i] ; j++) scanf("%s",listof_arg[i][j]);
}

int arr_fd[2*no_of_bidders];
for(int i = 0; i < no_of_bidders ; i ++){
	PIPE(arr_fd + 2*i);
}

for(int i = 0 ; i < no_of_bidders*2 ; i+=2){
	p = fork();
	if(p){
	       	close (arr_fd[i+1]);
		child_arr[i/2] = p;
	}
	else{
		close(arr_fd[i]);
		dup2(arr_fd[i+1],0);
		dup2(arr_fd[i+1],1);
		close(arr_fd[i+1]);
		int final_size = no_arr_arg[i/2];
		updated = (char**)calloc(final_size + 2 , sizeof(char*));
		for(int j = 0; j < final_size + 2 ; j++) updated[j] = (char*)calloc(25 , sizeof(char));
		strcpy(updated[0], arr_bidders[i/2]+2);
		for(t = 1 , k = 0; t < final_size + 1 ; t++ , k++) strcpy(updated[t] , listof_arg[i/2][k]);
		updated[t] = NULL;
		char path_store[120];
	        char* bid_store = (char*)calloc(25,sizeof(char));
		strcpy(bid_store, arr_bidders[i/2] + 2);
		strcpy(path_store,"/mnt/c/Users/USER/Desktop/OS/OS-the1/bin/" );
	        strcat(path_store , bid_store);	
		execv(path_store,updated);

	}
}
struct pollfd poll_fd[no_of_bidders];
for(int i = 0 ; i < no_of_bidders ; i++){
	poll_fd[i].fd = arr_fd[2*i];
	poll_fd[i].events = POLLIN;
	poll_fd[i].revents = 0;
}
int current_high[2] = {0,0};
cm buyer;
sm response;
oi out_info;
ii in_info;
int arr_stat[no_of_bidders];
while(1){
	int check = 0;
	for(int j = 0 ; j < no_of_bidders ; j++) if (poll_fd[j].fd>=0) check = 1;
	if (check == 0) break;
	poll(poll_fd , no_of_bidders , 0);
	for(int i = 0; i< no_of_bidders ; i++){
		if(poll_fd[i].revents && POLLIN) {
			r = read(poll_fd[i].fd , &buyer , sizeof(cm));
			in_info.type = buyer.message_id;
			in_info.pid = child_arr[i];
			in_info.info = buyer.params;
			print_input(&in_info , (i+1)*5);
			switch(buyer.message_id){
				case 1:
					response.message_id = 1;
					response.params.start_info.client_id = (i+1)*5;
					response.params.start_info.starting_bid = start_bid;
					response.params.start_info.current_bid = current_high[0];
					response.params.start_info.minimum_increment = min_incr;
					write(arr_fd[2*i] , &response , sizeof(sm));
					break;
				case 2:
					response.message_id = 2;
					if(buyer.params.bid < start_bid) response.params.result_info.result = 1;
					else if(buyer.params.bid < current_high[0]) response.params.result_info.result = 2;
					else if(buyer.params.bid - current_high[0] < min_incr) response.params.result_info.result = 3;
					else{
						response.params.result_info.result = 0;
						current_high[0] = buyer.params.bid;
						current_high[1] = (i+1)*5;
						response.params.result_info.current_bid = current_high[0];
					}
					write(arr_fd[2*i] , &response , sizeof(sm));
					break;
				case 3:
					response.message_id = 3;
					arr_stat[i] = buyer.params.status;
					poll_fd[i].fd = -5;
					break;
				default:
				       	break;
			
				
			}
			out_info.type = response.message_id;
			out_info.pid = child_arr[i];
			out_info.info = response.params;
			if(out_info.type != 3) print_output(&out_info , (i+1)*5);

		}
	}
}

response.params.winner_info.winner_id = current_high[1];
response.params.winner_info.winning_bid = current_high[0];
print_server_finished(current_high[1], current_high[0]);
for(int i = 0; i < no_of_bidders ; i++){
	out_info.type = 3;
	out_info.pid = child_arr[i];
	out_info.info = response.params;
	print_output(&out_info , (i+1)*5);
}
for(int i = 0 ; i < no_of_bidders ; i++) write(arr_fd[2*i] , &response , sizeof(sm));

for(int i = 0 ; i < no_of_bidders ; i++ ) {
	int status;
	pid_t w_p = waitpid(child_arr[i] , &status , 0);
	if (status == arr_stat[i]) print_client_finished((i+1)*5, status, 1);
}
for(int i = 0 ; i < no_of_bidders*2 ; i+=2) close(arr_fd[i]);

return 0;
}
