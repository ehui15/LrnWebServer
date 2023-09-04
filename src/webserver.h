class WebServer
{
private:
    int setNonblocking(int fd);
    void addFd(int epollFd, int fd, bool enableET);
    void doWithRead(int sockFd);

    int m_listenFd;
    int m_port;
    int m_epollFd;
    int m_epollTriggerMode; // true代表ET，false代表LT
public:
    WebServer(int port);
    ~WebServer();
    void init();
    void eventLoop();
};