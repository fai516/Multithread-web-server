#!/bin/sh
g++ connection.cpp connection_manager.cpp mime_types.cpp reply.cpp request_handler.cpp request_parser.cpp server.cpp main.cpp -pthread -std=c++11 -lboost_system -o server
