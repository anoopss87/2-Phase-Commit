/*Advanced Operating Systems - Spring 2016 
 * Programming Project #1
 * 
 * Client.cpp - Client node of the system
 * @author - Anoop S Somashekar
*/

#include "common.h"

//Global variables
map<int, string> serverList;
map<int, string> clientList;
map<int, int> serverPortList;
map<int, string> clientDirList;
int context;

void init_client()
{
	struct sockaddr_in init_server, init_client;
	int initSock, siz;
	
	//init Socket
	initSock = socket(AF_INET , SOCK_STREAM , 0);
	if (initSock == -1)
	{
		printf("Could not create socket");
		exit(1);
	}

	//Prepare the sockaddr_in structure
	init_server.sin_family = AF_INET;
	init_server.sin_addr.s_addr = INADDR_ANY;
	init_server.sin_port = htons(6500 + context);    

	int m_true = 1;
	setsockopt(initSock,SOL_SOCKET,SO_REUSEADDR,&m_true,sizeof(int));

	//Bind
	if( bind(initSock,(struct sockaddr *)&init_server , sizeof(init_server)) < 0)
	{
		puts("Bind failed. Error");
		exit(1);
	}
    siz = sizeof(struct sockaddr_in);

	//Listen
	listen(initSock , 1);
	
	if((accept(initSock,(struct sockaddr *)&init_client, (socklen_t*)&siz)) < 0)
	{
		printf("Initiator accept failed Error %d\n", errno);
		exit(1);
	}
}


int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Usage %s context\n", argv[0]);
        exit(1);
    }
    
    context = atoi(argv[1]);
    init_client();        
     
    serverList[0] = "10.176.66.73";
    serverList[1] = "10.176.66.74";
    serverList[2] = "10.176.66.75";	

    clientList[0] = "dc30.utdallas.edu";
    clientList[1] = "dc31.utdallas.edu";
    clientList[2] = "dc32.utdallas.edu";
    clientList[3] = "dc33.utdallas.edu";
    clientList[4] = "dc34.utdallas.edu";
    clientList[5] = "dc35.utdallas.edu";
    clientList[6] = "dc36.utdallas.edu";

    serverPortList[0] = 8003;
    serverPortList[1] = 8004;
    serverPortList[2] = 8005;     

    clientDirList[0] = "c1";
    clientDirList[1] = "c2";
    clientDirList[2] = "c3";
    clientDirList[3] = "c4";
    clientDirList[4] = "c5";
    clientDirList[5] = "c6";
    clientDirList[6] = "c7";
    	  
	struct sockaddr_in my_addr[NUM_OF_SERVERS];   
	int hsock[NUM_OF_SERVERS] = {0};
	int * p_int;
    int err;
    struct hostent *servDetails;    
	
    //temporary int pointer for setsockopt call
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
		
    //set client socket opt
    for(int i=0;i<NUM_OF_SERVERS;++i)
    {
		hsock[i] = socket(AF_INET, SOCK_STREAM, 0);		
		if(hsock[i] == -1)
		{
			printf("Error initializing socket %d\n",errno);
			exit(1);
		}    
		if( (setsockopt(hsock[i], SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
			(setsockopt(hsock[i], SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) )
		{
			printf("Error setting options %d\n",errno);
			free(p_int);
			exit(1);
		}
		*p_int = 1;
	}
	free(p_int);

    //server socket setup details.
    for(int i=0;i<NUM_OF_SERVERS;++i)
    {
		my_addr[i].sin_family = AF_INET ;
		my_addr[i].sin_port = htons(serverPortList.find(i)->second);
		memset((void *)&(my_addr[i].sin_zero), 0, 8);   
		servDetails = gethostbyname(serverList.find(i)->second.c_str());
		bcopy((char *)servDetails->h_addr, (char *)&my_addr[i].sin_addr.s_addr, servDetails->h_length);
		//my_addr[i].sin_addr.s_addr = inet_addr(serverList.find(i)->second.c_str());
	}
    
    //establish socket connection from client to server
    for(int i=0;i<NUM_OF_SERVERS;++i)
    {
		if( connect( hsock[i], (struct sockaddr*)&my_addr[i], sizeof(my_addr[i])) == -1 )
		{
			if((err = errno) != EINPROGRESS)
			{
				fprintf(stderr, "Error connecting socket %d\n", errno);
				exit(1);
			}
		}
	}

    char filePath[100] = {0};
    strcpy(filePath, clientDirList.find(context)->second.c_str());
    strcat(filePath, "/log.txt");
    int fd = open(filePath, O_RDWR|O_APPEND, 0644);    

    msgPkt m_pkt, m_pktRec;
    char str[100]; 
    struct timeval timestamp1, timestamp2;
    char logEntry[200];       
    for(int i=0;i<NUM_OF_WRITES;++i)
    {			
		memset(&m_pkt, 0, sizeof(m_pkt));	
		memset(&m_pktRec, 0, sizeof(m_pktRec));		
		m_pkt.context = context;
		m_pkt.type = C_W_REQUEST;		
		sprintf(str, "< Client Id : %d, Message No : %d, Hostname : %s >\n", context, (i+1), clientList.find(context)->second.c_str());
        strcpy(m_pkt.message, str);		
		
		int randomServer = rand() % 3;
		printf("Random Server is %d\n", randomServer);
		
		send(hsock[randomServer], (void *)&m_pkt, sizeof(struct msgPkt), 0);        
		// timestamp sent request
		gettimeofday(&timestamp1,NULL);
        recv(hsock[LEADER_ID],(void *)&m_pktRec, sizeof(struct msgPkt), 0);       
        while(m_pktRec.type != C_ACK)
        {
			recv(hsock[LEADER_ID],(void *)&m_pktRec, sizeof(struct msgPkt), 0);						
		}
        printf("%d write completed\n", (i+1));
        // timestamp received ACK
        gettimeofday(&timestamp2,NULL);

        long int t1_ms = timestamp1.tv_sec * 1000.0;   // sec to ms
        t1_ms += (timestamp1.tv_usec / 1000.0);         // us to ms
        long int t2_ms = timestamp2.tv_sec * 1000.0;   // sec to ms
        t2_ms += (timestamp2.tv_usec / 1000.0);         // us to ms

        long int elapsedTime = t2_ms - t1_ms; // in milliseconds
        char *logLine = logEntry;
        logLine += sprintf(logLine, "Elapsed time between request %d and receiving ACK from the leader:\n", (i+1));
        logLine += sprintf(logLine, "%li s %li ms\n\n\n", (elapsedTime / 1000), (elapsedTime % 1000));
        write(fd, logEntry, strlen(logEntry));
       
        //wait for some time before next write request
        int randomWait = (rand() % 40) + 10;
        usleep(randomWait * 1000);
	}

    memset(&m_pkt, 0, sizeof(m_pkt));
    m_pkt.context = context;
    m_pkt.type = TERMINATE;
    send(hsock[LEADER_ID], (void *)&m_pkt, sizeof(struct msgPkt), 0);	
    return 0;
}
