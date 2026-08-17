#include "../incl/Client.hpp"

Client::Client() : fd(-1), IPad("localhost") {}

Client::~Client(){}

std::string Client::getNick() const
{
	(void)fd;
	(void)IPad;
	return "Lena davai rabotaem";
}