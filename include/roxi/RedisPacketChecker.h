#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <vector>

namespace roxi {

class RedisPacketChecker
{
public:
    enum {
        INIT = 0,
        COMPLETE = 1,
        CMD_COMMAND_HEAD = 2,
        CMD_COMMAND_BODY = 3,
        CMD_EACH_HEAD = 4,
        CMD_EACH_BODY = 5,
        CMD_ONCE = 6,
        CONTINUE = 7,
        CMD_COMMAND_NUMBER = 8,
        CMD_EACH_NUMBER = 9,
        ERR_UNKWON_COMMAND=-1,
        ERR_BAD_PARAMETER=-2
    };

    enum { crlf_size = 2 };

    void process_init( char ch, PacketState& _state, int &_idx ) {
        if( ch == '*' ) {
            _idx++;
            _state.state_ = CMD_COMMAND_NUMBER;
            _state.current_ = _idx;
            //get cmd_count;
        } else {
            _state.state_ = CMD_ONCE;
        }
    }

    bool get_number( Packet* _packet, int &_start, int& _value ) {
        char buf[1024];
        int i = _start;

        bool ret = get_string( _packet, _start, buf );
        if( true == ret ) {
            _value = atoi(buf);
        }

        return ret;
    }

    bool is_buffer_size_enough( Packet* _packet, int _start, int _size ) {
        bool ret = true; 
        if( _packet->length() < _start + _size + crlf_size ) {
            ret = false;
        }

        return ret;
    }

    bool get_string( Packet* _packet, int &_start, int _size, char *_buffer ) {
        int i = _start;
        bool ret = false;
        int size = _size;

        while( i < _packet->length() ) {
            char ch = _packet->ch(i);
            if( ch == '\r' ) {
                _buffer[i - _start] = 0;
                _start = i + crlf_size;
                ret = true;
                break;
            }
            _buffer[i - _start] = ch;
            i++;
            size--;
        }

        std::cout << "size1 ==> " << _size << " size2 ===> " << size  << " ret: " << ret << std::endl;
        return ret && (size == 0);
    }

    bool valid_string( Packet* _packet, int &_start, int _size ) {
        int i = _start;
        bool ret = false;
        int size = _size;
        while( i < _packet->length() ) {
            char ch = _packet->ch(i);
            if( ch == '\r' ) {
                _start = i + crlf_size;
                ret = true;
                break;
            }
            i++;
            size--;
        }

        return ret && (size == 0);
    }

    bool get_string( Packet* _packet, int &_start, char *_buffer ) {
        int i = _start;
        bool ret = false;
        while( i < _packet->length() ) {
            char ch = _packet->ch(i);
            if( ch == '\r' ) {
                if( i + crlf_size > _packet->length() ){
                    return false;
                }

                _buffer[i - _start] = 0;
                _start = i+crlf_size;
                ret = true;
                break;
            }
            _buffer[i - _start] = ch;
            i++;
        }

        return ret;
    }

    bool check_signature( Packet *_packet, char _ch, int &_start, PacketState& _state, int _next_state ) {
        if( _packet->ch(_start) != _ch ) {
            return false;
        } 

        _start++;
        _state.state_ = _next_state;
        _state.current_ = _start;
        return true;
    }

    int isComplete(Packet *_packet, PacketState &_state, char *_cmd) {
        int length = _packet->length();
        int start = _state.current_;
        int ret = CONTINUE;

        for( int i = start; i < length; ) {
            switch(_state.state_) {
                case INIT:
                    {
                        process_init( _packet->ch(i), _state, i );
                        break;
                    }
                case CMD_ONCE:
                    {
                        std::cout << "CMD_ONCE\n";
                        if( false == get_string( _packet, i, _cmd ) ) {
                            return CONTINUE;
                        }

                        std::cout << "i: " << i << std::endl;
                        _state.current_ = i;
                        return COMPLETE;
                    }
                case CMD_COMMAND_NUMBER:
                    {
                        std::cout << "CMD_COMMAND_NUMBER\n";
                        int cmd_count = 0;
                        if( false == get_number( _packet, i, cmd_count ) ) {
                            std::cout << "CONTINUE\n";
                            return CONTINUE;
                        }
                        std::cout << "CMD_COMMAND_NUMBER: " << cmd_count << " " << "idx: " << i << std::endl;

                        _state.state_ = CMD_EACH_HEAD;
                        _state.current_ = i;
                        _state.max_cmd_count_ = cmd_count;
                        _state.current_cmd_idx_ = 0;
                        break;
                    }
                case CMD_EACH_HEAD:
                    {
                        char ch = _packet->ch(i);
                        std::cout << "CMD_EACH_HEAD: " << i << " ch: " << ch << std::endl ;
                        if( false == check_signature( _packet, '$', i, _state, CMD_EACH_NUMBER ) )
                            return ERR_BAD_PARAMETER;
                    } 

                    break;
                case CMD_EACH_NUMBER:
                    {
                        std::cout << "CMD_EACH_NUMBER\n";
                        int size = 0;
                        if( false == get_number( _packet, i, size) ) {
                            return CONTINUE;
                        }

                        std::cout << "CMD_EACH_NUMBER: " << size << std::endl;
                        _state.state_ = CMD_EACH_BODY;
                        _state.current_ = i;
                        _state.current_cmd_size_ = size;
                        break;
                    }
                case CMD_EACH_BODY:
                    {
                        std::cout << "CMD_EACH_BODY: current_size ===> " << _state.current_cmd_size_ << std::endl;
                        if( false == is_buffer_size_enough( _packet, i, _state.current_cmd_size_ ) ) {
                            return CONTINUE;
                        }

                        if( _state.current_cmd_idx_ == 0 ){
                            char buffer[1024];
                            if( false == get_string( _packet, i, _state.current_cmd_size_, buffer ) ){
                                return ERR_BAD_PARAMETER;
                            }

                            std::cout << "CMD_EACH_BODY: command -> " << buffer << std::endl;
                            _state.key_cmd_idx_ = 1;
                            _state.command_ = buffer;
                        }
                        else if( _state.current_cmd_idx_ == _state.key_cmd_idx_ ){
                            char buffer[1024];
                            if( false == get_string( _packet, i, _state.current_cmd_size_, buffer ) ){
                                return ERR_BAD_PARAMETER;
                            }
                            _state.key_ = buffer;
                            std::cout << "CMD_EACH_BODY: kye -> " << buffer << std::endl;
                        }
                        else {
                            if( false == valid_string( _packet, i, _state.current_cmd_size_ ) ) {
                                return ERR_BAD_PARAMETER;
                            }
                        }

                        _state.current_ = i;
                        _state.current_cmd_idx_++;

                        if( _state.current_cmd_idx_ == _state.max_cmd_count_ ) {
                            return COMPLETE;
                        }
                        _state.state_ = CMD_EACH_HEAD;
                    }
            }
        }
        
        return ret;
    }
};

class RedisReplyPacketChecker : public RedisPacketChecker 
{
public:
    void process_init( char ch, PacketState& _state, int &_idx ) {
        if( ch == '*' ) {
            _idx++;
            _state.state_ = CMD_COMMAND_NUMBER;
            _state.current_ = _idx;
            //get cmd_count;
        } else if ( ch == '$' ) {
            _idx++;
            _state.state_ = CMD_EACH_NUMBER;
            _state.current_ = _idx;
            _state.max_cmd_count_ = 1;
        } else {
            _state.state_ = CMD_ONCE;
        }
    }

