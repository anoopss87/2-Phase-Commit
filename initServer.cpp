/*Advanced Operating Systems - Spring 2016 
 * Programming Project #1
 * 
 * Initiator.cpp - initiates the client's maekawa algorithm
 * @author - Anoop S Somashekar
*/
#include "common.h"

map<int, string> serverList;
map<int, int> serverPort;

int main(int argc, char *argv[])
{ 
    serverList[0] = "10.176.66.73";
    serverList[1] = "10.176.66.74";
    serverList[2] = "10.176.66.75";    
    
    serverPort[0] = 9000;
    serverPort[1] = 9001;
    serverPort[2] = 9002;  

	struct sockaddr_in my_addr[NUM_OF_SERVERS];
    struct hostent *serv;
	int hsock[3] = {};
	int * p_int;
    int err;

    //client socket
    for(int i = 0;i < NUM_OF_SERVERS;++i)
    {
		hsock[i] = socket(AF_INET, SOCK_STREAM, 0); 
		if(hsock[i] <= 0)
		{
			printf("Error initializing socket %d\n",errno);
			exit(1);
		}       
    }	
	
    //temporary int pointer for setsockopt call
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
		
    //set client socket opt
    for(int i = 0;i < NUM_OF_SERVERS;++i)
    {    
		if( (setsockopt(hsock[i], SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
			(setsockopt(hsock[i], SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) )
		{
			printf("Error setting options %d\n",errno);
			free(p_int);
			return 1;
		}
	}
	free(p_int);    
    //server socket setup details.
    for(int i = 0;i < NUM_OF_SERVERS;++i)
    {
		my_addr[i].sin_family = AF_INET ;
		my_addr[i].sin_port = htons(serverPort.find(i)->second);
		memset(&(my_addr[i].sin_zero), 0, 8);        
        serv = gethostbyname(serverList.find(i)->second.c_str());
		bcopy((char *)serv->h_addr, (char *)&my_addr[i].sin_addr.s_addr, serv->h_length);
	}
    
    //establish socket connection from client to server
    for(int i = 0; i < NUM_OF_SERVERS;++i)
    {
		if((connect(hsock[i], (struct sockaddr*)&my_addr[i], sizeof(my_addr[i]))) == -1 )
		{            
			if((err = errno) != EINPROGRESS)
			{
				fprintf(stderr, "Error connecting socket %d\n", errno);
				return 1;
			}
		}
	}
	printf("********* Initiation completed successfully *************\n");  
}
