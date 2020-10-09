#include "servcli.h"
using namespace std;

string addr = "tcp://localhost:";

struct List {
	pid_t pid;
	pid_t ch_pid;
	int id;
	int ch_id;
	int port;
	int ch_port;
	void* socket;
};

List node;
pid_t add_node(Message *s2c);
Message* send_task(Message *s2c, void* socket);

int main(int argc, char const *argv[]){
	if(argc == 4){
		addr += argv[1];
		node.id = atoi(argv[2]);
	}
	else {
		cout<<"Wrong input"<<endl;
		return 1;
	}
	node.ch_id = -2;
	node.port = atoi(argv[3]);
	node.pid = getpid();
	void* context = zmq_ctx_new();
	node.socket = zmq_socket(context, ZMQ_REP);
	string newaddr = "tcp://*:" + to_string(node.port);
	zmq_bind(node.socket, newaddr.c_str());
	
	void* socket = zmq_socket(context, ZMQ_REQ);
	zmq_connect(socket, addr.c_str());
	zmq_msg_t in, out;
	Message *srv2clt, clt2srv;
	pid_t pid;

	clt2srv.pid = node.pid;
	clt2srv.id = node.id;
	clt2srv.port = node.port;
	clt2srv.status = 0;
	send_message(socket, &clt2srv);
	cout << addr << " ..... id_" << node.id << " ..... pid = " << getpid() << endl;

	bool loop = true, sent;
	while(loop){
		sent = false;
		//cout << "waiting message  " << node.pid << endl;
		srv2clt = receive_message(socket, &in);
		switch(srv2clt->task){
			case 1: // create
				if(srv2clt->p_id == node.id){
					pid = add_node(srv2clt);
					clt2srv.pid = node.ch_pid;
					clt2srv.id = node.ch_id;
					clt2srv.port = node.ch_port;
					clt2srv.status = 0;
					break;
				} else {
					if(srv2clt->p_id == -1 && srv2clt->port > 0){
						add_node(srv2clt);
						clt2srv.status = 1;
						break;
					} else {
						if(node.ch_id != -2 && srv2clt->port > 0) {
							Message *up = send_task(srv2clt, node.socket);
							send_message(socket, up);
							sent = true;
							break;
						} else {
							clt2srv.status = 2;
							break;
						}
					}
				}
			case 2: // remove
				if(srv2clt->id != node.id && srv2clt->status == 0) {
					if(node.ch_id != -2){
						Message *up = send_task(srv2clt, node.socket);
						send_message(socket, up);
						sent = true;
					} else clt2srv.status = 1;
				} else {
					srv2clt->status = -1;
					if(node.ch_id != -2){
						Message *up = send_task(srv2clt, node.socket);
						send_message(socket, up);
						sent = true;
					} else {
						clt2srv.status = 0;
						loop = false;
					}
				}
				break;
			case 3: // exec
			case 4: // pingall
				if(node.ch_id != -2){
					Message *up = send_task(srv2clt, node.socket);
					send_message(socket, up);
				} else {
					clt2srv.task = node.id;
					clt2srv.status = 0;
				}
			case 0: // exit
				if(node.ch_id != -2) send_task(srv2clt, node.socket);
				sent = false;
				loop = false;
				clt2srv.status = 0;
				break;
		}
		zmq_msg_close(&in);
		if(!sent) send_message(socket, &clt2srv);
	}
	zmq_close(socket);
	zmq_close(node.socket);
	zmq_ctx_destroy(context);
	exit(0);
	return 0;
}

Message* send_task(Message *s2c, void* socket){
	zmq_msg_t in;
	send_message(socket, s2c);
	Message *up = receive_message(socket, &in);
	zmq_msg_close(&in);
	return up;
}

pid_t add_node(Message *s2c){
	pid_t pid;
	List n;
	n.id = s2c->id;
	if(s2c->p_id != node.id) n.port = abs(s2c->port);
	else n.port = node.port;
	string arg1 = to_string(n.port), arg2 = to_string(n.id), arg3 = to_string(s2c->status);
	pid = fork();
	if(pid == 0) {
		char *args[] = {const_cast<char*>("child"), const_cast<char*>(arg1.c_str()), const_cast<char*>(arg2.c_str()), const_cast<char*>(arg3.c_str()), NULL};
		execv("./cli.out", args);
	} else if(pid > 0) {
		if(s2c->p_id == node.id) {
			cout << "--- Connecting id_" << n.id << " to id_" << node.id << " by port " << n.port << endl;
			zmq_msg_t in;
			Message *m = receive_message(node.socket, &in);
			pid = m->pid;
			cout << "--- Connected node's pid = " << pid << endl << endl;
			node.ch_id = n.id;
			node.ch_pid = pid;
			node.ch_port = m->port;
			zmq_msg_close(&in);
			return pid;
		} else {
			cout << "--- Connecting id_" << n.id << " to id_" << s2c->p_id << " by port " << n.port << endl;
			cout << "--- Connected node's pid = " << pid << endl << endl;
			return pid;
		}
	} 
}
