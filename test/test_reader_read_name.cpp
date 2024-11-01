#include <string>
#include <assert.h>
#include <iostream>

bool 
inline is_name_char(const char c)
{
    return std :: isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}

bool read_name(const char *&p, std :: string &name, bool &error) {
    /*
    STATE0: wait for signal for collecting name.
    STATE1: collect name letters.
    STATE2: wait for signal for end.
    END: end.
    */
    enum class state {
        START, STATE0, STATE1, STATE2, END, ERROR,
    };
    enum class type {
        SPACE, LPARAM, RPARAM, NAMECHAR, OTHER, END, 
    };
    auto readin = [&p]{
        type ret;
        switch(*p) {
            case 0:
                ret = type::END;
                break;
            case ' ':
            case '\n':
            case '\t':
                ret = type::SPACE;
                ++p;
                break;
            case '(':
                ret = type::LPARAM;
                ++p;
                break;
            case ')':
                ret = type::RPARAM;
                ++p;
                break;
            default:
                if (is_name_char(*p)){
                    ret = type::NAMECHAR;
                    ++p;
                }
                // Need further handling.
                else ret = type::OTHER;
                break;
        }
        return ret;
    };
    state s = state::START;
    type t;
    const char *start = nullptr, * end = nullptr;
    bool ret = false;

    error = false;
    while (1) {
        switch (s) {
            case state::START:
                t = readin();
                switch (t) {
                    case type::SPACE:
                        break;
                    case type::LPARAM:
                        s = state::STATE0;
                        break;
                    default:
                        s = state::END;
                        break;
                }
                break;
            case state::STATE0:
                t = readin();
                switch(t) {
                    case type::SPACE:
                        break;
                    case type::NAMECHAR:
                        start = p - 1;
                        s = state::STATE1;
                        break;
                    default:
                        s = state::ERROR;
                        break;
                }
                break;
            case state::STATE1:
                t = readin();
                switch(t) {
                    case type::NAMECHAR:
                        break;
                    case type::SPACE:
                        end = p;
                        s = state::STATE2;
                        break;
                    case type::RPARAM:
                        end = p - 1;
                        s = state::END;
                        break;
                    default:
                        s = state::ERROR;
                        break;
                }
                break;
            case state::STATE2:
                t = readin();
                switch(t) {
                    case type::SPACE:
                        break;
                    case type::RPARAM:
                        s = state::END;
                        break;
                    default:
                        s = state::ERROR;
                        break;
                }
                break;
            case state::ERROR:
                error = true;
                ret = false;
                goto OUT;
                break;
            case state::END:
                if (start && end){
                    ret = true;
                    name = {start, static_cast<size_t>(end - start)};
                }
                else assert(!start && !end);
                goto OUT;
                break;
            default:
                [[unlikely]];
                assert(0);
            break;
        }
    }

    OUT:

    return ret;
}

void test_reader_read_name() {
    const char *test_strs[] = {
        "   \t\n{hahs}",
        "  \t(name)  {}",
        "(name \t)",
        " (  namesss)",
    };
    bool ret, error;
    std::string name;
    // break point set here!
    for (int i = 0; i < sizeof(test_strs)/ sizeof(const char *); ++i) {
        ret = read_name(test_strs[i], name, error);
        fprintf(stdout, "Sample[%d]: %s\n", i, test_strs[i]);
        if (!ret) std::cout << "No name in sample." << std::endl;
        else std::cout << "Name: " << name << std::endl;
    }
    return;
}

int main() {
    test_reader_read_name();
}