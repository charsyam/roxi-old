#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <vector>

using boost::asio::ip::tcp;

#include <boost/thread.hpp>
#include <boost/date_time.hpp>

#include <roxi/RoxiConfig.h>
#include <roxi/Packet.h>
#include <roxi/PacketState.h>
#include <roxi/RedisPacketChecker.h>
#include <roxi/locking_queue.h>

namespace roxi {

template<typename PacketChecker>
class ClientSession
{
public:
    ClientSession(boost::asio::io_service& _io_service, locking_queue<RedisPacket>& _queue)
        : socket_(_io_service), packet_(NULL), packet_state_(NULL), queue_(_queue)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start() {
        reset_cmd();
    }

    void reset_cmd()
    {
        if( packet_ ) {
            delete packet_;
        }

        packet_ = new Packet;

        if( packet_state_ ) {
            delete packet_state_;
        }

        packet_state_ = new PacketState;
        read();
    }

    void write( const char* _msg, int _size)
    {
        boost::asio::async_write(socket_,
                    boost::asio::buffer(_msg, _size),
                    boost::bind(&ClientSession<RedisPacketChecker>::handle_write, this,
                    boost::asio::placeholders::error));
    }

    void read() {
        socket_.async_read_some(boost::asio::buffer(packet_->current(), packet_->reserved()),
            boost::bind(&ClientSession<RedisPacketChecker>::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code& error,
            size_t bytes_transferred)
    {
        if (!error)
        {
            PacketChecker pc;
            packet_->add( bytes_transferred ); 
            char cmd[2048];
            int ret = pc.isComplete( packet_, *packet_state_, cmd );
            if( RedisPacketChecker::COMPLETE == ret ) {
                std::cout << "Packet Parse Complete\n";
                Packet *new_packet = new Packet;
                PacketState *new_packet_state = new PacketState;
                packet_->split_packet( packet_state_->current_, new_packet );
                

                RedisPacket packet;
                packet.session_ = (void *)this;
                packet.packet_ = packet_;
                packet.cmd_ = packet_state_->command_;
                packet.key_ = packet_state_->key_;
                queue_.push(packet);

                delete packet_state_;
                packet_ = new_packet;
                packet_state_ = new_packet_state;
                read();
            }
            else if( -1 == ret ) {
                std::cout << "Packet Parse Error -1\n";
                delete this;
                //error
                //return err("-Err")
            }
            else if( -2 == ret ) {
                std::cout << "Packet Parse Error -2\n";
                delete this;
                //error
                //unknown or not to support
            }
            else if( RedisPacketChecker::CONTINUE == ret ) {
                //continue
                packet_->check_and_expand();
                read();
            }
        }
        else
        {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
        }
        else
        {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    Packet *packet_;
    PacketState *packet_state_;
    locking_queue<RedisPacket>& queue_;
};

class RedisSession {
public:
    RedisSession(boost::asio::io_service& _io_service, const char *_host, int _port ) 
        : socket_(_io_service), packet_(NULL), packet_state_(NULL)
    {
        tcp::resolver resolver(_io_service);
        char buffer[1024];
        sprintf( buffer, "%d", _port );

        tcp::resolver::query query(_host, buffer );
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        boost::asio::async_connect(socket_, endpoint_iterator,
                boost::bind(&RedisSession::handle_connect, this,
                    boost::asio::placeholders::error));
    }

    void handle_connect(const boost::system::error_code& error)
    {
        if (!error)
        {
            std::cout << "Connect to Redis\n";
        }
    }

    tcp::socket& socket()
    {
        return socket_;
    }

private:
    tcp::socket socket_;
    Packet *packet_;
    PacketState *packet_state_;
};

bool gContinue = true;

class RedisWorker 
{
private:
    locking_queue<RedisPacket> &queue_;
    std::vector<RedisSession*> &redis_sessions_;

public:
    RedisWorker(locking_queue<RedisPacket> &_queue, 
                std::vector<RedisSession*> &_redis_sessions )
                : queue_(_queue), redis_sessions_(_redis_sessions) {}
                
    RedisSession* get_session( const char *_key ) {
        return (redis_sessions_)[0];
    }

    void sendPacket( Packet* _packet, tcp::socket& _socket ) {
        int size = _packet->length();
        int length = _packet->length();
        for( int i = 0; i < _packet->count(); i++ ) {
            char *ptr = _packet->get(i);
            length = _packet->block_size();    
            if( i == _packet->count() - 1 ){
                length = _packet->length() % _packet->block_size();
                if( length == 0 ) {
                    length = _packet->block_size();
                }
            }

            int written = boost::asio::write( _socket, boost::asio::buffer(ptr, length));
            std::cout << "Write: " << written << " , " << length << std::endl;
        }
    }

    void operator()() {
        boost::posix_time::seconds workTime(0);  

        std::cout << "RedisWorker start...\n";
        while( gContinue ) {
            RedisPacket packet = queue_.pop(true);
            RedisSession* session = get_session( packet.key_.c_str() );

            std::cout << "Get New Packet From Queue...: " << packet.cmd_ << std::endl;
            sendPacket( packet.packet_, session->socket() );
            bool bContinue = true;
            delete packet.packet_;
            Packet* read_packet = new Packet;
            PacketState new_read_packet_state;
            while( bContinue ) {
                int read = boost::asio::read(session->socket(), 
                        boost::asio::buffer(read_packet->current(), 1 ));
                std::cout << read_packet->get(0) << std::endl;
                std::cout << "Read Size: " << read << std::endl;
                if( read > 0 ) {
                    read_packet->add(read);
                    RedisReplyPacketChecker pc;
                    char cmd[2048];
                    int ret = pc.isComplete( read_packet, new_read_packet_state, cmd );
                    if( RedisPacketChecker::COMPLETE == ret ) {
                        std::cout << "Packet Parse Complete\n";
                        bContinue = false;
                        ClientSession<RedisPacketChecker> *clientSession = (ClientSession<RedisPacketChecker> *)(packet.session_);
                        sendPacket( read_packet, clientSession->socket() );
                        //read_packet 
                    }
                    else if( -1 == ret ) {
                        std::cout << "Packet Parse Error -1\n";
                        bContinue = false;
                        //error
                        //return err("-Err")
                    }
                    else if( -2 == ret ) {
                        std::cout << "Packet Parse Error -2\n";
                        bContinue = false;
                        //error
                        //unknown or not to support
                    }
                    else if( RedisPacketChecker::CONTINUE == ret ) {
                        //continue
                        read_packet->check_and_expand();
                    }
                } else {
                    std::cout << "Redis Read error\n";
                }
            }

            delete read_packet;
            boost::this_thread::sleep(workTime); 
        }
    }
};

class Proxy
{
public:
    Proxy(boost::asio::io_service& _io_service, RoxiConfig& _config, locking_queue<RedisPacket>& _queue)
        : io_service_(_io_service), queue_(_queue),
            acceptor_(_io_service, tcp::endpoint(tcp::v4(), _config.port() ))
    {
        init(_io_service, _config);
        start_accept();
    }

private:
    void init( boost::asio::io_service& _io_service, RoxiConfig& _config ) {
        for( int i = 0; i < _config.getServerPairCount(); i++ ) {
            RoxiServerPair &pair = _config.server_pairs(i);
            RedisSession* new_conn = 
                new RedisSession(_io_service, pair.master(), 2000 );
            redis_sessions_.push_back(new_conn);
        }

        RedisWorker redisWorker( queue_, redis_sessions_ );
        boost::thread workerThread(redisWorker); 
    }

    void start_accept()
    {
        ClientSession<RedisPacketChecker>* new_session = new ClientSession<RedisPacketChecker>(io_service_, queue_);
        acceptor_.async_accept(new_session->socket(),
                boost::bind(&Proxy::handle_accept, this, new_session,
                    boost::asio::placeholders::error));
    }

    void handle_accept(ClientSession<RedisPacketChecker>* new_session,
            const boost::system::error_code& error)
    {
        if (!error)
        {
            new_session->start();
        }
        else
        {
            delete new_session;
        }

        start_accept();
    }

    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    locking_queue<RedisPacket>& queue_;
    std::vector<RedisSession*> redis_sessions_;
};

}
