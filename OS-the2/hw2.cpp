#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include <string.h>
#include "monitor.h"
#include <fstream>
#include <iostream>
using namespace std; 

class ElevatorController:public Monitor {
	
	int current_floor;
	int state;
	int num_people;
	int num_floors;
	int weight_capacity;
	int person_capacity;
	int total_weight;
	int total_people;
	int *served_p;
	int idle_time;
	int in_out_time;
	int travel_time;
	int* ex;
	int* hi;
	int* lo;

	Condition waiting_people_high, waiting_people_low, can_exit, is_changed, waiting_elev,waiting_call;

	struct dest{
		int floor;
		dest *next;
	};

	dest *dest_q;
	
public:
	ElevatorController(int a , int b , int c , int d, int e, int f, int g):waiting_call(this),waiting_people_high(this), waiting_people_low(this),waiting_elev(this),is_changed(this),can_exit(this)	
	{
		dest_q = NULL;
		current_floor = 0;
		state = 0;
		total_weight = 0;
		total_people = 0; 
		num_floors = a;
		weight_capacity = b;
		person_capacity = c;
		num_people = d;
		idle_time = e;
		in_out_time = f;
		travel_time = g;
		ex = new int [num_floors];
		hi = new int [num_floors]; 
		lo = new int [num_floors]; 
		for(int i = 0 ; i < num_floors ; i++) ex[i] = 0;
		for(int i = 0 ; i < num_floors ; i++) hi[i] = 0;
		for(int i = 0 ; i < num_floors ; i++) lo[i] = 0; 
		served_p = new int [num_people];
		for(int i = 0 ; i < num_people ; i++) served_p[i] = 0;	
	}
	
	~ElevatorController() { 
		delete [] dest_q;
	}
	int getFloor(){ 
		__synchronized__ ;
		return current_floor;
	}
	int getState(){ 
		__synchronized__ ;
		return state ;
	}
	dest* getQueue(){
		__synchronized__ ;
		return dest_q;
	}
	int* getServed(){
		__synchronized__ ;
		return served_p; 
	}
	bool can_call(int i, int d){
		__synchronized__ ;
		int init = i;
		int dst = d;
		if(state != 0) return 0;
		else{
			if(dest_q == NULL) return 1;
			else{
				if(current_floor - dest_q->floor < 0){
					if(dst - init > 0 && init >= current_floor) return 1;
					else return 0;
				} 
				else{
					if(dst - init < 0 && init <= current_floor) return 1;
					else return 0;
				}
			}	
		}	
	}
	
