/************************************************************************/
/*   PROMGRAMMER NAMES: Jerry Linder and the Google search engine		*/	
/*   PROGRAM NAME: server.cpp  (works with client.cpp)                  */
/*	 ASSIGNMENT: C3800 Operating Systems Project 3 (Inst: Fikret Ercal) */
/*                                                                      */
/*   Server creates a socket to listen for the connection from Client   */
/*   Client asks for a display name which then gets read by the Server. */
/*   When the communication established, Server echoes data from Client */
/*   and writes them back.                                              */
/*                                                                      */
/*   COMPILE:         g++ server.cpp -pthread -o server    		        */
/*	 TO RUN:		  ./server											*/
/*                                                                      */
/************************************************************************/

#include <sys/socket.h>  /* define socket */
#include <netinet/in.h>  /* define internet socket */
#include <netdb.h>       /* define internet socket */
#include <stdio.h>   //deprecated
#include <unistd.h>  //deprecated
#include <pthread.h>
#include <string.h> //deprecated
#include <iostream>
#include <cstdlib>
#include <signal.h>

#define SERVER_PORT 4045        /* define a server port number */

using namespace std;


const int MAX_CLIENT = 50;

void* thread_main(void* arg);
void strdel(char* str);
void sighandler(int);

int FD[MAX_CLIENT];   /* allocate as many socket file descriptors 
						as the number of clients  */
pthread_t TD[MAX_CLIENT];   /* same for thread descriptors */
int counter; 
int sd;
//mutex m;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
struct sockaddr_in client_addr = { AF_INET };

