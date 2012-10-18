roxi
====

roxi is Redis Server Proxy.( It's name comes from Moxi(Memcached Server Proxy) )

roxi.conf(json format)
===============================================================================   
{
    "proxy_port": 9999,
    "nodes": [
        { "name": "node_1", "host": "127.0.0.1", "port": 2000 },
        { "name": "node_2", "host": "127.0.0.1", "port": 2001 },
        { "name": "node_3", "host": "127.0.0.1", "port": 2002 },
        { "name": "node_4", "host": "127.0.0.1", "port": 2003 },
        { "name": "node_5", "host": "127.0.0.1", "port": 2004 },
        { "name": "node_6", "host": "127.0.0.1", "port": 2005 }
    ],
    "master_of" : [
        { "master": "node_1", "slave" : "node_2" },
        { "master": "node_3", "slave" : "node_4" },
        { "master": "node_5", "slave" : "node_6" }
    ]
}

Requirement
===========
1. boost asio
2. jsoncpp( to parse conf )