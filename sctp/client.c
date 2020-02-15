# include <stdio.h>
# include <stdlib.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netinet/sctp.h> //Insert
# include <netdb.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h> //Insert
# include <signal.h> //Insert
# include <sys/ioctl.h> //Insert
# include <errno.h>

/*
 * (c) 2011 dermesser
 * This piece of software is licensed with GNU GPL 3.
 *
 */
 
 //--------------Insert------------------------
 int i;
 int counter = 0;
 int asconf = 1;
 int ret;
 int addr_count;
 char address[16];
 sctp_assoc_t id;
 struct sockaddr_in addr;
 struct sctp_status status;
 struct sctp_initmsg initmsg;
 struct sctp_event_subscribe events;
 struct sigaction sig_handler;
 struct sctp_paddrparams heartbeat;
 struct sctp_rtoinfo rtoinfo;

void handle_signal(int signum);

//-----------------------------------------------

const char* help_string = "Usage: simple-http [-h] [-4|-6] [-p PORT] [-o OUTPUT_FILE] SERVER FILE\n";

void errExit(const char* str, char p)
{
	if ( p != 0 )
	{
		perror(str);
	} else
	{
		fprintf(stderr,str);
	}
	exit(1);
}

//----------Insert---------------------
int srvfd;
void handle_signal(int signum)
{
    switch(signum)
    {
        case SIGINT:
            if(close(srvfd) != 0)
                perror("close");
            exit(0);
            break;  
        default: exit(0);
            break;
    }
}
//---------------------------------------

int main (int argc, char** argv)
{
	struct addrinfo *result, hints;
	int  rwerr = 42, outfile, ai_family = AF_UNSPEC;
	char *request, buf[16], port[6],c;

	memset(port,0,6);

	if ( argc < 3 )
		errExit(help_string,0);
	
	strncpy(port,"80",2);

	while ( (c = getopt(argc,argv,"p:ho:46")) >= 0 )
	{
		switch (c)
		{
			case 'h' :
				errExit(help_string,0);
			case 'p' :
				strncpy(port,optarg,5);
				break;
			case 'o' :
				outfile = open(optarg,O_WRONLY|O_CREAT,0644);
				close(1);
				dup2(outfile,1);
				break;
			case '4' :
				ai_family = AF_INET;
				break;
			case '6' :
				ai_family = AF_INET6;
				break;
		}
	}
	//-----------------Insert---------------------
	memset(&hints,0,sizeof(struct addrinfo));
	memset(&events,     1,   sizeof(struct sctp_event_subscribe));
    memset(&status,     0,   sizeof(struct sctp_status));
    memset(&heartbeat,  0,   sizeof(struct sctp_paddrparams));
    memset(&rtoinfo,    0, sizeof(struct sctp_rtoinfo));
	hints.ai_family = ai_family;
	hints.ai_socktype = SOCK_STREAM;
	
	//----------------------------------------------

	if ( 0 != getaddrinfo(argv[optind],port,&hints,&result))
		errExit("getaddrinfo",1);

	// Create socket after retrieving the inet protocol to use (getaddrinfo)
	srvfd = socket(result->ai_family,SOCK_STREAM,IPPROTO_SCTP);// Change to IPPROTO_SCTP
	//---------------Insert---------------------------
	heartbeat.spp_flags = SPP_HB_ENABLE;
    heartbeat.spp_hbinterval = 5000;
    heartbeat.spp_pathmaxrxt = 1;

    rtoinfo.srto_max = 2000;

    sig_handler.sa_handler = handle_signal;
    sig_handler.sa_flags = 0;

    /*Handle SIGINT in handle_signal Function*/
    if(sigaction(SIGINT, &sig_handler, NULL) == -1)
        perror("sigaction");

    
    /*Configure Heartbeats*/
    if((ret = setsockopt(srvfd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat, sizeof(heartbeat))) != 0)
        perror("setsockopt");

    /*Set rto_max*/
    if((ret = setsockopt(srvfd, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo))) != 0)
        perror("setsockopt");

    /*Set SCTP Init Message*/
    if((ret = setsockopt(srvfd, SOL_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg))) != 0)
        perror("setsockopt");

    /*Enable SCTP Events*/
    if((ret = setsockopt(srvfd, SOL_SCTP, SCTP_EVENTS, (void *)&events, sizeof(events))) != 0)
        perror("setsockopt");

    /*Get And Print Heartbeat Interval*/
    i = (sizeof heartbeat);
    getsockopt(srvfd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &heartbeat, (socklen_t*)&i);

    printf("Heartbeat interval %d\n", heartbeat.spp_hbinterval);
	
	//----------------------------------------------

    /*Connect to Host*/
    	
	if ( connect(srvfd,result->ai_addr,result->ai_addrlen) == -1)
		errExit("connect",1);
	
	
	// Now we have an established connection.
	
	// XXX: Change the length if request string is modified!!!
	request = calloc(53+strlen(argv[optind+1])+strlen(argv[optind]),1);

	sprintf(request,"GET %s HTTP/1.1\nHost: %s\nUser-agent: simple-http client\n\n",argv[optind+1],argv[optind]);

	write(srvfd,request,strlen(request));
	
	shutdown(srvfd,SHUT_WR);

	while ( rwerr > 0 )
	{
		rwerr = read(srvfd,buf,256);
		write(1,buf,rwerr);
	}
	
	close(srvfd);

	return 0;

}