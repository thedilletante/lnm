#include <ace/INET_Addr.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Dgram.h>
#include <ace/SOCK_Dgram_Bcast.h>
#include "ace/Message_Block.h"
#include "ace/CDR_Stream.h"
#include <ace/Log_Msg.h>

#include <errno.h>


class Presense_Server
        : public ACE_Event_Handler
{
    typedef ACE_Event_Handler super;
public:

    Presense_Server(const ACE_INET_Addr &local_addr);

    virtual ACE_HANDLE get_handle() const;

    virtual int handle_input(ACE_HANDLE fd);
    virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

private:

    ACE_SOCK_Dgram endpoint_;

protected:
    ~Presense_Server(); // this object must be created on the heap
};


class Presense_Talker
        : public ACE_Event_Handler
{
    typedef ACE_Event_Handler super;
public:

    Presense_Talker(const ACE_INET_Addr &local_addr);

    virtual ACE_HANDLE get_handle() const;

    virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);
    virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

private:


    void start_timer(const ACE_Time_Value &val)
    {
        current_timer = ACE_Reactor::instance()->schedule_timer(this, NULL, val);
    }

    ACE_SOCK_Dgram_Bcast endpoint_;
    ACE_INET_Addr broad_addr;

    long current_timer;
    int sec_count;

protected:
    ~Presense_Talker(); // this object must be created on the heap
};



inline Presense_Server::Presense_Server(const ACE_INET_Addr &local_addr)
    : endpoint_(local_addr, ACE_PROTOCOL_FAMILY_INET, 0, 1)
{
    ACE_DEBUG((LM_DEBUG, "(%P|%t) server created\n"));
}

inline Presense_Talker::Presense_Talker(const ACE_INET_Addr &local_addr)
    : endpoint_(local_addr, ACE_PROTOCOL_FAMILY_INET, 0, 1),
      sec_count(1),
      current_timer(0),
      broad_addr(1907)
{
    ACE_Time_Value timeout(0, 500000); // half a second
    start_timer(timeout);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) talker created\n")));
}

inline Presense_Server::~Presense_Server()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("server removed\n")));
}

inline Presense_Talker::~Presense_Talker()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) talker removed\n")));
}


inline ACE_HANDLE Presense_Server::get_handle() const
{
    return this->endpoint_.get_handle();
}

inline ACE_HANDLE Presense_Talker::get_handle() const
{
    return this->endpoint_.get_handle();
}


inline int Presense_Server::handle_input(ACE_HANDLE fd)
{
    char buf[50];
    ACE_INET_Addr from_addr;

    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT("(%P|%t) activity occurred on handle %d!\n"),
                this->endpoint_.get_handle ()));

    ssize_t n = this->endpoint_.recv (buf,
                                      sizeof buf,
                                      from_addr);

    if (n == -1)
      ACE_ERROR ((LM_ERROR,
                  "%p\n",
                  ACE_TEXT("handle_input")));
    else
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT("(%P|%t) buf of size %d = %*s\n"),
                  n,
                  n,
                  buf));
    return 0;
}

inline int Presense_Talker::handle_timeout(const ACE_Time_Value &current_time, const void *act)
{
    current_timer = 0;
    ACE_UNUSED_ARG(current_time);
    ACE_UNUSED_ARG(act);

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) timed out for talker endpoint\n")));

    static const char buff[] = "hello";

    ssize_t sendn = endpoint_.send(buff, sizeof(buff), broad_addr);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) send %d bytes to %s:%d\n"),
               sendn,
               broad_addr.get_host_addr(),
               broad_addr.get_port_number()));

    ACE_Time_Value timeout(sec_count);
    start_timer(timeout);
    sec_count++;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) at the next time timer expired after %d seconds\n"),
               sec_count));
    return 0;
}

inline int Presense_Server::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
{
    ACE_UNUSED_ARG(handle);
    ACE_UNUSED_ARG(close_mask);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) close server\n")));
    this->endpoint_.close();
    delete this;
    return 0;
}

inline int Presense_Talker::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
{
    ACE_UNUSED_ARG(handle);
    ACE_UNUSED_ARG(close_mask);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) close talker\n")));
    this->endpoint_.close();
    if (0 != current_timer)
    {
        ACE_Reactor::instance()->cancel_timer(current_timer);
    }
    delete this;
    return 0;
}



int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
    ACE_UNUSED_ARG(argc);
    ACE_UNUSED_ARG(argv);

    ACE_INET_Addr listen_addr(1907);
    ACE_INET_Addr talker_addr(1906);

    Presense_Server *pserver = NULL;
    Presense_Talker *ptalker = NULL;

    ACE_NEW_RETURN(pserver,
                   Presense_Server(listen_addr),
                   -1);
    ACE_NEW_RETURN(ptalker,
                   Presense_Talker(talker_addr),
                   -1);

    if (-1 == ACE_Reactor::instance()->register_handler(pserver, ACE_Event_Handler::READ_MASK))
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("register_handler failed\n")), -1);
    }
    else
    {
        ACE_DEBUG((LM_INFO, ACE_TEXT("pserver registered\n")));
    }

    if (-1 == ACE_Reactor::instance()->register_handler(ptalker, ACE_Event_Handler::TIMER_MASK))
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("register talker failed\n")), -1);
    }
    else
    {
        ACE_DEBUG((LM_INFO, ACE_TEXT("ptalker registered\n")));
    }

    for (int i=0; i < 10; i++) // will work 1 minute for test
    {
        // will wait 1 second
        ACE_Time_Value tv(1);
        int res = ACE_Reactor::instance()->handle_events(tv);
        if (0 > res)
        {
            ACE_DEBUG((LM_ERROR, ACE_TEXT("handle_events failed\n")));
            break;
        }
        if (0 == res && errno == EWOULDBLOCK)
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ewouldblock")));
        }
        ACE_DEBUG ((LM_DEBUG,
                     "(%P|%t) return from handle events\n"));
    }

    if (-1 == ACE_Reactor::instance()->remove_handler(pserver, ACE_Event_Handler::READ_MASK))
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("remove handler failed\n")), -1);
    }
    if (-1 == ACE_Reactor::instance()->remove_handler(ptalker, ACE_Event_Handler::TIMER_MASK))
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("remove talker failed\n")), -1);
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("exiting\n")));


    return 0;




}
