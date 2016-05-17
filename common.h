/*
 * Advanced Operating Systems - Spring 2016 
 * Programming Project #1
 * 
 * common.h - Common includes and Structures
 * @author - Anoop S
*/

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <queue>
#include <map>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/time.h>
using namespace std;

#define NUM_OF_SERVERS 3
#define NUM_OF_CLIENTS 7
#define NUM_OF_WRITES 40
#define LEADER_ID 2

enum msgType
{
	C_W_REQUEST = 10, //client write request
	S_W_REQUEST = 11,     //server write request
	AGREE = 12,           //agreed message from servers
	COMMIT_REQUEST = 13,  //commit request from the server
	COMMIT = 14,          //commit message from leader
	COMMIT_ACK = 15,      //server commit ack message
	C_ACK = 16,           //client ack message
    TERMINATE = 17
};

struct msgPkt{    
    int context;
    msgType type;   
    char message[100];
};
#endif