	void insert_init(int i, int d, int id, int w, int pr){
	
		__synchronized__ ;
		int init = i;
		int dst = d;
		if(pr == 1) cout << "Person (" << id <<", hp, "<< init << " -> " << dst<<" ,"<<  w <<") made a request"<< endl;
		else cout << "Person (" << id <<", lp, "<< init << " -> " << dst<<" ,"<<  w <<") made a request"<< endl;
	
			if(pr == 1) hi[i]++;
			else lo[i]++;
		if(init != current_floor){
		if(state == 0){
			if(dest_q == NULL){
 		                dest* new_dest = new dest;
			      	new_dest -> floor = init;
				new_dest -> next = NULL; 
				dest_q = new_dest;
			}
			else if(init >= current_floor && (dest_q->floor - current_floor)>=0){
				dest* tmp = dest_q;
				while((tmp -> next != NULL) &&  (init >= tmp -> next -> floor)) tmp = tmp -> next;
				if(tmp->floor != init){
					dest* new_dest = new dest;
					new_dest -> floor = init;
					new_dest -> next = NULL;
					if(init > dest_q -> floor){
						new_dest -> next = tmp -> next;
						tmp -> next = new_dest;
					}
					else if(init < dest_q -> floor){
						new_dest -> next = dest_q;
						dest_q = new_dest;
					}
				}
		
			}
			else if(init <= current_floor && (dest_q->floor - current_floor)<=0){
				dest* tmp = dest_q;
				while((tmp -> next != NULL) &&  (init <= tmp -> next -> floor)) tmp = tmp -> next;
				if(tmp->floor != init){ 
					dest* new_dest = new dest;
					new_dest -> floor = init;
					new_dest -> next = NULL;
					if(init < dest_q -> floor){
						new_dest -> next = tmp -> next;
						tmp -> next = new_dest;
					}
					else if(init > dest_q -> floor){
						new_dest -> next = dest_q;
						dest_q = new_dest;
					}
				}	
			} 
			
	   	}
		else if(state == 1 && dst - init > 0 && init > current_floor){
			dest* tmp = dest_q;
			while((tmp -> next != NULL) &&  (init >= tmp -> next -> floor)) tmp = tmp -> next;
			if(tmp->floor != init){
				dest* new_dest = new dest;
				new_dest -> floor = init;
				new_dest -> next = NULL;
				if(init > dest_q -> floor){
					new_dest -> next = tmp -> next;
					tmp -> next = new_dest;
				}
				else if(init < dest_q -> floor){
					new_dest -> next = dest_q;
					dest_q = new_dest;
				}
			}
		}
		else if(state == 2 && dst - init < 0 && init < current_floor){
			dest* tmp = dest_q;
			while((tmp -> next != NULL) &&  (init <= tmp -> next -> floor)) tmp = tmp -> next;
			if(tmp->floor != init){
				dest* new_dest = new dest;
				new_dest -> floor = init;
				new_dest -> next = NULL;
				if(init < dest_q -> floor){
					new_dest -> next = tmp -> next;
					tmp -> next = new_dest;
				}
				else if(init > dest_q -> floor){
					new_dest -> next = dest_q;
					dest_q = new_dest;
				}
			}
		}
		}
		if(dest_q == NULL){
			if(dst > current_floor) cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> " << dst << ")" <<endl;
			else cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> " << dst << ")" <<endl;
		}
		else{
			if(dest_q->floor > current_floor){
				cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
				print_queue();
				cout << ")" << endl;
			}
			else if(dest_q->floor < current_floor){
				cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
				print_queue();
				cout << ")" << endl;
			}
		}
	
		
	}
	void insert_dest(int i, int d){	
		int init = i;
		int dst = d;
		ex[dst]++;
		if(dest_q == NULL){
			dest* new_dest = new dest;
			new_dest -> floor = dst;
			new_dest -> next = NULL;
			dest_q = new_dest;
		}
		else if(dest_q->floor - current_floor > 0){
			dest* tmp = dest_q;
			while((tmp -> next != NULL) &&  (dst >= tmp -> next -> floor)) tmp = tmp -> next;
			if(tmp->floor != dst){
				dest* new_dest = new dest;
				new_dest -> floor = dst;
				new_dest -> next = NULL;
			        if(dst > dest_q->floor){	
					new_dest -> next = tmp -> next;
					tmp -> next = new_dest;
				}
				else if(dst < dest_q->floor){
					new_dest -> next = dest_q;
					dest_q = new_dest;
				}
			}
		}
		else if(dest_q->floor - current_floor < 0){
			dest* tmp = dest_q;
			while((tmp -> next != NULL) &&  (dst <= tmp -> next -> floor)) tmp = tmp -> next;
			if(tmp->floor != dst){
				dest* new_dest = new dest;
				new_dest -> floor = dst;
				new_dest -> next = NULL;
			        if(dst < dest_q->floor){	
					new_dest -> next = tmp -> next;  
					tmp -> next = new_dest;
			        }
				else if(dst > dest_q->floor){
					new_dest -> next = dest_q;
					dest_q = new_dest;
				}
			}
		}	
	}
	void remove_dst(int a){
		dest* tmp = dest_q;
		if(tmp->floor == a){
			dest_q = tmp->next;
			tmp->next = NULL;
			delete tmp;
		}
	}
	
	bool all_served(int *arr){
		__synchronized__ ;
		for(int i = 0 ; i < num_people ; i++){
			if(arr[i] == 0) return 0;
		}
		return 1;
	}
	
	void elev_idle(){
		__synchronized__ ;
		waiting_elev.wait_specific(idle_time);
	}
	
	void move_upwards(){
		__synchronized__ ;
		waiting_elev.wait_specific(travel_time);
		current_floor++;
		if(dest_q->floor != current_floor){
			state = 1;
			cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
	        	print_queue();
			cout << ")" <<endl;	
		}
		else{ 
			state = 0;
			remove_dst(dest_q->floor); 
			if(dest_q != NULL){
				if(dest_q->floor > current_floor) {
					cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
					print_queue();
					cout << ")" <<endl;
				}
				else {
					cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
					print_queue();
					cout << ")" <<endl;
				}
			}
			else if(dest_q==NULL){
				cout << "Elevator (Idle, " << total_weight << ", " << total_people << ", " << current_floor << " ->"; 
				print_queue();
				cout << ")" <<endl;

			}

			if(ex[current_floor] > 0) can_exit.notifyAll();
			else if(hi[current_floor] > 0) waiting_people_high.notifyAll();
			else if(lo[current_floor] > 0) waiting_people_low.notifyAll();
			waiting_elev.wait_specific(in_out_time);
		}
		
	}
	void move_downwards(){
		__synchronized__ ;
		waiting_elev.wait_specific(travel_time);
		current_floor--;
		if(dest_q->floor != current_floor){
			state = 2;
			cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> "; 
			print_queue();
			cout<< ")"<< endl;
		}

		else{
			state = 0;
			remove_dst(dest_q->floor);
			if(dest_q != NULL){
				if(dest_q->floor > current_floor){
					cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
					print_queue();
					cout << ")" <<endl;
				}	
				else {
					cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
					print_queue();
					cout << ")" <<endl;
				}
			}
			else if(dest_q==NULL){
				cout << "Elevator (Idle, " << total_weight << ", " << total_people << ", " << current_floor << " ->";
				cout << ")" <<endl;
			}		
			if(ex[current_floor] > 0) can_exit.notifyAll();
			else if(hi[current_floor] > 0) waiting_people_high.notifyAll();
			else if(lo[current_floor] > 0) waiting_people_low.notifyAll();
			waiting_elev.wait_specific(in_out_time);
		}
	}
	void print_queue(){
		if(dest_q != NULL){ 
			dest* tm = dest_q;
			if(tm->floor!=current_floor){
				cout << tm->floor;
				tm = tm->next;
				while(tm != NULL) {
					cout << "," << tm->floor;
					tm = tm->next;
				}
			}
			else{
				tm = tm->next;
				if(tm != NULL){ 
					cout << tm->floor;
					tm = tm->next;
				
				while(tm != NULL) {
					cout << "," << tm->floor;
					tm = tm->next;
				}
				}
				else cout << "";
			}
		}
		else cout << "";
	}
	void get_in(int in, int des, int pr, int id, int w, int p){
		__synchronized__ ;
		if(p == -1){
			if(pr == 1) cout << "Person (" << id <<", hp, "<< in << " -> " << des<<" ,"<<  w <<") made a request"<< endl;
			else cout << "Person (" << id <<", lp, "<< in << " -> " << des<<" ,"<<  w <<") made a request"<< endl;
			if(dest_q == NULL) cout << "Elevator (Idle, " << total_weight << ", " << total_people << ", " << current_floor << " ->)"<<endl ;
			else{
				if(dest_q->floor > current_floor){
					cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
					print_queue();
					cout<< ")"<< endl;
				}
				else{
					cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
					print_queue();
					cout<< ")"<< endl;
				}
			}	
		}
		insert_dest(in, des);
		total_weight += w;
		total_people++;
		if(pr == 1) {
			cout << "Person (" << id <<", hp, "<< in << " -> " << des <<" ,"<< w <<") entered the elevator"<<endl;
			hi[current_floor]--;
		}
			
		else{
			cout << "Person (" << id <<", lp, "<< in << " -> " << des <<" ,"<< w <<") entered the elevator"<<endl;
			lo[current_floor]--;
		}
			
		if(dest_q->floor > current_floor){
			cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
			print_queue();
			cout<< ")"<< endl;
		}
		else{
			cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
			print_queue();
			cout<< ")"<< endl;
		}
		
		if(hi[current_floor] == 0 && lo[current_floor] > 0) waiting_people_low.notifyAll();
		else if (hi[current_floor] == 0 && lo[current_floor] == 0) waiting_call.notifyAll(); 

	}
	void get_out(int in, int des, int pr, int id, int w){
		__synchronized__ ;
		total_weight -= w;
	 	total_people--;
		served_p[id] = 1;
		ex[current_floor]--;
		if(pr == 1) cout << "Person (" << id <<", hp, "<< in << " -> " << des <<" ,"<< w <<") has left the elevator"<<endl;
		else cout << "Person (" << id <<", lp, "<< in << " -> " << des <<" ,"<< w <<") has left the elevator"<<endl;
		if(dest_q == NULL){
		       	state = 0;
			cout << "Elevator (Idle, " << total_weight << ", " << total_people << ", " << current_floor << " ->)"<<endl ;
		}
		else{
			if(dest_q->floor > current_floor){
				cout << "Elevator (Moving-up, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
				print_queue();
				cout<< ")"<< endl;
			}
			else{
				cout << "Elevator (Moving-down, " << total_weight << ", " << total_people << ", " << current_floor << " -> ";
				print_queue();
				cout<< ")"<< endl;
			}
		}
		if(ex[current_floor] == 0){ 
			if(hi[current_floor] > 0 ) waiting_people_high.notifyAll();
			else if(lo[current_floor] > 0 ) waiting_people_low.notifyAll();
			else waiting_call.notifyAll();
		}
	}
	void set_state(){
		__synchronized__ ;
		state = 0;
	}
	void wait_high(){
		__synchronized__ ;
		waiting_people_high.wait();
	}
	void wait_low(){
		__synchronized__ ;
		waiting_people_low.wait();
	}
	void wait_exit(){
		__synchronized__ ;	
		can_exit.wait();
	}
	void wait_call(){
		__synchronized__ ;
		waiting_call.wait();
	}
	void notify_call(){
		__synchronized__ ;
		waiting_call.notifyAll();
	}
	void notify_high(){
		__synchronized__ ;
		waiting_people_high.notifyAll();
	}
	void notify_low(){  
		__synchronized__ ;
		waiting_people_low.notifyAll();
	}
	int* hi_arr(){
		__synchronized__ ;
		return hi;
	}
	int* lo_arr(){
		__synchronized__ ; 
		return lo;
	}
	int* ex_arr(){
	__synchronized__ ;
		return ex;
}
	bool can_in(int w){
		__synchronized__ ;
		if(total_weight + w <= weight_capacity && total_people < person_capacity) return 1;
		else return 0;
	}
};

