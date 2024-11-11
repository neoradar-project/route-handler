#include <stdexcept>

namespace RouteParser
{
    class InvalidAirwayDirectionException : public std::runtime_error
    {
    public:
        InvalidAirwayDirectionException(const std::string &message)
            : std::runtime_error(message) {}
    };

    class AirwayRoutingException : public std::runtime_error
    {
    public:
        AirwayRoutingException(const std::string &message)
            : std::runtime_error(message) {}
    };

    class AirwayNotFoundException : public std::runtime_error
    {
    public:
        AirwayNotFoundException(const std::string &message)
            : std::runtime_error(message) {}
    };

    class FixNotFoundException : public std::runtime_error
    {
    public:
        FixNotFoundException(const std::string &message)
            : std::runtime_error(message) {}
    };
}