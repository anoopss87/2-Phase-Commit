/*Advanced Operating Systems - Spring 2016 
 * Programming Project #1
 * 
 * Server.cpp - Server node of the system
 * @author - Anoop S Somashekar
*/

#include "common.h"

void *connectToOtherServers(void *);
void *ServerReplyReceiveHandler(void *);
void *ServerRequestReceiveHandler(void *);
void *ClientMessageRecHandler(void *);

//map which will hold server Lists.
map<int, string> serverList;
map<int, int> serverPortList;
map<string, int> clientList;
map<int, string> dirList;

map<int, string> messageBuffer;

int context;
int leaderSock = -1;
int clientSock[NUM_OF_CLIENTS];
int agreeCount[NUM_OF_CLIENTS] = {0};
int ackCount[NUM_OF_CLIENTS] = {0};
int fd;
int doneCount = 0;
int logFD;

int serverReqSendCount[NUM_OF_CLIENTS];
int agreeSendCount[NUM_OF_CLIENTS];
int commitReqSendCount[NUM_OF_CLIENTS];
int commitSendCount[NUM_OF_CLIENTS];
int commitAckSendCount[NUM_OF_CLIENTS];

int logCounter[NUM_OF_CLIENTS];

int serverSock[NUM_OF_SERVERS-1];

pthread_mutex_t agreeCountLock[NUM_OF_CLIENTS];
pthread_mutex_t ackCountLock[NUM_OF_CLIENTS];
pthread_mutex_t messageBufferLock;
pthread_mutex_t fileLock;
pthread_mutex_t serializeLock;
pthread_mutex_t doneCountLock;

pthread_mutex_t serverReqSendCountLock[NUM_OF_CLIENTS];
pthread_mutex_t agreeSendCountLock[NUM_OF_CLIENTS];
pthread_mutex_t commitReqSendCountLock[NUM_OF_CLIENTS];
pthread_mutex_t commitSendCountLock[NUM_OF_CLIENTS];
pthread_mutex_t commitAckSendCountLock[NUM_OF_CLIENTS];
pthread_mutex_t logCounterLock[NUM_OF_CLIENTS];

pthread_mutex_t logFDLock;

void init_server()
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
	init_server.sin_port = htons(9000 + context);    

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
	close(initSock);
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        printf("Bad arguments: Usage %s server_port context e.g. ./server 8000 0", argv[0]);
        exit(1);
    }   
    
    struct sockaddr_in my_addr;
    int cSocket = 0;
    
    context = atoi(argv[2]);
    
    init_server();

    serverList[0] = "10.176.66.73";
    serverList[1] = "10.176.66.74";
    serverList[2] = "10.176.66.75";  

    serverPortList[0] = 8003;
    serverPortList[1] = 8004;
    serverPortList[2] = 8005; 

    dirList[0] = "s1";
    dirList[1] = "s2";
    dirList[2] = "s3";  
    
    clientList["dc30.utdallas.edu"] = 0;
    clientList["dc31.utdallas.edu"] = 1;
    clientList["dc32.utdallas.edu"] = 2;
    clientList["dc33.utdallas.edu"] = 3;
    clientList["dc34.utdallas.edu"] = 4;
    clientList["dc35.utdallas.edu"] = 5;
    clientList["dc36.utdallas.edu"] = 6; 
    
    int hsock;
    int * p_int ;
    int err;    
    
    sockaddr_in sadr;
    pthread_t client_thread_id, connect_thread, server_thread_id;
    socklen_t addr_size = sizeof(sockaddr_in);

    //create server socket
    hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock == -1)
	{
		printf("Error initializing socket %d\n", errno);
		exit(1);
    }
	
    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;
		
    //set sock options for the server socket    
    if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) )
	{
	    printf("Error setting options %d\n", errno);
	    free(p_int);
	    exit(1);
	}
    free(p_int);

    //server socket setup 
    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(atoi(argv[1]));
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY ;
	
    //binding server socket with server details
    if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
	{
		fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n",errno);
		exit(1);
    }

    //listen at server port
    if(listen( hsock, 20) == -1 )
	{
		fprintf(stderr, "Error listening %d\n",errno);
		exit(1);
    }    
  
	//to make sure that all servers are listening
    sleep(1);
    
    if(pthread_create(&connect_thread, NULL, connectToOtherServers, (void *)NULL) < 0)
	{
		puts("Failed to create the Server Thread");
		exit(1);
	}
    
    int connCount = 0;
    int *lsocket;
    //wait for client connections	
    	
	while((cSocket = accept( hsock, (sockaddr*)&sadr, &addr_size)))
	{
		connCount++;
		lsocket = (int*)malloc(sizeof(int));
		*lsocket = cSocket;		
		if(connCount <= 2)
		{
			char node[1024] = {0};
			int res = getnameinfo((sockaddr*)&sadr, sizeof(sadr), node, sizeof(node), NULL, 0, 0);
			printf("Server is %s\n", node);			
			pthread_create(&server_thread_id,0,&ServerRequestReceiveHandler, (void*)lsocket );
		}
		else if(connCount <= 9)
		{
			char node[1024] = {0};
			int res = getnameinfo((sockaddr*)&sadr, sizeof(sadr), node, sizeof(node), NULL, 0, 0);
			printf("Client is %s\n", node);	
			int index = clientList.find(node)->second;
			clientSock[index] = cSocket;		
			pthread_create(&client_thread_id,0,&ClientMessageRecHandler, (void*)lsocket );
		}					
    }
}

