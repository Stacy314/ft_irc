#pragma once 

#include <string>
#include <set>

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