struct EParam{
	ElevatorController *ec;
	int person_id;
	int weight_person ;
	int initial_floor ;
	int destination_floor ;
	int priority ;
};
void *people_func(void *p){
	EParam *per = (EParam*) p;
	ElevatorController *ec = per -> ec;
	int past = 1;
	while(1){
begin:
		if(ec->can_call(per->initial_floor, per->destination_floor)){
			ec->insert_init(per->initial_floor, per->destination_floor, per->person_id, per->weight_person ,per->priority);
		notyet:
			if(per->initial_floor != ec->getFloor()){
				if(per->priority == 1) ec->wait_high();
				else ec->wait_low();
		}
			if(ec->getFloor() == per->initial_floor){
				if(ec->can_call(per->initial_floor, per->destination_floor)){
					if(ec->can_in(per->weight_person)){
					       	ec->get_in(per->initial_floor, per->destination_floor, per->priority, per->person_id, per->weight_person, past);
					}
					else{
						if(per->priority == 1) ec->hi_arr()[per->initial_floor]--;
						else ec->lo_arr()[per->initial_floor]--;
						past = -1;
				       		goto change;
					}
				}
				else{
					if(per->priority == 1) ec->hi_arr()[per->initial_floor]--; 
					else ec->lo_arr()[per->initial_floor]--;
					past = -1; 
					goto change; 
				}
			}
			else goto notyet;
		hooop:
			ec->wait_exit();
			if(ec->getFloor() == per->destination_floor) ec->get_out(per->initial_floor, per->destination_floor, per->priority, per->person_id, per->weight_person);
			else goto hooop; 
			break;
		}
	change:
		ec->wait_call();
		goto begin;
	}	
}
void *elev_func(void *p){
	ElevatorController *elev = (ElevatorController*) p;
	while(!(elev->all_served(elev->getServed()))){
		if(elev->getState() == 0){
			while(elev->getQueue() == NULL){
				elev->elev_idle();
				if((elev->all_served(elev->getServed()))) break;
			}
			if((elev->all_served(elev->getServed()))) break;
			if(elev->getFloor() < elev->getQueue()->floor){
				elev->elev_idle();
				elev->move_upwards();
			}
			else if(elev->getFloor() > elev->getQueue()->floor){
				elev->elev_idle();
				elev->move_downwards();
			} 			
		}
		else if(elev->getState() == 1) elev->move_upwards();
		else if(elev->getState() == 2) elev->move_downwards(); 
	}	
}