void *connectToOtherServers(void *temp)
{
	int *sock;
	struct sockaddr_in server_addr[NUM_OF_SERVERS-1];
	struct hostent *servDetails;
	int err;
	
	for(int i=0, j=0;i<NUM_OF_SERVERS;i++)
	{
		if(i == context)
			continue;			
			
		server_addr[j].sin_family = AF_INET;
		server_addr[j].sin_port = htons(serverPortList.find(i)->second);
		memset((void *)&(server_addr[j].sin_zero), 0, 8);
		servDetails = gethostbyname(serverList.find(i)->second.c_str());
		bcopy((char *)servDetails->h_addr, (char *)&server_addr[j].sin_addr.s_addr, servDetails->h_length);
		j++;
	}
	
	pthread_t serverThread;

    // connect to other servers 
	for(int i=0;i<NUM_OF_SERVERS-1;i++)
	{				
		// Create a socket		
		serverSock[i] = socket(AF_INET, SOCK_STREAM, 0);
		sock = (int*)malloc(sizeof(int));
		*sock = serverSock[i];	
				
		if( connect(serverSock[i], (struct sockaddr *)&(server_addr[i]), sizeof(server_addr[i])) < 0) 
		{
			if((err = errno) != EINPROGRESS)
			{
				fprintf(stderr, "Error connecting socket %d\n", errno);
				exit(1);
			}			
		}
		
		//create receiver handler thread for each request messages
		if(pthread_create(&serverThread, NULL, ServerReplyReceiveHandler, (void *)sock) < 0)
		{
			puts("Failed to create the Server Thread");
			exit(1);
		}		
	}

    if(LEADER_ID == 0)
    {
        if(context == 1)
		{
			leaderSock = serverSock[0];
		}
		else if(context == 2)
		{
			leaderSock = serverSock[0];
		}
    }
	
	else if(LEADER_ID == 1)
	{
		if(context == 0)
		{
			leaderSock = serverSock[0];
		}
		else if(context == 2)
		{
			leaderSock = serverSock[1];
		}
	}
    else if(LEADER_ID == 2)
    {
        if(context == 0)
		{
			leaderSock = serverSock[1];
		}
		else if(context == 1)
		{
			leaderSock = serverSock[1];
		}
    }
    
    //replica file
    char filePath[100] = {0};
    strcpy(filePath, dirList.find(context)->second.c_str());
    strcat(filePath, "/client.txt");
    fd = open(filePath, O_RDWR|O_APPEND, 0644);	

    //log file
    memset((void*)filePath, 0, sizeof(filePath));
    strcpy(filePath, dirList.find(context)->second.c_str());
    strcat(filePath, "/log.txt");
    logFD = open(filePath, O_RDWR|O_APPEND, 0644);
}

