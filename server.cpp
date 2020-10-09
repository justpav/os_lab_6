#include <algorithm>
#include "servcli.h"
using namespace std;

int port_gen(vector<bool>& ports){
	auto free = find(ports.begin(), ports.end(), false);
	int index = distance(ports.begin(), free);
	ports[index] = true;
	return index;
}

typedef struct {
	int id;
	int port;
} Leaf;

void print_menu(){
	cout << "create [parent id] [child id]" << endl
	<< "remove [id]" << endl
	<< "exec [id] [n] [k1, k2, ...]" << endl
	<< "pingall" << endl
	<< "ids" << endl
	<< "menu" << endl
	<< "exit" << endl;
}

int main(int argc, char const *argv[])
{
	if(argc != 2) {
		cout<<"Error: wrong input"<<endl;
		return 1;
	}
	void* context = zmq_ctx_new();
	int status = 0, id, p_id, n, k, startport = atoi(argv[1]);
	bool loop = true, send = false;
	Message srv2clt, *clt2srv;
	vector<int> numbers, ids;
	vector<bool> ports(100, false);
	vector<Leaf> leaves;
	vector<void*> sockets;
	vector<vector<int>> tree;
	// map<int, void*> sockets;
	// map<int, void*> :: iterator it;
	zmq_msg_t out, in;
	string command, addr = "tcp://*:";
	Leaf leaf;

	init_message(&srv2clt);
	//init_message(clt2srv);
	ports[0] = true;
	leaf.id = -1;
	leaf.port = startport;
	leaves.push_back(leaf);

	sockets.push_back(zmq_socket(context, ZMQ_REP));
	addr += argv[1];
	zmq_bind(sockets[0], addr.c_str());
	cout << "Control node's address is " << addr.c_str() << endl;
	clt2srv = receive_message(sockets[0], &in);
	zmq_msg_close(&in);
	cout << "Connected pid = " << clt2srv->pid << endl;
	ports[1] = true;
	ids.push_back(clt2srv->id);
	leaf.id = clt2srv->id;
	leaf.port = clt2srv->port;
	leaves.push_back(leaf);

	print_menu();
	while(loop) {
		cout << '>';
		cin >> command;
		if(command == "create") {
			cin >> id;
			auto id_it = find(ids.begin(), ids.end(), id);
			if(id_it == ids.end()) {
				if(id > 0) {
					cin >> p_id;
					int i;
					for(i = 0; i < leaves.size(); i++){ if (leaves[i].id == p_id) break;}
					if(i != leaves.size()) {
						srv2clt.id = id;
						srv2clt.p_id = p_id;
						srv2clt.task = 1;
						if(p_id != -1) srv2clt.port = leaves[i].port;
						else srv2clt.port = startport + port_gen(ports);
						srv2clt.status = startport + port_gen(ports);
						send = true;
						if(p_id == -1) {
							sockets.push_back(zmq_socket(context, ZMQ_REP));
							string ad = "tcp://*:" + to_string(srv2clt.port);
							zmq_bind(sockets[sockets.size() - 1], ad.c_str());
						}
					} else { cout << "Error: parent not found" << endl; }
				} else { cout << "Error: invalid child id" << endl; }
			} else { cout << "Error: already exists" << endl; }

		} else if(command == "remove") {
			cin >> id;
			if(id > 0) {
				for(int i = 0; i < ids.size(); i++)
					if(ids[i] == id) ids.erase(ids.begin() + i);
				srv2clt.task = 2;
				srv2clt.id = id;
				srv2clt.status = 0;
				send = true;
			}else { cout << "Error: invalid id" << endl; }

		} else if(command == "exec") {
			cin >> id;
			auto id_it = find(ids.begin(), ids.end(), id);
			if(id_it != ids.end()) {
				cin >> n;
				for(int i = 0; i < n; i++) {
					cin >> k;
					numbers.push_back(k); // нужно передать данные процессу (file-mapping)
				}
			} else { cout << "Error: invalid id" << endl; }

		} else if(command == "pingall") {
			srv2clt.task = 3;
			send = true;

		} else if(command == "ids") {
			for(int i : ids) cout << "[" << i << "] ";
			cout << endl;

		} else if(command == "menu") {
			print_menu();

		} else if(command == "exit") {
			srv2clt.task = 0;
			send = true;

		} else { cout << "Error: incorrect command" << endl; }

		if(send) {
			for(auto item : sockets) {
				if(p_id != -1 || item != sockets[sockets.size() - 1]) send_message(item, &srv2clt);
				if(p_id == -1 && srv2clt.port > 0) srv2clt.port *= -1;
				clt2srv = receive_message(item, &in);
				switch(clt2srv->status) {
					case 0:
						if(command == "create") {
							cout << "OK: " << clt2srv->pid << endl;
							for(int i = 1; i < leaves.size(); i++) { if(leaves[i].id == p_id) leaves.erase(leaves.begin() + i); }
							leaf.id = clt2srv->id;
							leaf.port = clt2srv->port;
							leaves.push_back(leaf);
							ids.push_back(clt2srv->id);
							for(int i = 0; i < leaves.size(); i++) {
								cout << leaves[i].id << "--" << leaves[i].port << " ";
							}
							cout << endl;
							for(int i : ids) {
								cout << i << " ";
							}
							cout << endl;
							zmq_msg_close(&in);
						} else if(command == "pingall") {
							
							cout << endl;
						} else if(command == "exit") {
							loop = false;
						}
						
						break;
					case 1:
						if(command == "create") {
							// cout << "Creating new list..." << endl;
						} else if(command == "remove") {

						}
						break;
					case 2:
						// cout << "No node with such id in this list" << endl;
						break;
				}
				zmq_msg_close(&in);	
			}

			send = false;
		}
	}
	for(auto item : sockets) {
		zmq_close(item);
	}
	zmq_ctx_destroy(context);
	cout << "Session is over" << endl;
	return 0;
}
