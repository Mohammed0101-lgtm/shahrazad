#pragma once

#include <stdexcept>
#include <string>


namespace Shahrazad {
namespace error {


// Custom exception class for position-related errors (e.g., invalid data for moves)
class Square_error: public std::exception {
   private:
    std::string imp_;  // Stores the detailed error message
    std::string msg_;  // Stores the generic error message + detailed message

   public:
    // Constructors for initializing the error message
    explicit Square_error(const std::string& message) :
        imp_(message),
        msg_("Square error occured : " + message) {}

    explicit Square_error(const char* message) :
        imp_(message),
        msg_("Square error occured : " + std::string(message)) {}

    // Copy constructor for safely duplicating error objects
    Square_error(const Square_error& other) noexcept :
        imp_(other.imp()),
        msg_(other.message()) {}

    // Copy assignment operator with self-assignment check
    Square_error& operator=(const Square_error& other) noexcept {
        if (this != &other)
        {
            imp_ = other.imp();
            msg_ = other.message();
        }

        return *this;
    }

    // Override base class destructor
    ~Square_error() override = default;

    const char* what() const noexcept override { return msg_.c_str(); }

    // Provides access to the stored detailed message
    const std::string& imp() const { return imp_; }
    const std::string& message() const { return msg_; }
};


// Custom exception class for position-related errors (e.g., invalid data for moves)
class Move_error: public std::exception {
   private:
    std::string imp_;  // Stores the detailed error message
    std::string msg_;  // Stores the generic error message + detailed message

   public:
    // Constructors for initializing the error message
    explicit Move_error(const std::string& message) :
        imp_(message),
        msg_("Square error occured : " + message) {}

    explicit Move_error(const char* message) :
        imp_(message),
        msg_("Square error occured : " + std::string(message)) {}

    // Copy constructor for safely duplicating error objects
    Move_error(const Move_error& other) noexcept :
        imp_(other.imp()),
        msg_(other.message()) {}

    // Copy assignment operator with self-assignment check
    Move_error& operator=(const Move_error& other) noexcept {
        if (this != &other)
        {
            imp_ = other.imp();
            msg_ = other.message();
        }

        return *this;
    }

    // Override base class destructor
    ~Move_error() override = default;

    const char* what() const noexcept override { return msg_.c_str(); }

    // Provides access to the stored detailed message
    const std::string& imp() const { return imp_; }
    const std::string& message() const { return msg_; }
};


// Custom exception class for position-related errors (e.g., invalid data for moves)
class Position_error: public std::exception {
   private:
    std::string imp_;  // Stores the detailed error message
    std::string msg_;  // Stores the generic error message + detailed message

   public:
    // Constructors for initializing the error message
    explicit Position_error(const std::string& message) :
        imp_(message),
        msg_("Square error occured : " + message) {}

    explicit Position_error(const char* message) :
        imp_(message),
        msg_("Position error occured : " + std::string(message)) {}

    // Copy constructor for safely duplicating error objects
    Position_error(const Position_error& other) noexcept :
        imp_(other.imp()),
        msg_(other.message()) {}

    // Copy assignment operator with self-assignment check
    Position_error& operator=(const Position_error& other) noexcept {
        if (this != &other)
        {
            imp_ = other.imp();
            msg_ = other.message();
        }

        return *this;
    }

    // Override base class destructor
    ~Position_error() override = default;

    const char* what() const noexcept override { return msg_.c_str(); }

    // Provides access to the stored detailed message
    const std::string& imp() const { return imp_; }
    const std::string& message() const { return msg_; }
};


class Position_error: public std::exception {
   public:
    explicit Position_error(const char* message) :
        msg_(message) {}

    virtual const char* what() const noexcept override { return msg_; }

   private:
    const char* msg_;
};


}  // namespace error
}  // namespace Shahrazad