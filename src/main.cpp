//-----------------------------------------------------------------------------
// Includes:
//-----------------------------------------------------------------------------

#include <libstrmanager/Manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

//-----------------------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------------------

#define PORT 6666
#define LISTENQ 512
#define MAXLINE 512
#define MyDBG printf("Error: %s:%d", __FILE__, __LINE__);

//-----------------------------------------------------------------------------
// Typedefs:
//-----------------------------------------------------------------------------

typedef void Sigfunc (int);

//-----------------------------------------------------------------------------
// Headers:
//-----------------------------------------------------------------------------

int dispatch_start (int clientfd);
int dispatch_stop (int clientfd);
Sigfunc *signal (int signo, Sigfunc *func);
void sig_chld(int signo);
ssize_t readn(int fd, void *vptr, size_t n);
ssize_t writen(int fd, const void *vptr, size_t n);
int digits(int number);
int addSource(int workerType, int src_id, char* ip, int port);
int addDestination(int src_id, int dest_id, char* ip, int port);

//-----------------------------------------------------------------------------
// Globals:
//-----------------------------------------------------------------------------

Manager * sm;

//-----------------------------------------------------------------------------
// Entry point:
//-----------------------------------------------------------------------------

int main(void)

{
	//-----------------
	// Init variables:
	//-----------------

	sm = new Manager();
	int data, listenfd, clientfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len = sizeof(cliaddr);
	pid_t pid;
	char frameid[1];

	//--------------------------
	// Socket: bind and listen:
	//--------------------------

	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {MyDBG; goto halt0;}
	if(setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &data, sizeof(data)) < 0) {MyDBG; goto halt1;}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {MyDBG; goto halt1;}
	if(listen(listenfd, LISTENQ) < 0) {MyDBG; goto halt1;}

	//-------------------
	// Signal callbacks:
	//-------------------

	if((signal(SIGCHLD, sig_chld)) == SIG_ERR) {MyDBG; goto halt1;}

	//------------
	// Main loop:
	//------------

	while(1)

	{
		//-----------------------------
		// Accept a client connection:
		//-----------------------------

		if((clientfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0) continue;

		//--------------------
		// Concurrent server:
		//--------------------

		if((pid = fork()) == 0)

		{
			//---------------------------------------
			// Do not duplicate 'listenfd' in child:
			//---------------------------------------

			close(listenfd);

			//-------------------------------
			// Get a valid frame ID or drop:
			//-------------------------------

			readn(clientfd, frameid, 1);

			switch(frameid[0])

			{
				case '0': dispatch_start(clientfd); break;
				case '1': dispatch_stop(clientfd); break;
				default: printf("Frame drop: [%c]", frameid[0]);
			}

			//------------
			// Child end:
			//------------

			close(clientfd);
			exit(0);
		}

		//---------------------
		// End of parent loop:
		//---------------------

		close(clientfd);
	}

	//------------------
	// Return on error:
	//------------------

	halt1: close(listenfd);
	halt0: return 1;
}

//-----------------------------------------------------------------------------
// dispatch_start:
//-----------------------------------------------------------------------------

int dispatch_start (int clientfd)

{
	//-----------------
	// Init variables:
	//-----------------

	char buff1[MAXLINE], buff2[MAXLINE], *c=buff1;
	int i=0, j=0, k=0, p=(int)getpid();

	//----------------------
	// Retrieve the buffer:
	//----------------------

	if((i = readn(clientfd, buff1, 2)) != 2) {MyDBG; goto halt0;}
	buff1[i]='\0'; if((j = atoi(buff1)) == 0) {MyDBG; goto halt0;}
	if((i = readn(clientfd, buff1, j)) != j) {MyDBG; goto halt0;}
	buff1[i]='\0'; j=0;

	//-------------------
	// Split the buffer:
	//-------------------

	while(sscanf(c, "%[^:]:%d", buff2, &i) == 2)

	{
		printf("PID: %d [%s]:[%d]\n", p, buff2, i);

		//------------------------------
		// Add source and destinations:
		//------------------------------

		if(j==0){k += addSource(1, p, buff2, i); j++;}
		else{k += addDestination(p, j, buff2, i); j++;}

		//---------------------------
		// Evaluate break condition:
		//---------------------------

		c += (strlen(buff2)+digits(i)+2)*sizeof(char);
		if(*(c-sizeof(char)) == '\0') {break;}
	}

	if (k != 0) {MyDBG; goto halt0;}

	//---------------------------------
	// Send PID close socket and loop:
	//---------------------------------

	if(sprintf(buff1, "%d", p) < 0) {MyDBG; goto halt0;};
	writen(clientfd, buff1, strlen(buff1));
	close(clientfd);
	while(1);

	//------------------
	// Return on error:
	//------------------

	halt0: return -1;
}

