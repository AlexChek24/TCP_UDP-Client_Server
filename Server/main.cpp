#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <poll.h>
#include <unistd.h>
#include <string.h>


int receiving(int sockfd,std::string& msg){
    int ret_code;
    char M [1024];
    memset(M, 0, sizeof(M));
    ret_code = recv(sockfd, M, sizeof(M), 0);
    if(ret_code <0)
    {
        perror("recv failure");
        return -1;
    }
    msg = M;
    std::cout << "Received TCP: " << msg << std::endl;
    return 0;
}
bool calculation(std::string &msg){
    bool server_stop;
    if (msg == "server stop") {
        msg = "server down";
        server_stop = true;
    } else {
        std::string tmp = msg;
        auto Not_digit = [](char x) { return x < '0' || x > '9'; };
        std::replace_if(msg.begin(), msg.end(), Not_digit, ' ');
        std::vector<uint64_t> numbers;
        std::stringstream ss(msg);
        copy(std::istream_iterator<uint64_t>(ss), {}, back_inserter(numbers));
        if (numbers.size() == 0) {
            msg = tmp;
        } else {
            std::sort(numbers.begin(), numbers.end());
            uint64_t Num_sum = 0;
            for (auto &n:numbers)
                Num_sum += n;
            ss = std::stringstream();
            copy(numbers.begin(), numbers.end(), std::ostream_iterator<uint64_t>(ss, " "));
            msg = ss.str();
            msg = msg + "\n" + "Sum: "+ std::to_string(Num_sum);
            server_stop = false;
        }
    }
    return server_stop;
}

int sending (int sockfd, std::string& msg){
    int ret_code;
    ret_code = send(sockfd,msg.data(),msg.length(),0);
    if(ret_code < 0)
    {
        perror("send failure");
        return -1;
    }
    std::cout<<"Sent TCP: "<<msg<<std::endl;
    return  0;
}


int main() {


    std::cout << "Server start" << std::endl;
    int  err_code, option = 1;
    socklen_t client_adr_len;
    int tcp_listen, tcp_new, udp_fd;
    struct sockaddr_in client_adr,server_adr;
    std::string msg_buf;
    tcp_listen = socket(AF_INET,SOCK_STREAM,0);
    if(tcp_listen<0)
    {
        perror("Server cannot create socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(tcp_listen,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

    server_adr.sin_family =  AF_INET;
    server_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_adr.sin_port = htons(9031);
    char UDP_BUFF [1024];
    err_code = bind(tcp_listen,(struct sockaddr*)&server_adr,sizeof(server_adr));
    if(err_code < 0 )
    {
        perror("Server cannot bind socket");
        exit(EXIT_FAILURE);
    }
    err_code = listen(tcp_listen,4);
    if(err_code < 0 )
    {
        perror("Listen queue");
        exit(EXIT_FAILURE);
    }

    pollfd fd_set [128];
    fd_set [0].fd = tcp_listen;
    fd_set [0].events = POLLIN;
    fd_set [0].revents = 0;
    int size_of_set = 1;
    //UDP
    udp_fd = socket(AF_INET,SOCK_DGRAM,0);
    bind(udp_fd,(struct sockaddr*)&server_adr,sizeof(server_adr));
    fd_set [1].fd = udp_fd;
    fd_set [1].events = POLLIN;
    fd_set [1].revents = 0;
    size_of_set ++;
    bool  server_stop = false;
    while(!server_stop){
        int err_code = poll(fd_set,size_of_set, - 1);
        if(err_code < 0 ){
            perror("poll failure");
            exit(EXIT_FAILURE);
        }
        for(int i=0; i<size_of_set; i++){
            if(fd_set[i].revents & POLLIN)
            {
                fd_set[i].revents &= ~POLLIN;
                if( i == 0)
                { //new connection TCP
                    client_adr_len = sizeof(client_adr);
                    tcp_new = accept(fd_set[i].fd,(struct sockaddr*)&client_adr,&client_adr_len);
                    std::cout<<"New client at port "<<ntohs(client_adr.sin_port)<<std::endl;
                    if(size_of_set <128)
                    {
                        fd_set[size_of_set].fd = tcp_new;
                        fd_set[size_of_set].events = POLLIN;
                        fd_set[size_of_set].revents = 0;
                        size_of_set++;
                    }
                    else
                    {
                        std::cout<<"Connected maximum number of clients" <<std::endl;
                        close(tcp_new);
                    }
                }
                else
                {
                    if(fd_set[i].fd == udp_fd)
                    { //UDP
                        client_adr_len = sizeof(client_adr);
                        bzero(UDP_BUFF,sizeof(UDP_BUFF));
                        err_code = recvfrom(udp_fd,UDP_BUFF,sizeof(UDP_BUFF),0,(struct sockaddr*)&client_adr,(&client_adr_len));
                        msg_buf = UDP_BUFF;
                        std::cout<<"Received UDP: "<<msg_buf<<std::endl;
                        server_stop = calculation(msg_buf);
                        if(msg_buf == "disconnect" || msg_buf == "exit")
                            msg_buf ="disconnected";
                        if(server_stop)
                        {
                            for(int i=2; i<size_of_set; i++)
                            {
                                sending(fd_set[i].fd,msg_buf);
                                close(fd_set[i].fd);
                            }
                        }
                        std::cout<<"Sent UDP: "<<msg_buf<<std::endl;
                        sendto(udp_fd,msg_buf.data(),msg_buf.length(),0,(struct sockaddr*)&client_adr,sizeof(client_adr));
                    }
                    else
                    {//TCP
                        err_code = receiving(fd_set[i].fd, msg_buf);
                        server_stop = calculation(msg_buf);
                        if (err_code < 0)
                        {
                            close(fd_set[i].fd);
                            if (i < size_of_set - 1)
                            {
                                fd_set[i] = fd_set[size_of_set - 1];
                                size_of_set--;
                                i--;
                            }
                        }
                        else
                        {
                            if (msg_buf == "disconnect" || msg_buf == "exit")
                                msg_buf = "disconnected";
                            sending(fd_set[i].fd, msg_buf);
                            if (msg_buf == "disconnected") {
                                close(fd_set[i].fd);
                                if (i < size_of_set - 1) {
                                    fd_set[i] = fd_set[size_of_set - 1];
                                    size_of_set--;
                                    i--;
                                }
                            }
                            if (msg_buf == "server down")
                            {
                                sendto(udp_fd,msg_buf.data(),msg_buf.length(),0,(struct sockaddr*)&client_adr,sizeof(client_adr));
                                close(udp_fd);
                                for (int i = 2; i < size_of_set; i++)
                                {
                                    sending(fd_set[i].fd, msg_buf);
                                    close(fd_set[i].fd);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
