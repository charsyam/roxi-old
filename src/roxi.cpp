#include <cstdlib>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <roxi/roxi.h>
#include <roxi/RoxiCommandInfo.h>
#include <roxi/RoxiConfig.h>
#include <roxi/Proxy.h>
#include <roxi/locking_queue.h>


const char *default_roxi_conf_path = "./roxi.conf";

void usage() {
}

void dumpConfig( roxi::RoxiConfig& config ) {
    std::cout << "Port: " << config.port() << std::endl;
    for( int i = 0 ; i < config.getServerPairCount(); i++ ) {
        roxi::RoxiServerPair &info = config.server_pairs(i);
        std::cout<< info.master() << " " << info.slave() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    roxi::RoxiCommandInfo command_info;
    roxi::RoxiConfig config;

    command_info.parse_config( argc, argv );
    int rc = config.load_conf( command_info.path() );
    dumpConfig(config);

    try {
        boost::asio::io_service io_service;
        roxi::locking_queue<roxi::RedisPacket> queue;
        roxi::Proxy s(io_service, config, queue);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    } 
    return 0;
}