void* ServerRequestReceiveHandler(void* lp)
{
	int ssock = *(int*)lp;
	msgPkt m_pkt;
    memset(&m_pkt, 0, sizeof(m_pkt));    
     	
    //receive the data after accepting the connection.
	while(recv(ssock, (void *)&m_pkt, sizeof(m_pkt), 0) != -1)
	{

        if(m_pkt.type == TERMINATE)
        {
            printf("SERVER TERMINATING\n");
            close(fd);
            exit(0);
        }
		//cohort
		if(m_pkt.type == S_W_REQUEST)
		{
			msgPkt pkt1;
            memset(&pkt1, 0, sizeof(pkt1));                       
                                 
            pkt1.type = AGREE;
            pkt1.context = m_pkt.context;            

            //send agree message 
            send(ssock, (void *)&pkt1, sizeof(pkt1), 0); 
            //printf("Server write request received - %s\n", m_pkt.message);             
            
            pthread_mutex_lock(&messageBufferLock);
                messageBuffer[m_pkt.context] = m_pkt.message;
            pthread_mutex_unlock(&messageBufferLock); 
            
            pthread_mutex_lock(&agreeSendCountLock[m_pkt.context]);
                agreeSendCount[m_pkt.context]++;                  
            pthread_mutex_unlock(&agreeSendCountLock[m_pkt.context]);
		}
		
		//leader
		if(m_pkt.type == COMMIT_REQUEST)
		{
                char temp[100] = {0};
                strcpy(temp, messageBuffer.find(m_pkt.context)->second.c_str());

                pthread_mutex_lock(&fileLock);
                    write(fd, temp, strlen(temp));
                pthread_mutex_unlock(&fileLock);

                printf("%s\n", temp);  
			    msgPkt pkt1, pkt2;
			    memset(&pkt1, 0, sizeof(pkt1));			
			    pkt1.type = COMMIT;
			    pkt1.context = m_pkt.context;
					
			    send(serverSock[0], (void *)&pkt1, sizeof(pkt1), 0);
					
			    memset(&pkt2, 0, sizeof(pkt2));			
			    pkt2.type = COMMIT;
			    pkt2.context = m_pkt.context;
					
			    send(serverSock[1], (void *)&pkt2, sizeof(pkt2), 0);

                pthread_mutex_lock(&commitSendCountLock[m_pkt.context]);
                    commitSendCount[m_pkt.context] += 2;
                pthread_mutex_unlock(&commitSendCountLock[m_pkt.context]);
		}
		
		//cohort
		if(m_pkt.type == COMMIT)
		{
                char temp[100] = {0};
                strcpy(temp, messageBuffer.find(m_pkt.context)->second.c_str());

                pthread_mutex_lock(&fileLock);
                    write(fd, temp, strlen(temp));
                pthread_mutex_unlock(&fileLock);
                printf("%s\n", temp);

			    msgPkt pkt1;
                memset(&pkt1, 0, sizeof(pkt1));                       
                                 
                pkt1.type = COMMIT_ACK;
                pkt1.context = m_pkt.context; 
           
                //send commit ack message 
                send(ssock, (void *)&pkt1, sizeof(pkt1), 0);

                pthread_mutex_lock(&commitAckSendCountLock[m_pkt.context]);
                    commitAckSendCount[m_pkt.context]++;
                pthread_mutex_unlock(&commitAckSendCountLock[m_pkt.context]);

                pthread_mutex_lock(&logCounterLock[m_pkt.context]);
                    logCounter[m_pkt.context]++;
                pthread_mutex_unlock(&logCounterLock[m_pkt.context]);
                char logEntry[1000] = {0};
                char *logData = logEntry;
                logData += sprintf(logData, "*********** Message Counts for Client %d ****************\n", m_pkt.context);
                logData += sprintf(logData, "Server request count - %d\n", serverReqSendCount[m_pkt.context]);
                logData += sprintf(logData, "Agree Count - %d\n", agreeSendCount[m_pkt.context]);
                logData += sprintf(logData, "Commit request Count - %d\n", commitReqSendCount[m_pkt.context]);
                logData += sprintf(logData, "Commit count - %d\n", commitSendCount[m_pkt.context]);
                logData += sprintf(logData, "Commit ack count - %d\n", commitAckSendCount[m_pkt.context]);
                logData += sprintf(logData, "Client ack count - 0\n *******************************************************\n");
                
                if(logCounter[m_pkt.context] == NUM_OF_WRITES)
                {
                    int total = serverReqSendCount[m_pkt.context] + agreeSendCount[m_pkt.context] +  commitReqSendCount[m_pkt.context] + commitSendCount[m_pkt.context] + commitAckSendCount[m_pkt.context];
                    logData += sprintf(logData, "+++++++++++ Total number of messages sent for Client %d - %d ++++++++++++++\n",m_pkt.context, total);
                }

                pthread_mutex_lock(&logFDLock);
                    write(logFD, logEntry, strlen(logEntry));
                pthread_mutex_unlock(&logFDLock);
  
		}
		memset(&m_pkt, 0, sizeof(m_pkt));		
	}
}

