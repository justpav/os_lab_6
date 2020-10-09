#pragma once
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <map>
#include <signal.h>
#include "zmq.h"

typedef struct {
	int task;
	int status;
	int p_id;
	int id;
	int port;
	pid_t pid;
} Message;

void init_message(Message *mes){
	mes->task = -10;
	mes->status = -10;
	mes->p_id = -10;
	mes->id = -10;
	mes->port = -10;
	mes->pid = -10;
}

void send_message(void* socket, Message* out) {
	zmq_msg_t msg;
	zmq_msg_init_size(&msg, sizeof(Message));
	memcpy(zmq_msg_data(&msg), out, sizeof(Message));
	zmq_msg_send(&msg, socket, 0);
	zmq_msg_close(&msg);
}

Message* receive_message(void* socket, zmq_msg_t* msg) {
	Message *in;
	zmq_msg_init(msg);
	zmq_msg_recv(msg, socket, 0);
	in = static_cast<Message*>(zmq_msg_data(msg));
	return in;
}