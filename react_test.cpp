#include <ace/INET_Addr.h>
#include <ace/Connector.h>
#include <ace/Acceptor.h>
#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Svc_Handler.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/OS.h>
#include <ace/SOCK_Dgram_Bcast.h>

#include <ace/Thread_Manager.h>

#include <iostream>


class Handler
        : public ACE_Svc_Handler <ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
    typedef ACE_Svc_Handler <ACE_SOCK_STREAM, ACE_NULL_SYNCH> super;

public:

    int open(void *)
    {
        ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::READ_MASK);
        ACE_INET_Addr remote_addr;
        this->peer().get_remote_addr(remote_addr);
        std::cout << "register client " << remote_addr.get_host_addr() << ":" <<
                     remote_addr.get_port_number() << std::endl;
        return 0;
    }

    virtual int handle_input(ACE_HANDLE fd)
    {
        return 0;
    }
};


typedef ACE_Acceptor<Handler, ACE_SOCK_ACCEPTOR> Acceptor;

#define MAX_LISTEN_PORT_RANGE 20
#define DEFAULT_LISTEN_PORT   1917


void acceptor_thread(void *arg)
{
    ACE_INET_Addr listen_addr(DEFAULT_LISTEN_PORT);


    Acceptor acceptor;

    for (int i = 0; i < MAX_LISTEN_PORT_RANGE; ++i)
    {
        if (0 == acceptor.open(listen_addr, ACE_Reactor::instance()))
        {
            break;
        }
        listen_addr.set_port_number( listen_addr.get_port_number() + 1 );
    }

    if (DEFAULT_LISTEN_PORT + MAX_LISTEN_PORT_RANGE == listen_addr.get_port_number())
    {
        std::cout << "cant open connection" << std::endl;
        return;
    }

    std::cout << "my address is: " << listen_addr.get_host_addr()
              << ":" << listen_addr.get_port_number() << std::endl;

    while(1)
    {
        ACE_Reactor::instance()->handle_events();
    }

}

void presense_thread(void *arg)
{
    ACE_INET_Addr remote;
    ACE_INET_Addr local(1907);
    ACE_SOCK_Dgram_Bcast udp;

    if (-1 == udp.open(local, ACE_PROTOCOL_FAMILY_INET, 0, 1))
    {
        std::cout << "presense thread failed" << std::endl;
        return;
    }


    while (1)
    {
        char buff[20];
        ssize_t recv = udp.recv(buff, sizeof(buff), remote);
        if (recv > 0)
        {
            std::cout << "received message from " << remote.get_addr() << ":" << std::endl;
            std::cout << buff << std::endl;
        }

    }
}

int main()
{

    ACE_Thread_Manager manager;

    manager.spawn((ACE_THR_FUNC)presense_thread);
    manager.spawn((ACE_THR_FUNC)acceptor_thread);

    manager.wait();


    return 0;

}

