#ifndef LC_EXCEPTION_H
#define LC_EXCEPTION_H
#include <exception>
#include <string>

class exception_with_code : std::exception {
    private:
        int code;
        const std::string errorMessage;
    public:
        exception_with_code(int code, std::string errorMessage)
        :   code{code},
            errorMessage{errorMessage}
        {}
        exception_with_code(int code, const char *errorMessage)
        :   code{code},
            errorMessage{errorMessage}
        {}
        inline int get_error_code() const noexcept
        {
            return code;
        }
        inline const char *what() const noexcept override
        {
            return errorMessage.c_str();
        }
};

#endif