void* ServerReplyReceiveHandler(void *lp)
{
	int ssock = *(int*)lp;	
	msgPkt m_pkt;
    memset(&m_pkt, 0, sizeof(m_pkt));
     	
    //receive the data after accepting the connection.
	while(recv(ssock, (void *)&m_pkt, sizeof(m_pkt), 0) != -1)
	{
		//master server randomly selected by client						
		if(m_pkt.type == AGREE)
		{
			//printf("Agree received\n");
			pthread_mutex_lock(&agreeCountLock[m_pkt.context]);			
			    agreeCount[m_pkt.context]++;
            //printf("So far agree count for context %d is %d\n", m_pkt.context, agreeCount[m_pkt.context]);
			
			if(agreeCount[m_pkt.context] == 2)
			{                
				if(leaderSock > 0)
				{
					msgPkt pkt1;
					memset(&pkt1, 0, sizeof(pkt1));				
					pkt1.type = COMMIT_REQUEST;
					pkt1.context = m_pkt.context;
					send(leaderSock, (void *)&pkt1, sizeof(pkt1), 0);

                    pthread_mutex_lock(&commitReqSendCountLock[m_pkt.context]);
                        commitReqSendCount[m_pkt.context]++;
                    pthread_mutex_unlock(&commitReqSendCountLock[m_pkt.context]);
					//printf("commit request sent to the leader from context %d\n", m_pkt.context);					
				}
				else
				{
                    pthread_mutex_lock(&serializeLock);
                        char temp[100] = {0};
                        strcpy(temp, messageBuffer.find(m_pkt.context)->second.c_str()); 
                       
                        pthread_mutex_lock(&fileLock);
                            write(fd, temp, strlen(temp));
                        pthread_mutex_unlock(&fileLock);
                        printf("%s\n", temp); 

					    msgPkt pkt1, pkt2;
					    memset(&pkt1, 0, sizeof(pkt1));
					    memset(&pkt2, 0, sizeof(pkt2));					
					
					    pkt1.type = COMMIT;
					    pkt1.context = m_pkt.context;
					
					    send(serverSock[0], (void *)&pkt1, sizeof(pkt1), 0);					
									
					    pkt2.type = COMMIT;
					    pkt2.context = m_pkt.context;
					
					    send(serverSock[1], (void *)&pkt2, sizeof(pkt2), 0);

                        commitSendCount[m_pkt.context] += 2;
                    pthread_mutex_unlock(&serializeLock);
					
														
				}
				agreeCount[m_pkt.context] = 0;
			}		
			pthread_mutex_unlock(&agreeCountLock[m_pkt.context]);
		}
		
		//leader
		if(m_pkt.type == COMMIT_ACK)
		{
			//printf("Ack received\n");
			pthread_mutex_lock(&ackCountLock[m_pkt.context]);
				ackCount[m_pkt.context]++;		
				
			if(ackCount[m_pkt.context] == 2)
			{
				msgPkt pkt1;
				memset(&pkt1, 0, sizeof(pkt1));				
				pkt1.type = C_ACK;
				send(clientSock[m_pkt.context], (void *)&pkt1, sizeof(pkt1), 0);
				//printf("Ack sent to the client\n");			
				ackCount[m_pkt.context] = 0;

                pthread_mutex_lock(&logCounterLock[m_pkt.context]);
                    logCounter[m_pkt.context]++;
                pthread_mutex_unlock(&logCounterLock[m_pkt.context]);

                char logEntry[1000] = {0};
                char *logData = logEntry;
                logData += sprintf(logData, "*********** Message Counts for Client %d ****************\n", m_pkt.context);
                logData += sprintf(logData, "Server request count - %d\n", serverReqSendCount[m_pkt.context]);
                logData += sprintf(logData, "Agree Count - %d\n", agreeSendCount[m_pkt.context]);
                logData += sprintf(logData, "Commit request Count - %d\n", commitReqSendCount[m_pkt.context]);
                logData += sprintf(logData, "Commit count - %d\n", commitSendCount[m_pkt.context]);
                logData += sprintf(logData, "Commit ack count - %d\n", commitAckSendCount[m_pkt.context]);
                logData += sprintf(logData, "Client ack count - 1\n ***********************************************************\n");
        
                if(logCounter[m_pkt.context] == NUM_OF_WRITES)
                {
                    int total = serverReqSendCount[m_pkt.context] + agreeSendCount[m_pkt.context] +  commitReqSendCount[m_pkt.context] + commitSendCount[m_pkt.context] + commitAckSendCount[m_pkt.context] + 40;
                    logData += sprintf(logData, "+++++++++++ Total number of messages sent for Client %d - %d ++++++++++++++\n",m_pkt.context, total);
                }             

                pthread_mutex_lock(&logFDLock);
                    write(logFD, logEntry, strlen(logEntry));
                pthread_mutex_unlock(&logFDLock);               			
			}	
			pthread_mutex_unlock(&ackCountLock[m_pkt.context]);		
		}
		memset(&m_pkt, 0, sizeof(m_pkt));
	}
}

