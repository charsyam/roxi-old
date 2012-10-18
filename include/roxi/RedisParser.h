#pragma once

namespace roxi {

#define REDISPARSER_START   0
#define REDISPARSER_COUNT   1
#define REDISPARSER_PARAM   2
#define REDISPARSER_COMPLETE    3

class RedisPacket
{
public:
    RedisPacket() : ptr_(NULL), size_(0), condition_(0), prev_check_(0), cmd_count_(0),
                    cur_cmd_count_(0), cur_size_(0){}
    const char *ptr_;
    int size_;
    int condition_;
    int prev_check_;
    int cmd_count_;
    int cur_cmd_count_;
    int cur_size_;
};

class RedisParser
{
public:
    static int isValidSize( const int _size ) {
    }

    static int isValidCommand( RedisPacket* _packet, char **_key ) {
        char *ptr = _packet->ptr_;
        if( REDISPARSER_START != condition_ ) {
            ptr = _packet->ptr_;
        }
        

        for( int i = 0; i < size_  ; i++ ) {
            if( REDISPARSER_START == condition_ ) {
                if( *ptr == '\n' )
            }
        }
    }
};

}