int main(int argc, char *argv[]){

pthread_t *people_th , elev_th;
std::string const & input = argv[1];
ifstream file(input);

int num_floors, num_people, weight_capacity, person_capacity, travel_time, idle_time, in_out_time;
file >> num_floors >> num_people >> weight_capacity >> person_capacity >> travel_time >> idle_time >> in_out_time;
int** arr_people = (int**)calloc(num_people , sizeof(int*));
for(int i = 0 ; i < num_people ; i++) arr_people[i] = (int*)calloc(4 , sizeof(int));
for(int i = 0 ; i < num_people ; i++) file >> arr_people[i][0] >> arr_people[i][1] >> arr_people[i][2] >> arr_people[i][3] ;
EParam *eparams = new EParam[num_people];
ElevatorController ec(num_floors, weight_capacity, person_capacity, num_people, idle_time, in_out_time, travel_time);
people_th = new pthread_t[num_people];
pthread_create(&elev_th , NULL, elev_func , (void *) &ec);
for(int i = 0 ; i < num_people ; i++){
	eparams[i].person_id = i ;
	eparams[i].weight_person = arr_people[i][0] ;
	eparams[i].initial_floor = arr_people[i][1] ;
	eparams[i].destination_floor = arr_people[i][2] ;
	eparams[i].priority = arr_people[i][3] ;
	eparams[i].ec = &ec;
	pthread_create(&people_th[i], NULL, people_func , (void *) (eparams + i));
}
for(int j = 0 ; j < num_people ;j++) pthread_join(people_th[j], NULL);
pthread_join(elev_th,NULL);

delete [] eparams;
delete [] people_th;
return 0;
	  
}



