#include <ace/Log_Msg.h>
#include <ace/INET_Addr.h>
#include <ace/SOCK_Dgram_Bcast.h>
#include <ace/CDR_Stream.h>

#include <iostream>

class Multi_Receiver
{
public:
    Multi_Receiver(short port);


};



int main()
{
    ACE_INET_Addr remote;
    ACE_INET_Addr local(1907);
    ACE_SOCK_Dgram_Bcast udp;

    if (-1 == udp.open(local, ACE_PROTOCOL_FAMILY_INET, 0, 1))
    {
        ACE_ERROR_RETURN((LM_ERROR, "%p\n", "open"), -1);
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
    return 0;

}
