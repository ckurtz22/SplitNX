#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <switch.h>


bool tryConnect(const char* address, int port);
ssize_t send_msg(const char* msg);
ssize_t recv_msg(char* msg, size_t msg_size);