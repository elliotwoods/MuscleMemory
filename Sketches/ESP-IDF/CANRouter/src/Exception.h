
#include <exception>
#include <string>

class Exception : std::exception {
public:
	Exception(const char *, ...);
	Exception(const std::string &);
	const char * what() const noexcept override;
private:
	std::string message;
};