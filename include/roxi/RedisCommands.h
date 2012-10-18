#pragma once

#include <map>

namespace roxi {

#define REDIS_CMD_MAP(x,y)  make_pair<std::strin, int>(x,y)
class RedisCommands 
{
private:
    RedisCommands(){
        load_command_map();
    }
    ~RedisCommands(){}

    void load_command_map() {
        struct redis_cmd {
            const char *cmd;
            int key_pos;
        };

        static struct redis_cmd cmd_set = {
           { "APPEND"   ,       1 },
           { "HEXISTS"  ,       1 },
           { "HGET"     ,       1 },
           { "HGETALL"  ,       1 },
           { "HINCRBY"  ,       1 },
           { "HINCRBYFLOAT",    1 },
           { "HKEYS",           1 },
           { "HLEN",            1 },
           { "HMGET",           1 },
           { "HMSET",           1 },
           { "HSET",            1 },
           { "HSETNX",          1 },
           { "HVALS",           1 },
           { "INCR",            1 },
           { "INCRBY",          1 },
           { "INCRBYFLOAT",     1 },
           { "LINDEX",          1 },
           { "LLEN",            1 },
           { "LSET",            1 },
           { "LTRIM",           1 },
           { "SET",             1 },
           { "GET",             1 },
           { "ZADD",            1 },
           { "ZINCRBY",         1 },
           { "ZRANGE",          1 },
           { "ZRANGEBYSCORE",   1 },
           { "ZRANK",           1 },
           { NULL, 0 }
        };

        struct redis_cmd* cmd_ptr = cmd_set;
        while( cmd_ptr->cmd != NULL ) {
            command_map_.insert( REDIS_CMD_MAP(cmd_ptr->cmd, cmd_ptr->key_pos) );
            cmd_ptr++;
        }
    }
public:
    static RedisCommands* instance() {
        static RedisCommands sInstance;
        return &sInstance; 
    }

    int getKeyPosition(const char *cmd) {
        int ret = 0;
        std::map<std::string,int>::iterator it;
        it = command_map_.find( cmd );
        if( it != command_map_.end() ){
            ret = (it->second)->key_pos;
        }

        return ret;
    }

private:
    std::map<std::string, int> command_map_;    
};

}
