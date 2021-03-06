#include "fsi/FSIDummyAC2CxxProxy.h"
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	
	#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
	#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)
	#pragma comment(lib, "Ws2_32.lib")
#else
  #include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <net/if.h>
	#include <unistd.h>
	#include <arpa/inet.h>
#endif

#include <pthread.h>
#include <ctime>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include "tinyxml_ascodt.h"
#include <hash_map>
#include <vector>


#include "fsi/FSIDummyAImplementation.h"

FSI_FSIDUMMYA_arg daemon_args;

using namespace ascodt;

void writeTime(std::ostream& o) {
  time_t t = time(0);
  struct tm* time = localtime(&t);
  o << time->tm_hour << ":" << time->tm_min << ":" << time->tm_sec;
}

void open_client(const char* hostname,const char* port,
#ifdef _WIN32
SOCKET
#else
int
#endif  
&sockfd,
#ifdef _WIN32
SOCKET
#else
int
#endif  
&newsockfd){
#ifdef _WIN32
		 WSADATA wsaData;
		 int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	     assert (iResult == 0);
#endif
          struct addrinfo *result = NULL,
     	                   hints;
          bzero( &hints, sizeof(hints) );
          hints.ai_family = AF_UNSPEC;
          hints.ai_socktype = SOCK_STREAM;
          hints.ai_protocol = IPPROTO_TCP;
          getaddrinfo(hostname, port, &hints, &result);
          sockfd = socket(result->ai_family, result->ai_socktype, 
            result->ai_protocol);
          assert(sockfd >= 0);
          
          int tries=60;
                while(tries>0){
                 if (connect(sockfd, result->ai_addr, (int)result->ai_addrlen)==0)
                    break;
                         tries--;
#ifdef _WIN32
						 Sleep(1000);
#else
						 sleep(1);
#endif	
                }   
         newsockfd=sockfd;
}

void bind_server(const char* port,
#ifdef _WIN32
SOCKET
#else
int
#endif 
&sockfd,
const int numberOfWorkers
){


#ifdef _WIN32
		 WSADATA wsaData;
		 int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	     assert (iResult == 0);
#endif
        
     int sockd, sockd2;
       int addrlen;
       struct sockaddr_in my_name, peer_name;
       int status;

       /* create a socket */
       sockd = socket(AF_INET, SOCK_STREAM, 0);
       if (sockd < 0)
       {
         perror("Socket creation error");
         exit(1);
       }


       /* server address  */
       my_name.sin_family = AF_INET;
       my_name.sin_addr.s_addr = INADDR_ANY;
       my_name.sin_port = htons(atoi(port));
#ifdef _WIN32
       BOOL yes=TRUE;
#else
       int yes=1;
#endif
       setsockopt(sockd, SOL_SOCKET, SO_REUSEADDR,
#ifdef _WIN32
       (char const*)&yes,
#else
       &yes,
#endif
       sizeof(int));
       status = bind(sockd, (struct sockaddr*)&my_name, sizeof(my_name));
       
       if (status < 0)
       {
         perror("Binding error");
         exit(1);
       }

       status = listen(sockd, numberOfWorkers);
       
       if (status < 0)
       {
         perror("Listening error");
         exit(1);
       }
       
       sockfd = sockd;
}

void accept_on_server(
#ifdef _WIN32
SOCKET&
#else
int&
#endif
sockfd,
#ifdef _WIN32
SOCKET&
#else
int&
#endif
clientfd
){
          clientfd = accept(sockfd, NULL, NULL);
          assert (clientfd >= 0);
}

