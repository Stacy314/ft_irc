#pragma once
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <csignal>
# define KNRM  "\x1B[0m"
# define KRED  "\x1B[31m"
# define KGRN  "\x1B[32m"
# define KYEL  "\x1B[33m"
# define KBLU  "\x1B[34m"
# define KMAG  "\x1B[35m"
# define KCYN  "\x1B[36m"
# define KWHT  "\x1B[37m"

class Client
{
	private:
		int fd;
		std::string IPad;
	public:
		Client();
		~Client();
		std::string getNick() const;

};