//-----------------------------------------------------------------------------
// dispatch_stop:
//-----------------------------------------------------------------------------

int dispatch_stop (int clientfd)

{
	//-----------------
	// Init variables:
	//-----------------

	char buff[MAXLINE];
	int i, j;

	//----------------------
	// Retrieve the buffer:
	//----------------------

	if((i = readn(clientfd, buff, 1)) != 1) {MyDBG; goto halt0;}
	buff[i]='\0'; if((j = atoi(buff)) == 0) {MyDBG; goto halt0;}
	if((i = readn(clientfd, buff, j)) != j) {MyDBG; goto halt0;}
	buff[i]='\0'; j=0;

	//--------------------------
	// Get PID/SID and kill it:
	//--------------------------

	if(sscanf(buff, "%d", &i) != 1) {MyDBG; goto halt0;}
	kill(i, SIGINT);

	//--------------------
	// Return on success:
	//--------------------

	return 0;

	//------------------
	// Return on error:
	//------------------

	halt0: return -1;
}

//-----------------------------------------------------------------------------
// signal:
//-----------------------------------------------------------------------------

Sigfunc *signal(int signo, Sigfunc *func)

{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if(signo == SIGALRM)

	{
		#ifdef  SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
		#endif
	}

	else

	{
		#ifdef  SA_RESTART
                act.sa_flags |= SA_RESTART;	/* SVR4, 44BSD */
		#endif
        }

        if(sigaction(signo, &act, &oact) < 0) return(SIG_ERR);
        return(oact.sa_handler);
}

//-------------------------------------------------------------------
// sig_chld:
//-------------------------------------------------------------------

void sig_chld(int signo)

{
	pid_t	pid;
	int	stat;

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0);
	return;
}

//-------------------------------------------------------------------
// readn:
//-------------------------------------------------------------------

ssize_t readn(int fd, void *vptr, size_t n)

{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = (char*)vptr;
	nleft = n;

	while(nleft > 0)

	{
		if((nread = read(fd, ptr, nleft)) < 0)

		{
			if(errno == EINTR) {nread = 0;}
			else {return -1;}
		}

		else {if(nread == 0) {break;}}

		nleft -= nread;
		ptr += nread;
	}

	return(n - nleft);
}

//-----------------------------------------------------------------------------
// writen:
//-----------------------------------------------------------------------------

ssize_t writen(int fd, const void *vptr, size_t n)

{
	size_t nleft;
	ssize_t nwritten;
        const char *ptr;

	ptr = (char*)vptr;
	nleft = n;

	while(nleft > 0)

	{
		if((nwritten = write(fd, ptr, nleft)) <= 0)

		{
			if(errno == EINTR) {nwritten = 0;}
			else {return -1;}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return n;
}

//-----------------------------------------------------------------------------
// digits:
//-----------------------------------------------------------------------------

int digits(int number)

{
	int digits = 1;
	int step = 10;

	while(step <= number)

	{
		digits++;
		step *= 10;
	}

	return digits;
}

//-----------------------------------------------------------------------------
// addSource:
//-----------------------------------------------------------------------------

int addSource(int workerType, int src_id, char* ip, int port)

{
	return sm->addSource(workerType, src_id, ip, port);
}

//-----------------------------------------------------------------------------
// addDestination:
//-----------------------------------------------------------------------------

int addDestination(int src_id, int dest_id, char* ip, int port)

{
	return sm->addDestination(src_id, dest_id, ip, port);
}
