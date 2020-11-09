#include "Exception.h"

#include <stdarg.h>
#include <stdio.h>

//----------
Exception::Exception(const char * format, ...)
{
	char buffer[200];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 200, format, args);
	va_end(args);

	this->message.assign(buffer);
}

//----------
Exception::Exception(const std::string & message)
: message(message)
{
}

//----------
const char *
Exception::what() const noexcept
{
	return this->message.c_str();
}