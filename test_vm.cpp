#include <ace/INET_Addr.h>
#include <ace/SOCK.h>
#include <map>
#include <memory>
#include <iostream>

enum Peer_State
{
    AVAILABLE = 0,
    CONNECTED,

    NUM_STATES
};

bool is_valid(Peer_State state)
{
    return ((AVAILABLE <= state) && (state < NUM_STATES));
}

struct Action_Context
{

};

struct Peer_Info
{
    ACE_INET_Addr addr;
    ACE_SOCK *connection;
    Peer_State state;

    Peer_Info()
        : connection(NULL), state(AVAILABLE) {}

    Peer_Info(ACE_INET_Addr _addr)
        : addr(_addr), connection(NULL), state(AVAILABLE) {}
};

struct Peer_Addr
{
    u_int32_t ip;
    short port;

    Peer_Addr(u_int32_t _ip, short _port)
        : ip(_ip), port(_port) {}
};

inline
bool operator<(const Peer_Addr &addr1, const Peer_Addr &addr2)
{
    return
        (addr1.ip != addr2.ip) ?
        (addr1.ip  < addr2.ip) :
        (addr1.port < addr2.port);
}


typedef std::map<Peer_Addr, Peer_Info> Peer_Store_t;


Peer_Store_t peer_store;



class Peer_SM
{
public:
    enum Peer_Actions
    {
        OPEN_CONNECTION = 0,
        CLOSE_CONNECTION,
        HANDLE_REQUEST,
        HANDLE_RESPONSE,

        NUM_ACTIONS
    };

    static bool is_valid_action(Peer_Actions action)
    {
        return ((OPEN_CONNECTION <= action) && (action < NUM_ACTIONS));
    }

    static void do_action(const Peer_Addr &address, Peer_Actions action, const Action_Context &context)
    {
        if (is_valid_action(action))
        {
            Peer_Store_t::iterator peer_iter = peer_store.find(address);
            if (peer_iter != peer_store.end())
            {
                Peer_Info &peer_info = peer_iter->second;
                Peer_State current_state = peer_info.state;
                if (is_valid(current_state))
                {
                    SM_Action_t act = actions_map[current_state][action];
                    Peer_State new_state = states_map[current_state][action];

                    if (act(peer_info, context))
                    {
                        peer_info.state = new_state;
                    }
                }
            }
        }
    }

private:
    typedef bool (*SM_Action_t)(Peer_Info &, const Action_Context &);

    static Peer_State states_map[NUM_STATES][NUM_ACTIONS];
    static SM_Action_t actions_map[NUM_STATES][NUM_ACTIONS];

    static bool no_operation(Peer_Info &, const Action_Context &);
    static bool open_connection(Peer_Info &, const Action_Context &);
    static bool close_connection(Peer_Info &, const Action_Context &);
    static bool hanle_request(Peer_Info &, const Action_Context &);
    static bool handle_response(Peer_Info &, const Action_Context &);


};


Peer_State Peer_SM::states_map[NUM_STATES][Peer_SM::NUM_ACTIONS] = {
// OPEN_CONNECTION  CLOSE_CONNECTION    HANDLE_REQUEST  HANDLE_RESPONSE
{CONNECTED,         AVAILABLE,          AVAILABLE,      AVAILABLE}, // AVAILABLE
{CONNECTED,         AVAILABLE,          CONNECTED,      CONNECTED}, // CONNECTED
};

Peer_SM::SM_Action_t Peer_SM::actions_map[NUM_STATES][Peer_SM::NUM_ACTIONS] = {
// OPEN_CONNECTION          CLOSE_CONNECTION            HANDLE_REQUEST              HANDLE_RESPONSE
{Peer_SM::open_connection,  Peer_SM::no_operation,      Peer_SM::no_operation,      Peer_SM::no_operation}, // AVAILABLE
{Peer_SM::no_operation,     Peer_SM::close_connection,  Peer_SM::hanle_request,     Peer_SM::handle_response}, // CONNECTED
};


bool Peer_SM::no_operation(Peer_Info &info, const Action_Context &context)
{
    std::cout << "no_op with " << info.addr.get_host_addr() << std::endl;
    return true;
}

bool Peer_SM::open_connection(Peer_Info &info, const Action_Context &context)
{
    std::cout << "open connection with " << info.addr.get_host_addr() << std::endl;
    return true;
}

bool Peer_SM::close_connection(Peer_Info &info, const Action_Context &context)
{
    std::cout << "close connection with " << info.addr.get_host_addr() << std::endl;
    info.connection->close();
    return true;
}

bool Peer_SM::hanle_request(Peer_Info &info, const Action_Context &context)
{
    std::cout << "handle request from " << info.addr.get_host_addr() << std::endl;
    return true;
}

bool Peer_SM::handle_response(Peer_Info &info, const Action_Context &context)
{
    std::cout << "send response to " << info.addr.get_host_addr() << std::endl;
    return true;
}

int main()
{
    ACE_INET_Addr addr("127.0.0.1:456");
    ACE_INET_Addr addr1("10.245.65.13:65");

    peer_store[Peer_Addr(1,1)] = Peer_Info(addr);
    peer_store[Peer_Addr(1,2)] = Peer_Info(addr1);

    Peer_Info &info = peer_store[Peer_Addr(1,2)];


    Peer_SM::Peer_Actions actions[] =
    {
        Peer_SM::CLOSE_CONNECTION,
        Peer_SM::HANDLE_REQUEST,
        Peer_SM::OPEN_CONNECTION,
        Peer_SM::HANDLE_REQUEST,
        Peer_SM::HANDLE_RESPONSE,
        Peer_SM::OPEN_CONNECTION,
        Peer_SM::CLOSE_CONNECTION,
    };

    Action_Context context;

    for (int i = 0; i < 7; ++i)
        Peer_SM::do_action(Peer_Addr(1,2), actions[i], context);

    return 0;
}
