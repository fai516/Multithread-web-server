
#include <iostream>
#include <string>
#include <sys/socket.h>  /* define socket */
#include <netinet/in.h>  /* define internet socket */ 
#include <netdb.h>       /* define internet socket */
#include <string.h>  //deprecated
#include <stdio.h>   //deprecated
#include <unistd.h>  //deprecated
#include <pthread.h>
#include <signal.h>
#include <cstdlib>

using namespace std;

#define SERVER_PORT 4045

void* reader(void* arg);
void sighandler(int sig);

int main()
{
 // socket stuff (including gethostbyname(), bcopy(), socket())

	int soc; 
	pthread_t td;
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
	char buf[512]; 
	struct hostent *hp;
	string hostname;
	string nickname;
	
	signal(SIGINT,sighandler);
	signal(SIGTERM,sighandler);
	
	cout << endl << "Please enter the server's hostname: ";
	
	cin >> hostname;
	
	bool tryagain = true;
	
	while ( ( hp = gethostbyname( hostname.c_str() ) ) == NULL) 
	{ 
		cout << endl << "Unknown host \"" << hostname << "\"!" << endl;
		
		if (!tryagain) return 1;
		
		cout << endl << "Please try another hostname: ";
			
		cin >> hostname;
				
		tryagain = false;
	}
	bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length ); 
	
	/* create a socket */ 
	if( ( soc = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) 
	{ 
		cerr << "client: socket failed" << endl; 
		return 1; 
	} 
 
	/* connect a socket */ 
	if( connect( soc, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 ) 
	{ 
		cerr << "client: connect FAILED:" << endl; 
		return 1; 
	} 
 
	//cout << "connect() successfull!" << endl; 
   
	//introduce the client to server by passing the nickname;
	cout << "Please enter a nickname: ";
	
	cin >> nickname;
	//read the echoed message;
	//create a thread for reading messages;
	
	//cout << "nickname entered" << endl;
	
	int writestatus = write(soc, nickname.c_str(), nickname.size()+1);
	
	//cout << "wrote nickname, status " << writestatus << endl;
	
	pthread_create(&td,NULL,&reader,(void*)&soc);
	
	//write input to socket
	while( gets(buf) != NULL) //while still input from keyboard
	{ 
		bool quit = strcmp(buf,"/exit") == 0 || strcmp(buf,"/quit") == 0 || strcmp(buf,"/part") == 0;
		write(soc, buf, sizeof(buf)); 
		if (quit)
		{
			close(soc);
			exit(0);
			//or break, but I don't know how running threads get closed;
		}

	}   

	//close stuff ...
	close(soc); 
}

void* reader(void* arg)
{
	//cout << "reader thread started" << endl;
	char buf[512];
	int* FD = (int*)arg;
	while (	read(*FD, buf, sizeof(buf)) >= 0)
	{
		if (strcmp(buf,"\\c") != 0)
			cout << buf << endl; 
		else
		{
			close(*FD);
			exit(0);
		}
	}
	close(*FD);
	//pthread_exit();
	return NULL;
}

void sighandler(int sig)
{
	cout << "Please enter either \"/exit\", \"/quit\", or \"/part\" in order to exit." << endl;
}