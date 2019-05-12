#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <switch.h>


bool tryConnect(int* sock, const char* address, int port);
void send_msg(int sock, const char* msg);