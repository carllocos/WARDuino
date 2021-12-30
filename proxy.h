#ifndef PROXY_H
#define PROXY_H

class ProxyHost {

    private:
        // for singleton
        static ProxyHost *proxyHost;

        char *host;
        int port, sockfd;

        struct ServerData;
        struct ServerData *data;

        ProxyHost();
    public:
        char* exceptionMsg;

        void registerHost(char * t_host, int t_port);
        void closeConnection(void);
        bool openConnection(void);
        void updateExcpMsg(const char * msg);
        bool send(void *t_buffer, int t_size);
        char* readReply(short int amount = 1024);

        static ProxyHost * getProxyHost(void);
};

#endif