void* ClientMessageRecHandler(void* lp)
{
    int csock = *(int*)lp;   

    msgPkt m_pkt;
    memset(&m_pkt, 0, sizeof(m_pkt));
     	
    //receive the data after accepting the connection.
	while(recv(csock, (void *)&m_pkt, sizeof(m_pkt), 0) != -1)
	{
		//master server randomly selected by client					
		if(m_pkt.type == C_W_REQUEST)
		{
			//printf("client write request received - %s\n", m_pkt.message);
            pthread_mutex_lock(&messageBufferLock);
                messageBuffer[m_pkt.context] = m_pkt.message;
            pthread_mutex_unlock(&messageBufferLock);	
			msgPkt pkt1, pkt2;
            memset(&pkt1, 0, sizeof(pkt1));
            memset(&pkt2, 0, sizeof(pkt2));                                
           
            pkt1.type = S_W_REQUEST;
            strcpy(pkt1.message, m_pkt.message);
            pkt1.context = m_pkt.context;

            //forward the message to other server 
            send(serverSock[0], (void *)&pkt1, sizeof(pkt1), 0);
        
            pkt2.type = S_W_REQUEST;
            pkt2.context = m_pkt.context;
            strcpy(pkt2.message, m_pkt.message);            

            //forward the message to other server
            send(serverSock[1], (void *)&pkt2, sizeof(pkt2), 0);

            pthread_mutex_lock(&serverReqSendCountLock[m_pkt.context]);
                serverReqSendCount[m_pkt.context] += 2;
            pthread_mutex_unlock(&serverReqSendCountLock[m_pkt.context]);
		}
        else if(m_pkt.type == TERMINATE)
        {
            pthread_mutex_lock(&doneCountLock);
                doneCount++;
                if(doneCount == NUM_OF_CLIENTS)
                {
                    msgPkt p1, p2;
                    memset(&p1, 0, sizeof(struct msgPkt));
                    memset(&p2, 0, sizeof(struct msgPkt));
                    p1.type = TERMINATE;
                    p2.type = TERMINATE;
                    send(serverSock[0], (void *)&p1, sizeof(p1), 0);
                    send(serverSock[1], (void *)&p2, sizeof(p2), 0);
                    printf("LEADER TERMINATING\n");
                    close(fd);
                    usleep(100);
                    exit(0);
                }
            pthread_mutex_unlock(&doneCountLock);
        }
        else
        {
            //printf("Invalid message - %d\n", m_pkt.type);
        }							
		memset(&m_pkt, 0, sizeof(m_pkt));     
    }
}