void sendData(char* data, size_t numberOfBytes, char* sendBuffer,
#ifdef _WIN32
SOCKET
#else
int
#endif 
newsockfd,int bufferSize){
     char* data_ptr=(char*)data;
     int remaining_bytes_to_send=0,total_send_bytes=0,send_bytes=0,n=0;

     //clear buffer
     bzero(sendBuffer,bufferSize);
     while(total_send_bytes<numberOfBytes){
          remaining_bytes_to_send=(numberOfBytes-total_send_bytes<=bufferSize)?numberOfBytes-total_send_bytes:bufferSize;
          memcpy(sendBuffer,data_ptr,remaining_bytes_to_send);
          send_bytes=0;
          char* send_buffer_ptr=sendBuffer;
          while(send_bytes<bufferSize){
               n = 
#ifdef _WIN32
				   send(
#else

				   write(
#endif
				   newsockfd,send_buffer_ptr,bufferSize-send_bytes
#ifdef _WIN32
				   ,0
#else

#endif
				   );
               if(n>0){
                    send_bytes+=n;
                    send_buffer_ptr+=n;
               }
          }
          total_send_bytes+=send_bytes;
          data_ptr+=send_bytes;
     }

}

#ifdef Parallel
void broadcastParallelData(char* data,size_t size_of_data,MPI_Comm newsockfd){
          MPI_Status status;
           MPI_Bcast(data, size_of_data, MPI_BYTE, 0, newsockfd);
          
}
#endif

void readData(char* data,size_t size_of_data,char* readBuffer,
#ifdef _WIN32
SOCKET
#else
int
#endif 
newsockfd, int bufferSize){
          bzero(readBuffer,bufferSize);
          int bytes_received=0;
          int total_bytes_received=0;
          int local_bytes_received=0;
          int bytes_to_copy=0;
          char* data_ptr=data;
          char* buffer_ptr;

          while(total_bytes_received<size_of_data){
               bytes_received=0;
               buffer_ptr=readBuffer;
               while(bytes_received<bufferSize){
                    local_bytes_received = 
#ifdef _WIN32
						recv(
#else
						read(
#endif
							newsockfd,buffer_ptr,bufferSize-bytes_received
#ifdef _WIN32
							,0
#else
#endif
							);
                    if(local_bytes_received>0){
                         bytes_received+=local_bytes_received;
                         buffer_ptr+=local_bytes_received;
                    }
               }
               bytes_to_copy=(total_bytes_received+bytes_received>size_of_data)?size_of_data-total_bytes_received:bytes_received;
               memcpy(data_ptr,readBuffer,bytes_to_copy);
               data_ptr+=bytes_to_copy;
               total_bytes_received+=bytes_to_copy;
          }

}

void invoker_create_instance(void** ref,int,int,char*,char*
#ifdef Parallel
	 ,MPI_Comm communicator,int methodID
#endif  
){
     *ref = new fsi::FSIDummyAImplementation();
}

void invoker_destroy_instance(void** ref,int,int,char*,char*
#ifdef Parallel
	 ,MPI_Comm communicator,int methodID
#endif  
){
      delete ((fsi::FSIDummyAImplementation*)*ref);
  
}
#include "fsi/FSICommNativeSocketDispatcher.h"
#include "fsi/FSICommCxx2SocketPlainPort.h"
void invoker_create_client_port_for_b(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
  long long portref=0;
  int port;
  int hostname_len=0;
  char* host;
  readData((char*)&hostname_len,sizeof(int),rcvBuffer,newsockfd,buffer_size);
  host = new char[hostname_len];
  readData((char*)host,hostname_len,rcvBuffer,newsockfd,buffer_size);
  readData((char*)&port,sizeof(int),rcvBuffer,newsockfd,buffer_size);

  portref=(long long)new fsi::FSICommCxx2SocketPlainPort(
        host,
        port,
        buffer_size
   );

  delete [] host;
  sendData((char*)&portref,sizeof(long long),sendBuffer,newsockfd,buffer_size);
 
}
  
void invoker_create_client_port_for_b(void** ref, void** dispatcherRef, void** portRef, char* host,int port,int buffer_size){
  
  *portRef=new fsi::FSICommCxx2SocketPlainPort(
        host,
        port,
        buffer_size
   );
  
}
void invoker_connect_client_dispatcher_b(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
  long long portref=0;
  int port;
  int hostname_len=0;
  char* host;
  readData((char*)&hostname_len,sizeof(int),rcvBuffer,newsockfd,buffer_size);
  host = new char[hostname_len];

  readData((char*)host,hostname_len,rcvBuffer,newsockfd,buffer_size);
  readData((char*)&port,sizeof(int),rcvBuffer,newsockfd,buffer_size);
  portref=(long long)new fsi::FSICommNativeSocketDispatcher(
           host,
           port,
           buffer_size
     );

  ((fsi::FSIDummyAImplementation*)*ref)->connectb((fsi::FSICommNativeSocketDispatcher*)portref);   
  delete[]host;
  sendData((char*)&portref,sizeof(long long),sendBuffer,newsockfd,buffer_size);
}

void invoker_connect_client_dispatcher_b(void** ref,void** dispatcherRef, void** portRef, char* host,int port,int buffer_size){
  if(*dispatcherRef==NULL){
     *dispatcherRef=new fsi::FSICommNativeDispatcher();
     ((fsi::FSIDummyAImplementation*)(*ref))->connectb((fsi::FSICommNativeDispatcher*) *dispatcherRef);   
  }
  ((fsi::FSICommNativeDispatcher*) (*dispatcherRef))->connect((fsi::FSIComm*)(*portRef));
}




void invoker_disconnect_client_dispatcher_b(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
 ((fsi::FSIDummyAImplementation*)*ref)->disconnectb();
}

void invoker_disconnect_client_dispatcher_b(void** ref,void** dispatcherRef, void** portRef, char* host,int port,int buffer_size){
 ((fsi::FSICommNativeDispatcher*)*dispatcherRef)->disconnect((fsi::FSIComm*)(*portRef));
}

void invoker_dataAck(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
  int ack;
readData((char*)&ack,sizeof(int),rcvBuffer,newsockfd,buffer_size);

  ((fsi::FSIDummyAImplementation*)*ref)->dataAck(ack);
  sendData((char*)&ack,sizeof(int),sendBuffer,newsockfd,buffer_size);

}


void parallel_master_invoker_dataAck(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
 	
  int ack;
readData((char*)&ack,sizeof(int),rcvBuffer,newsockfd,buffer_size);

  #ifdef Parallel
  broadcastParallelData((char*)&methodId,sizeof(int),communicator);
  broadcastParallelData((char*)&ack,sizeof(int),communicator);

  #endif
  ((fsi::FSIDummyAImplementation*)*ref)->dataAck(ack);
  //int ack=1;
  //sendData((char*)&ack,sizeof(int),sendBuffer,newsockfd,buffer_size);
}
void parallel_worker_invoker_dataAck(void** ref
#ifdef Parallel
,MPI_Comm newsockfd
#endif
){
  #ifdef Parallel
  int ack;
broadcastParallelData((char*)&ack,sizeof(int),newsockfd);

  ((fsi::FSIDummyAImplementation*)*ref)->dataAck(ack);
  #endif		  
} 
void invoker_transferData(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
  int coordId_len=0;
readData((char*)&coordId_len,sizeof(int),rcvBuffer,newsockfd,buffer_size);
int* coordId=new int[coordId_len];
readData((char*)coordId,sizeof(int)*coordId_len,rcvBuffer,newsockfd,buffer_size);
int data_len=0;
readData((char*)&data_len,sizeof(int),rcvBuffer,newsockfd,buffer_size);
double* data=new double[data_len];
readData((char*)data,sizeof(double)*data_len,rcvBuffer,newsockfd,buffer_size);

  ((fsi::FSIDummyAImplementation*)*ref)->transferData(coordId,coordId_len,data,data_len);
  delete [] coordId;
delete [] data;

}


void parallel_master_invoker_transferData(void** ref,int newsockfd, int buffer_size,char* rcvBuffer, char* sendBuffer
#ifdef Parallel
,MPI_Comm communicator, int methodId
#endif
){
 	
  int coordId_len=0;
readData((char*)&coordId_len,sizeof(int),rcvBuffer,newsockfd,buffer_size);
int* coordId=new int[coordId_len];
readData((char*)coordId,sizeof(int)*coordId_len,rcvBuffer,newsockfd,buffer_size);
int data_len=0;
readData((char*)&data_len,sizeof(int),rcvBuffer,newsockfd,buffer_size);
double* data=new double[data_len];
readData((char*)data,sizeof(double)*data_len,rcvBuffer,newsockfd,buffer_size);

  #ifdef Parallel
  broadcastParallelData((char*)&methodId,sizeof(int),communicator);
  broadcastParallelData((char*)&coordId_len,sizeof(int),communicator);
broadcastParallelData((char*)coordId,sizeof(int)*coordId_len,communicator);
broadcastParallelData((char*)&data_len,sizeof(int),communicator);
broadcastParallelData((char*)data,sizeof(double)*data_len,communicator);

  #endif
  ((fsi::FSIDummyAImplementation*)*ref)->transferData(coordId,coordId_len,data,data_len);
  //int ack=1;
  //sendData((char*)&ack,sizeof(int),sendBuffer,newsockfd,buffer_size);
}
void parallel_worker_invoker_transferData(void** ref
#ifdef Parallel
,MPI_Comm newsockfd
#endif
){
  #ifdef Parallel
  int coordId_len=0;
broadcastParallelData((char*)&coordId_len,sizeof(int),newsockfd);
int* coordId=new int[coordId_len];
broadcastParallelData((char*)coordId,sizeof(int)*coordId_len,newsockfd);
int data_len=0;
broadcastParallelData((char*)&data_len,sizeof(int),newsockfd);
double* data=new double[data_len];
broadcastParallelData((char*)data,sizeof(double)*data_len,newsockfd);

  ((fsi::FSIDummyAImplementation*)*ref)->transferData(coordId,coordId_len,data,data_len);
  #endif		  
} 

void close(
#ifdef _WIN32
SOCKET
#else
int
#endif
&sockfd,
#ifdef _WIN32
SOCKET
#else
int
#endif
&newsockfd){
#ifdef _WIN32
	 if(newsockfd>=0)
         closesocket(newsockfd);
     if(sockfd>=0)
         closesocket(sockfd);
#else

     if(newsockfd>=0)
         close(newsockfd);
     if(sockfd>=0)
         close(sockfd);
#endif
}

#ifndef _WIN32
std::string retrieveSocketAddress(){
	std::stringstream res;
	  int rank = 0 ;
#ifdef Parallel
  
     MPI_Comm_rank(MPI_COMM_WORLD,&rank);
#endif
	int _sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(_sockfd>=0);
	const char* network_interface = getenv("FSI_FSIDUMMYA_NET_INTERFACE");
	if(network_interface==NULL)
		network_interface="lo";	
	struct if_nameindex *curif, *ifs;
	struct ifreq req;
	ifs = if_nameindex();
	if(ifs) {
		for(curif = ifs; curif && curif->if_name; curif++) {
			strncpy(req.ifr_name, curif->if_name, IFNAMSIZ);
			req.ifr_name[IFNAMSIZ] = 0;
			if (ioctl(_sockfd, SIOCGIFADDR, &req) < 0)
				perror("ioctl");
			else{
				printf("%s: [%s]\n", curif->if_name,
                                                        inet_ntoa(((struct sockaddr_in*) &req.ifr_addr)->sin_addr));
				if(strcmp(curif->if_name,network_interface)==0)
				{
					res<<std::string(inet_ntoa(((struct sockaddr_in*) &req.ifr_addr)->sin_addr))<<":"<<daemon_args.daemon_port;
					
				}
			}
		}
		if_freenameindex(ifs);
	}
	close(_sockfd);
	return res.str();
}
#endif

void socket_worker_loop(void* ref,
#ifdef _WIN32
SOCKET
#else
int
#endif
clientfd,int bufferSize
#ifdef Parallel
,MPI_Comm communicator
#endif
){
     char *sendBuffer=new char[bufferSize];
     char *rcvBuffer=new char[bufferSize];
     void (*invokers[18])(void**,int,int,char*,char*
#ifdef Parallel
	 ,MPI_Comm,int
#endif     
     );
     invokers[0]=invoker_create_instance;
     invokers[1]=invoker_destroy_instance;
     int methodId=0;
     invokers[4]=invoker_disconnect_client_dispatcher_b;
invokers[3]=invoker_connect_client_dispatcher_b;
invokers[2]=invoker_create_client_port_for_b;
invokers[17]=parallel_master_invoker_dataAck;
invokers[16]=invoker_dataAck;
invokers[15]=parallel_master_invoker_transferData;
invokers[14]=invoker_transferData;

     
     while(methodId!=1){
          readData((char*)&methodId,sizeof(int),rcvBuffer,clientfd,bufferSize);
          invokers[methodId](&ref,clientfd,bufferSize,rcvBuffer,sendBuffer
#ifdef Parallel
	 		,communicator,methodId
#endif            
          );
     }
     delete [] sendBuffer;
     delete [] rcvBuffer;
#ifdef _WIN32
     closesocket(clientfd);
#else
     close(clientfd);    
#endif
}

#ifdef Parallel
void parallel_worker_loop(void* ref,
MPI_Comm clientfd){
     void (*parallel_worker_invokers[18])(void**,MPI_Comm);
     int methodId=0;
     parallel_worker_invokers[16]=parallel_worker_invoker_dataAck;
parallel_worker_invokers[14]=parallel_worker_invoker_transferData;

     while(methodId!=1){
          broadcastParallelData((char*)&methodId,sizeof(int),clientfd);
          parallel_worker_invokers[methodId-1](&ref,clientfd);
     }
}

#endif

#ifdef Parallel
void* parallel_deamon_run(void* daemon_args){
	parallel_worker_loop(((FSI_FSIDUMMYA_arg*)daemon_args)->ref,((FSI_FSIDUMMYA_arg*)daemon_args)->communicator);
}
#endif
 
//#ifdef _WIN32
//DWORD WINAPI server_deamon_run(void* daemon_args){
//      SOCKET clientfd;
//#else  
//void* server_deamon_run(void* daemon_args){
//      int clientfd=0;
//#endif

void* server_deamon_run(void* daemon_args){
#ifdef _WIN32
      SOCKET clientfd;
#else  
      int clientfd=0;
#endif
      accept_on_server(((FSI_FSIDUMMYA_arg*)daemon_args)->daemon_serverfd,clientfd);
      std::cout<<"server accepted"<<std::endl;
      socket_worker_loop(
      	((FSI_FSIDUMMYA_arg*)daemon_args)->ref,
      	clientfd,
      	((FSI_FSIDUMMYA_arg*)daemon_args)->buffer_size
#ifdef Parallel
		,((FSI_FSIDUMMYA_arg*)daemon_args)->communicator
#endif      	
      	);
}
void startMPIDaemon(FSI_FSIDUMMYA_arg& arg){
#ifdef Parallel	 
int rank = -1;
MPI_Comm_rank(MPI_COMM_WORLD,&rank);
if(rank>0){
	  pthread_t task;
       //tasks.push_back(task);
       pthread_create(&task,NULL,parallel_deamon_run,&arg);
	 //parallel_deamon_run(&arg);
}
#endif     
}

void startSocketDaemons(FSI_FSIDUMMYA_arg& arg){
     std::vector<pthread_t> tasks;
  
     for(int i=0;i<arg.number_of_workers;i++){
     //#ifdef _WIN32    
     //     CreateThread(NULL, 0,server_deamon_run, &arg, 0, NULL);
     //#else     
          pthread_t task;
          tasks.push_back(task);
          pthread_create(&task,NULL,server_deamon_run,&arg);
     //#endif
     }
   
}

void initialiseENV(FSI_FSIDUMMYA_arg& arg){
          const char* client_port = getenv("FSI_FSIDUMMYA_PORT");
          const char* daemon_port = getenv("FSI_FSIDUMMYA_DAEMON_PORT");
          const char* buffer_size = getenv("FSI_FSIDUMMYA_BUFFER_SIZE");
          const char* hostname = getenv("FSI_FSIDUMMYA_HOSTNAME");
          const char* java_client_flag = getenv("FSI_FSIDUMMYA_JAVA");
          const char* number_of_workers = getenv("FSI_FSIDUMMYA_WORKERS");
          const char* xmlFile = getenv("FSI_FSIDUMMYA_XML");
          if(buffer_size!=NULL)
               arg.buffer_size = atoi(buffer_size);
          if(hostname!=NULL)
               arg.hostname = hostname;
          if(client_port!=NULL)
               arg.client_port = client_port;
          if(daemon_port!=NULL)
               arg.daemon_port = daemon_port;
          if(java_client_flag!=NULL)
               arg.java_client_flag = (strcmp(java_client_flag,"off")==0)?false:true;
          if(number_of_workers!=NULL)
               arg.number_of_workers = atoi(number_of_workers);
          if(xmlFile!=NULL)
               arg.xml=xmlFile;
               
}

void initialiseXMLDaemons(FSI_FSIDUMMYA_arg& arg){
    int rank=0;
   
    if(arg.xml!=NULL){
          tinyxml2::XMLDocument confFile;
          confFile.LoadFile(arg.xml);
          tinyxml2::XMLElement* root = confFile.FirstChildElement("diagram");
          __gnu_cxx::hash_map<int,std::vector<int> > connections;
          __gnu_cxx::hash_map<int,int> componentPorts;
          __gnu_cxx::hash_map<int,std::string> componentHosts;
          __gnu_cxx::hash_map<int,void*> dispatchers;
          void (*invokers[18])(void**,void**,void**,char* host,int port,int buffer_size);
          
           invokers[4]=invoker_disconnect_client_dispatcher_b;
invokers[3]=invoker_connect_client_dispatcher_b;
invokers[2]=invoker_create_client_port_for_b;

          for(tinyxml2::XMLElement* e = root->FirstChildElement("component"); e != NULL; e = e->NextSiblingElement("component"))
          {
            if(strcmp(e->Attribute("name"),"fsi.FSIDummyA")==0){
                 int port;
                 std::stringstream str;
                 e->QueryIntAttribute("port",&port);
                 str<<(port+1);
                 arg.daemon_port=str.str();
                 for(
                           tinyxml2::XMLElement* conElement = e->FirstChildElement("outputPort");
                           conElement != NULL;
                           conElement = conElement->NextSiblingElement("outputPort"))
                 {
                      int key=0;
                      int createId=0,connectId=0,disconnectId=0;

                      conElement->QueryIntAttribute("index",&key);
                      conElement->QueryIntAttribute("createId",&createId);
                      conElement->QueryIntAttribute("connectId",&connectId);
                      conElement->QueryIntAttribute("disconnectId",&disconnectId);
                      connections[key].resize(3);
                      connections[key][0]=createId;
                      connections[key][1]=connectId;
                      connections[key][2]=disconnectId;

                 }
            }else{
                 int port=0;
                 e->QueryIntAttribute("port",&port);
                 const char* hostname=e->Attribute("host");
                 for(
                           tinyxml2::XMLElement* conElement = e->FirstChildElement("inputPort");
                           conElement != NULL;
                           conElement = conElement->NextSiblingElement("inputPort"))
                 {
                     int key=0;
                     conElement->QueryIntAttribute("index",&key);
                     componentPorts[key]=port+1;
                     componentHosts[key]=hostname;
                 }
            }
          }
      
     }
}
void initialiseXMLConnections(FSI_FSIDUMMYA_arg& arg){
    int rank=0;
   
    if(arg.xml!=NULL){
          tinyxml2::XMLDocument confFile;
          confFile.LoadFile(arg.xml);
          tinyxml2::XMLElement* root = confFile.FirstChildElement("diagram");
          __gnu_cxx::hash_map<int,std::vector<int> > connections;
          __gnu_cxx::hash_map<int,int> componentPorts;
          __gnu_cxx::hash_map<int,std::string> componentHosts;
          __gnu_cxx::hash_map<int,void*> dispatchers;
          void (*invokers[18])(void**,void**,void**,char* host,int port,int buffer_size);
          
           invokers[4]=invoker_disconnect_client_dispatcher_b;
invokers[3]=invoker_connect_client_dispatcher_b;
invokers[2]=invoker_create_client_port_for_b;

          for(tinyxml2::XMLElement* e = root->FirstChildElement("component"); e != NULL; e = e->NextSiblingElement("component"))
          {
            if(strcmp(e->Attribute("name"),"fsi.FSIDummyA")==0){
                 int port;
                 std::stringstream str;
                 e->QueryIntAttribute("port",&port);
                 str<<(port+1);
                // arg.daemon_port=str.str();
                 for(
                           tinyxml2::XMLElement* conElement = e->FirstChildElement("outputPort");
                           conElement != NULL;
                           conElement = conElement->NextSiblingElement("outputPort"))
                 {
                      int key=0;
                      int createId=0,connectId=0,disconnectId=0;

                      conElement->QueryIntAttribute("index",&key);
                      conElement->QueryIntAttribute("createId",&createId);
                      conElement->QueryIntAttribute("connectId",&connectId);
                      conElement->QueryIntAttribute("disconnectId",&disconnectId);
                      connections[key].resize(3);
                      connections[key][0]=createId;
                      connections[key][1]=connectId;
                      connections[key][2]=disconnectId;

                 }
            }else{
                 int port=0;
                 e->QueryIntAttribute("port",&port);
                 const char* hostname=e->Attribute("host");
                 for(
                           tinyxml2::XMLElement* conElement = e->FirstChildElement("inputPort");
                           conElement != NULL;
                           conElement = conElement->NextSiblingElement("inputPort"))
                 {
                     int key=0;
                     conElement->QueryIntAttribute("index",&key);
                     componentPorts[key]=port+1;
                     componentHosts[key]=hostname;
                 }
            }
          }
       #ifdef Parallel
    
       MPI_Comm_rank(MPI_COMM_WORLD,&rank);
       if(rank==0){
          #endif
          for(tinyxml2::XMLElement* e = root->FirstChildElement("connection"); e != NULL; e = e->NextSiblingElement("connection"))
          {
            int source=-1;
            int destination=-1;
            e->QueryIntAttribute("source",&source);
            e->QueryIntAttribute("destination",&destination);
            __gnu_cxx::hash_map<int,std::vector<int> >::iterator itSource = connections.find(source);
            __gnu_cxx::hash_map<int,std::string >::iterator itDestination = componentHosts.find(destination);
            if(itSource!=connections.end()&&itDestination!=componentHosts.end()){
                 std::cout<<"establish connection from xml id:"<< source<<" host:"<<
                    componentHosts[destination]<<" port:"<<componentPorts[destination]<<std::endl;
                 if(dispatchers.find(source)==dispatchers.end())
                     dispatchers[source]=NULL;
                 void* port =NULL;
                 invokers[(*itSource).second[0]](
                          &arg.ref,
                          NULL,
                          &port,
                          (char*)componentHosts[destination].c_str(),
                          componentPorts[destination],
                          arg.buffer_size);
                 invokers[(*itSource).second[1]](
                               &arg.ref,
                               &dispatchers[source],
                               &port,
                              (char*)componentHosts[destination].c_str(),
                               componentPorts[destination],
                               arg.buffer_size);
            }
          }
       #ifdef Parallel
       }
       #endif
     }
}

void initialiseParallel(FSI_FSIDUMMYA_arg& arg){
#ifdef Parallel
     int rank = -1 ;
     int comm_size;
     std::stringstream st;

     MPI_Comm_rank(MPI_COMM_WORLD,&rank);
     MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
     int port=atoi(arg.daemon_port.c_str())+rank;
     st<<port;
     arg.communicator = MPI_COMM_WORLD;
     arg.daemon_port=st.str();
     if(rank>0){
          arg.java_client_flag=false;
          arg.joinable=false;
     }
      
#endif
}
extern "C"{

#ifdef _WIN32
void INITIALISE(FSI_FSIDUMMYA_arg& arg,bool joinable){
#else
void initialise_(FSI_FSIDUMMYA_arg& arg,bool joinable){
#endif
     arg.buffer_size = 4096;
     arg.hostname = "127.0.0.1";
     arg.client_port = "50000";
     arg.daemon_port = "50001";
     arg.java_client_flag = true;
     arg.number_of_workers = 10;
     arg.xml=NULL;
     arg.joinable=joinable;
     invoker_create_instance(&arg.ref,0,0,NULL,NULL
#ifdef Parallel
   ,MPI_COMM_WORLD,0     
#endif   
   );
     initialiseENV(arg);
     
     initialiseXMLDaemons(arg);
     initialiseParallel(arg);


   
   /*if(arg.java_client_flag)         
     open_client(arg.hostname.c_str(),arg.client_port.c_str(),arg.java_serverfd,arg.java_clientfd);

   bind_server(arg.daemon_port.c_str(),arg.daemon_serverfd,arg.number_of_workers);
   startSocketDaemons(arg);
   initialiseXMLConnections(arg);
   if(arg.joinable)
     server_deamon_run(&arg);
   startMPIDaemon(arg);*/
}


#ifdef _WIN32
void BIND_COMPONENT(FSI_FSIDUMMYA_arg& arg,bool joinable){
#else
void bind_component_(FSI_FSIDUMMYA_arg& arg,bool joinable){
#endif
   if(arg.java_client_flag)         
     open_client(arg.hostname.c_str(),arg.client_port.c_str(),arg.java_serverfd,arg.java_clientfd);

   bind_server(arg.daemon_port.c_str(),arg.daemon_serverfd,arg.number_of_workers);
   startSocketDaemons(arg);
   initialiseXMLConnections(arg);
   if(arg.joinable)
     server_deamon_run(&arg);
   startMPIDaemon(arg);
}


#ifdef _WIN32
void DESTROY(FSI_FSIDUMMYA_arg& arg, bool joinable){
#else
void destroy_(FSI_FSIDUMMYA_arg& arg, bool joinable){
#endif
if(joinable){
#ifdef _WIN32
  closesocket(arg.daemon_serverfd);
  if(arg.java_client_flag)
     closesocket(arg.java_serverfd);    
#else
  close(arg.daemon_serverfd);
  if(arg.java_client_flag)
     close(arg.java_serverfd);
#endif
  
  
   
#ifdef _WIN32
  WSACleanup();
#endif   
}
}

#ifdef _WIN32
void SOCKET_LOOP(FSI_FSIDUMMYA_arg& arg,bool joinable){
#else
void socket_loop_(FSI_FSIDUMMYA_arg& arg,bool joinable){
#endif
  if(arg.java_client_flag)       
     socket_worker_loop(arg.ref,arg.java_clientfd,arg.buffer_size
#ifdef Parallel
     ,arg.communicator
#endif     
     );
}

#ifdef _WIN32
void MAIN_LOOP(bool joinable){
#else
void main_loop_(bool joinable){
#endif
  

  
#ifdef _WIN32
  INITIALISE(daemon_args,joinable);
  SOCKET_LOOP(daemon_args,joinable);
  DESTROY(daemon_args,joinable);     
#else  
  initialise_(daemon_args,joinable);
  bind_component_(daemon_args,joinable);
  socket_loop_(daemon_args,joinable);
  destroy_(daemon_args,joinable);  
#endif
  
}

}