    int isComplete(Packet *_packet, PacketState &_state, char *_cmd) {
        int length = _packet->length();
        int start = _state.current_;
        int ret = CONTINUE;

        for( int i = start; i < length; ) {
            std::cout << "length: " << i << " state: " << _state.state_ << std::endl;
            switch(_state.state_) {
                case INIT:
                    {
                        process_init( _packet->ch(i), _state, i );
                        break;
                    }
                case CMD_ONCE:
                    {
                        std::cout << "CMD_ONCE\n";
                        if( false == get_string( _packet, i, _cmd ) ) {
                            return CONTINUE;
                        }

                        std::cout << "i: " << i << std::endl;
                        _state.current_ = i;
                        return COMPLETE;
                    }
                case CMD_COMMAND_NUMBER:
                    {
                        std::cout << "CMD_COMMAND_NUMBER\n";
                        int cmd_count = 0;
                        if( false == get_number( _packet, i, cmd_count ) ) {
                            std::cout << "CONTINUE\n";
                            return CONTINUE;
                        }
                        std::cout << "CMD_COMMAND_NUMBER: " << cmd_count << " " << "idx: " << i << std::endl;

                        _state.state_ = CMD_EACH_HEAD;
                        _state.current_ = i;
                        _state.max_cmd_count_ = cmd_count;
                        _state.current_cmd_idx_ = 0;
                        break;
                    }
                case CMD_EACH_HEAD:
                    {
                        char ch = _packet->ch(i);
                        std::cout << "CMD_EACH_HEAD: " << i << " ch: " << ch << std::endl ;
                        if( false == check_signature( _packet, '$', i, _state, CMD_EACH_NUMBER ) )
                            return ERR_BAD_PARAMETER;
                    } 

                    break;
                case CMD_EACH_NUMBER:
                    {
                        std::cout << "CMD_EACH_NUMBER\n";
                        int size = 0;
                        if( false == get_number( _packet, i, size) ) {
                            return CONTINUE;
                        }

                        std::cout << "CMD_EACH_NUMBER: " << size << std::endl;
                        _state.state_ = CMD_EACH_BODY;
                        _state.current_ = i;
                        _state.current_cmd_size_ = size;
                        break;
                    }
                case CMD_EACH_BODY:
                    {
                        std::cout << "CMD_EACH_BODY: current_size ===> " << _state.current_cmd_size_ << std::endl;
                        if( false == is_buffer_size_enough( _packet, i, _state.current_cmd_size_ ) ) {
                            return CONTINUE;
                        }

                        if( _state.current_cmd_idx_ == 0 ){
                            char buffer[1024];
                            if( false == get_string( _packet, i, _state.current_cmd_size_, buffer ) ){
                                return ERR_BAD_PARAMETER;
                            }
                        }
                        else if( _state.current_cmd_idx_ == _state.key_cmd_idx_ ){
                            char buffer[1024];
                            if( false == get_string( _packet, i, _state.current_cmd_size_, buffer ) ){
                                return ERR_BAD_PARAMETER;
                            }
                        }
                        else {
                            if( false == valid_string( _packet, i, _state.current_cmd_size_ ) ) {
                                return ERR_BAD_PARAMETER;
                            }
                        }

                        _state.current_ = i;
                        _state.current_cmd_idx_++;

                        if( _state.current_cmd_idx_ == _state.max_cmd_count_ ) {
                            return COMPLETE;
                        }
                        _state.state_ = CMD_EACH_HEAD;
                    }
            }
        }
        
        return ret;
    }
};

}
