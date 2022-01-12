
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <string>


#define PORT 9031


struct arg {
    sockaddr_in serv;
    int sock;
    bool TCP_or_UDP;
};
int send_or_sendto(arg& connection_info,std::string& msg_buf){
    int s;
    bool send_stop = false;
    std::string msg;


    std::cout<<std::endl<<"Enter data>>";
    getline(std::cin,msg);
    while(msg.size()==0)
    {

        std::cout<<"Enter a non-empty string"<<std::flush;
        std::cout<<std::endl<<"Enter data>>";
        getline(std::cin,msg);
    }
    if(connection_info.TCP_or_UDP ==0) {
        s = send(connection_info.sock, msg.data(), msg.length(), 0);
        if (s < 0) {
            perror("send failure");
            return -1;
        }
    }
    else {
        s = sendto(connection_info.sock, msg.data(), msg.length(), 0,
                   (const struct sockaddr *) &connection_info.serv, sizeof(connection_info.serv));
        if (s < 0) {
            perror("sendto failure");
            return -1;
        }
    }
    return  0;
}

int recv_or_recvfrom (arg& connection_info,std::string& msg_buf) {

    int r;
    bool recv_stop = false;
    char M[1024];
    std::string msg;
    memset(M, 0, sizeof(M));
    if (connection_info.TCP_or_UDP == 0) {
        r = recv(connection_info.sock, M, sizeof(M), 0);
        if (r < 0) {
            perror("recv failure");
            return -1;
        }
    }
    else {
        socklen_t len = sizeof connection_info.sock;
        r = recvfrom(connection_info.sock, M, sizeof(M), 0, (struct sockaddr *) &connection_info.serv,
                     &len);
        if (r < 0) {
            perror("recvfrom failure");
            return -1;
        }
    }
    msg = M;
    std::cout << msg << std::endl;
    if (msg == "server down" || msg == "disconnected")
        return -1;
    return  0;
}

int main(int argc, char** argv){
    int socket_fd;
    int err_code;
    std::string selected_mode = argv[1];
    struct sockaddr_in server_adr;
    bool TCP_or_UDP;
    if(selected_mode=="TCP"){
        std::cout<<"TCP mode selected"<<std::endl;
        if((socket_fd = socket(AF_INET,SOCK_STREAM,0))<0){
            std::cout<<"socket creation failed";
            exit(0);
        }
        TCP_or_UDP = 0;
    }
    if(selected_mode=="UDP") {
        std::cout<<"UDP mode selected"<<std::endl;
        if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            std::cout << "socket creation failed";
            exit(0);
        }
        TCP_or_UDP = 1;
    }
    if(selected_mode!= "TCP" && selected_mode!="UDP"){
        std::cout<<"None of the modes are selected";
        exit(0);
    }

    server_adr.sin_family=AF_INET;
    server_adr.sin_port =htons(PORT);
    server_adr.sin_addr.s_addr=inet_addr("127.0.0.1");
    err_code = connect(socket_fd,(const struct sockaddr*) &server_adr,sizeof(server_adr));
    struct arg connection_info;
    connection_info.TCP_or_UDP = TCP_or_UDP;
    connection_info.sock = socket_fd;
    connection_info.serv = server_adr;
    std::string msg;
    while(1) {
        if(send_or_sendto(connection_info, msg) < 0) break;
        if(recv_or_recvfrom(connection_info, msg) < 0) break;
    }
    return 0;
}