#include "ace/OS.h"
#include "ace/Log_Msg.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Dgram_Bcast.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Message_Block.h"
#include "ace/CDR_Stream.h"


class Multi_Sender
{
    static const short DEFAULT_REMOTE_PORT = 1907;
    static const short DEFAULT_LOCAL_PORT = 1906;


    static const size_t max_payload_size =
            sizeof(u_int32_t) +
            sizeof(short) +
            ACE_CDR::MAX_ALIGNMENT;
    static const size_t IOV_COUNT = 2;

public:
    Multi_Sender()
        : remote_port(DEFAULT_REMOTE_PORT),
          udp(ACE_INET_Addr(DEFAULT_LOCAL_PORT)),
          header(ACE_CDR::MAX_ALIGNMENT + sizeof(ACE_CDR::Boolean)),
          payload(max_payload_size)
    {
        header << ACE_CDR::Byte_Order();
        iov[0].iov_base = header.begin()->rd_ptr();
        iov[0].iov_len = sizeof(u_int32_t);
        iov[1].iov_base = 0;
    }

    ~Multi_Sender()
    {
        udp.close();
    }

    Multi_Sender &set_local_addr(u_int32_t ip, short port)
    {
        payload.reset();

        payload << ACE_CDR::ULong(ip);
        payload << ACE_CDR::Short(port);

        iov[1].iov_base = payload.begin()->rd_ptr();
        iov[1].iov_len = payload.total_length();
        return *this;
    }

    Multi_Sender &set_multi_port(short port)
    {
        if (port > 0)
        {
            remote_port = port;
        }
        return *this;
    }

    bool send_to_all()
    {
        bool retval = false;

        if (iov[1].iov_base != 0)
        {
            ssize_t sent = udp.send(iov, (int)IOV_COUNT, remote_port);
            if (sent > 0)
            {
                retval = true;
            }
        }

        return retval;
    }

private:
    short remote_port;
    iovec iov[IOV_COUNT];
    ACE_SOCK_Dgram_Bcast udp;
    ACE_OutputCDR payload;
    ACE_OutputCDR header;
};



int main(int argc, char *argv[])
{
    Multi_Sender sender;
    sender.set_local_addr(2130706433, 21);
    for (int i = 0; i < 10; i++)
    {
        sender.send_to_all();
        sleep(1);
        if (i == 5)
            sender.set_local_addr(167772677, 2222);
    }
    return 0;
}