int main()
{
	int ns;
	unsigned int client_len = sizeof( client_addr );
	signal(SIGINT,sighandler);
	signal(SIGTERM,sighandler);	
	for (int i = 0; i < MAX_CLIENT; i++) FD[i] = -1; // initialize FD array
	//socket

	if( ( sd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		perror( "server: socket failed" );
		return 1;	
	}
	//cout << "socket created" << endl;
	//bind
	if( bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
	{
		perror( "server: bind failed" );
		return 1;	
	}
	//cout << "bind" << endl;
	//listen(n)   /* n is the size of the queue that holds incoming requests
	//               from clients that want to connect  */

	if( listen( sd, 10 ) == -1 )
	{
		perror( "server: listen failed" );
		return 1;	
	}
	//cout << "listen" << endl;
	cout << "server started" << endl;
	while(( ns = accept(sd,(struct sockaddr*)&client_addr, &client_len)) > 0)
	{  
		bool maxClientsConnected = false;
		counter = 0;
		int* FDindex = new int;
		//cout << "accepting client" << endl;
		//lock(m);
		pthread_mutex_lock(&m);
		if (FD[counter] == -1 && counter != MAX_CLIENT) //FD[counter] is unused and not out of bounds
		{
			FD[counter] = ns;
			*FDindex = counter++;
			//cout << "initial FDindex = " << *FDindex << endl;
		}
		else
		{
			for (int i = 0; i < MAX_CLIENT; i++) //search for empty place in FD
			{
				if (FD[i] != -1) 
				{
					counter = i;
					FD[counter] = ns;  
					*FDindex = counter++;	
					//cout << "else statement: initial FDindex = " << *FDindex << endl;
					break;
				}  
			}
			if (counter == MAX_CLIENT) //max number of clients have already connected
			{
				pthread_mutex_unlock(&m);		//unlock(m) early, not needed for this;
				char* msg = new char[81];
				strcpy(msg,"SERVER MESSAGE: Sorry, but the maximum number of clients has already connected.\n");
				write(ns,&msg,81);
				close(ns);
				maxClientsConnected = true;
				delete [] msg;
			}
		}
		pthread_mutex_unlock(&m);		//unlock(m);
		if (!maxClientsConnected)
		{
			//if ( pthread_create(&TD[counter], NULL, &thread_main, (void*)newsocket) != 0)
			//	cout << "thread create error" <<endl;
			pthread_create(&TD[counter], NULL, &thread_main, (void*)FDindex);
		}
	}   
	//close stuff ...
	close(ns);
	close(sd);
	unlink( (const char*)&server_addr.sin_addr);

	return(0);

}

void* thread_main(void* arg)
{
	//cout << "client thread started" << endl;
	//get  fd index;
	int FDindex = *((int*)arg);
	//cout << "FDindex = "  << FDindex << endl;
	delete (int*)arg;
	pthread_mutex_lock(&m);	//lock(m);
	int thisFD = FD[FDindex];
	//cout << "thisFD = " << thisFD << endl;
	pthread_mutex_unlock(&m); //unlock(m);
	const int k = 512;
	char outtext[k];
	char clientname[k];
	char clienthandle[k];
	char buf[k];
	//read in client name;
	int status;
	//cin.get();
	//cout << "about to read nickname" << endl;
	if ( (status = read(thisFD, buf, k)) < 0)
	{
		perror("Reading Name Error\n");
		exit(1);
	}
	//cout << "read status " << status << endl;
	//cout << "nickname=" << buf << endl;
	strcpy(clientname,buf);
	//print a message about the new client;
	const char* servermessage = "SERVER MESSAGE: ";
	const char* hasjoined = " has joined the chat.\n";
	strcat(strcat(strcat(outtext,servermessage),clientname),hasjoined);
	//cout << "outtext len = " << strlen(outtext) << endl;
	//cout << "outtext=" << endl;
	cout << outtext << endl;
	pthread_mutex_lock(&m);
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		if (FD[i] != -1) write(FD[i], outtext, k);
		//unlock(m)          
	}
	pthread_mutex_unlock(&m);
	//strdel(outtext);
	strcat(strcpy(clienthandle,clientname),": ");
	//cout << "starting main read loop" << endl;
	while ((status = read(thisFD, buf, k)) > 0)
	{
		if (status == k)
		{
			//cout << "read text" << endl;
			bool quit = strcmp(buf,"/exit") == 0 || strcmp(buf,"/quit") == 0 || strcmp(buf,"/part") == 0;
			if (quit)
			{
				sprintf(outtext,"%s%s has left the chat room.\n",servermessage,clientname);
				cout << outtext;
				pthread_mutex_lock(&m);		//lock(m);
				for (int i = 0; i < MAX_CLIENT; i++)
				{
					if (FD[i] != -1) write(FD[i], outtext, k);
				}
				write(thisFD,"\\c",3);
				close(thisFD);
				//remove myself from FDarray
				FD[FDindex] = -1;
				pthread_mutex_unlock(&m); 	//unlock(m) 
				pthread_exit(NULL);
			}
			strcat(strcpy(outtext,clienthandle),buf);
			//cout << "readstatus = " << status << endl;
			cout << outtext << endl;
			//loop
			//write message to each FD
			pthread_mutex_lock(&m);		//unlock(m);
			for (int i = 0; i < MAX_CLIENT; i++)
			{
				if (FD[i] != -1) write(FD[i], outtext, k);
			}
			pthread_mutex_unlock(&m);		//unlock(m);
			//strdel(outtext);
		}
	}
	write(thisFD,"\\c",2);
	close(thisFD);
	//remove myself from FDarray
	pthread_mutex_lock(&m);		//lock(m);
	FD[FDindex] = -1;
	pthread_mutex_unlock(&m); 	//unlock(m) 
	//TD[count] = -1;
	//pthread_exit(NULL);
	return NULL;
}

void sighandler(int sig)
{
	cout << "kill signal recieved" << endl;
	for (int i = 10; i > 0; i--)
	{
		sleep(1);
		char* outtext = new char[65];
		sprintf(outtext,"SERVER MESSAGE: Server process will exit in %d seconds!\n",i);
		cout << outtext;
		pthread_mutex_lock(&m);		//lock(m);
		for (int j = 0; j < MAX_CLIENT; j++)
		{
			if (FD[j] != -1) write(FD[j], outtext, 65);
		}
		pthread_mutex_unlock(&m);		//unlock(m);
		delete [] outtext;
		
	}
	sleep(1);
	pthread_mutex_lock(&m);		//lock(m);
	for (int j = 0; j < MAX_CLIENT; j++)
	{
		if (FD[j] != -1)
		{
			write(FD[j],"\\c",3);
			close(FD[j]);
		}
	}
	pthread_mutex_unlock(&m);		//unlock(m);
	close(sd);
	unlink( (const char*)&server_addr.sin_addr);
	exit(0);
}