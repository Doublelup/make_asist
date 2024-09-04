#include "../include/parser.h"
#include <cctype>

using namespace reader;
Reader :: Reader(std :: string &file_path)
:   file_path{file_path},
    content{},
    p{nullptr},
    current{nullptr},
    s{}
{
    std :: ifstream file(file_path);
    if (!file.is_open())
        throw std :: runtime_error("Reader 1 [file path error]");
    content = {std :: istreambuf_iterator<char>(file), 
                        std :: istreambuf_iterator<char>()};
    file.close();
    p = content.c_str();
}

Reader :: ~Reader()
{
    assert (current);
    delete current;
}

bool
Reader :: is_finish()
{
    if (*p) return false;
    else return true;
}

bool is_name_char(const char c)
{
    return std :: isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}

// return whether read a name, error indicates syntax error.
// p point at the char after ')' or '\0'
bool read_name(const char *&p, std :: string &name, bool &error)
{
    char status = 0;
    // '(' 0, ')' 1, 'space' 2, name_char 3, other 4
    char t[4][4] = {
        {1, 5, 0, 5},
        {5, 4, 1, 2},
        {5, 4, 3, 2},
        {5, 4, 3, 5},
    };
    char type = 0;
    const char *start = nullptr;
    const char *end = nullptr;
    char pre = 0;
    bool ret = false;
    while (status != 4 && status != 5){
        pre = status;
        switch (*p){
            case '(':
                type = 0;
                break;
            case ')':
                type = 1;
                break;
            case ' ':
            case '\t':
                type = 2;
                break;
            default:
                if (is_name_char(*p)) type = 3;
                else type = 4;
                break;
        }
        status = t[status][type];
        if (pre == 1 && status == 2) start = p;
        if (pre == 2 && (status == 3 || status == 4)) end = p;
        if (!*p)break;
        ++p;
    }
    if (status == 5) error = true;
    else if (start && end){
        name = {start, static_cast<unsigned long>(end - start)};
        ret = true;
    }
    return ret;
}

enum order_type
Reader :: read_order(name_t &name)
{
    size_t remain_size = content.size() - static_cast<size_t>(p - content.c_str());
    bool error = false;
    assert (!is_finish());
    enum order_type ret;
    if (remain_size >= 6 && strncmp(p, "defdir", 6) == 0){
        p += 6;
        if(!read_name(p, name, error)) name = "";
        if (error) ret = order_t :: OERROR;
        else ret = DEFDIR;
    }
    else if (remain_size >= 8 && strncmp(p, "noextend", 8) == 0){
        p += 8;
        if (!read_name(p, name, error)) name = "";
        if (error) ret = order_t :: OERROR;
        else ret = NOEXTEND; 
    }
    else if (remain_size >= 8 && strncmp(p, "undefdir", 8) == 0){
        p += 8;
        if (!read_name(p, name, error) || error) ret = order_t :: OERROR;
        else ret = UNDEFDIR;
    }
    else if (remain_size >= 10 && strncmp(p, "unnoextend", 10) == 0){
        p += 10;
        if (!read_name(p, name, error) || error) ret = order_t :: OERROR;
        else ret = UNNOEXTEND;
    }
    else if (remain_size >= 7 && strncmp(p, "context", 7) == 0){
        p += 7;
        ret = CONTEXT;
    }
    else if (remain_size >= 3 && strncmp(p, "raw", 3) == 0){
        p += 3;
        ret = RAW;
    }
    else if (remain_size >= 4 &&strncmp(p, "make", 4) == 0){
        p += 4;
        ret = MAKE;
    }
    else{
        ret = RAW;
    }
    return ret;
}

size_t
Reader :: content_size()
{
    return content.size();
}

namespace trans_matrix{
    // for blocks {}
    // space except '\n' 0, '\n' 1, '#' 2, '{' 3, '}' 4, '\0' 5, '\' 6,other 7
    char block_char_num(char c)
    {
        char ret;
        switch (c){
            case ' ':
            case '\t': ret = 0; break;
            case '\n': ret = 1; break;
            case '#': ret = 2; break;
            case '{': ret = 3; break;
            case '}': ret = 4; break;
            case '\0': ret = 5; break;
            case '\\': ret = 6; break;
            default: ret = 7; break;
        }
        return ret;
    }
    const char block_trans[4][8] = {
        {0, 0, 1, 2, 4, 4, 4, 4},
        {1, 0, 1, 1, 1, 4, 1, 1},
        {2, 2, 2, 2, 2, 4, 3, 2},
        {2, 2, 2, 2, 2, 4, 2, 2},
        // state 4, error;
        // state 5, finish;
    };

    // for make and raw. They need distinguish ':' and '{';
    char line_char_num(char c)
    {
        char ret;
        // '\n' 0, '#' 1, ' ' '\t' 2, '{' 3, ':' 4, '\' 5, '\0' 6, name_char 7, other 8
        switch (c){
            case '\n': ret = 0; break;
            case '#': ret = 1; break;
            case ' ':
            case '\t': ret = 2; break;
            case '{': ret = 3; break;
            case ':': ret = 4; break;
            case '\\': ret = 5; break;
            case '\0': ret = 6; break;
            default: 
                if (is_name_char(c)) ret = 7;
                else ret = 8;
                break;
        }
        return ret;
    }
    const char line_trans[4][9] = {
        {0, 1, 0, 5, 2, 6, 6, 2, 6},
        {0, 1, 1, 1, 1, 1, 6, 1, 1},
        {4, 4, 2, 2, 2, 3, 6, 2, 2},
        {2, 2, 2, 2, 2, 2, 6, 2, 2},
        // state 4, line type end;
        // state 5, block type begin;
        // state 6, error;
    };

    // for context. For example:
    // context[comment or space]{
    char context_pre_char_num(char c)
    {
        char ret;
        // '\n' 0, '#' 1, ' ' '\t' 2, '{' 3, '\0' 4, other 5
        switch (c){
            case '\n': ret = 0; break;
            case '#': ret= 1; break;
            case ' ':
            case '\t': ret = 2; break;
            case '{': ret = 3; break;
            case '\0': ret = 4; break;
            default: ret = 5;break;
        }
        return ret;
    }
    const char context_pre_trans[2][6] = {
        {0, 1, 0, 2, 3, 3},
        {0, 1, 1, 1, 3, 1},
        // state 2, end;
        // state 3, error;
    };
}

signal_t 
Reader :: block_handler()
{
    std :: string name = "";
    signal_t ret;
    order_t type = read_order(name);
    const char *start, *end;
    char status, pre;
    char char_type;
    switch (type){
        case RAW:
        case MAKE:
            {
                start = nullptr;
                end = nullptr;
                status = 0;
                // status == 4 || 5 end, 6 error
                while (status != 4 && status != 5 && status != 6){
                    pre = status;
                    char_type = trans_matrix :: line_char_num(*p);
                    status = trans_matrix :: line_trans[status][char_type];
                    if (pre == 0 && status == 2){
                        if (char_type == 4) start = p + 1;
                        else start = p;
                    }
                    if (pre == 2 && status == 4){
                        end = p;
                        if (char_type == 1) --p;
                    }
                    ++p;
                }
                if (status == 6){
                    ret = signal_t :: ERROR;
                    --p;
                    break;
                }
                else if(status == 4){
                    assert(start && end);
                    if (type == RAW) current->raw(start, end);
                    else if (type == MAKE) current->make(start, end);
                    else assert(0);
                    ret = READ;
                    break;
                }
                else{
                    assert(status == 5);
                    --p;
                }
            }
        case DEFDIR:
        case NOEXTEND:
            {
                start = nullptr;
                end = nullptr;
                status = 0;
                int depth = 0;
                // status == 4 error, status == 5 end
                while(status != 4 && status != 5)
                {
                    pre = status;
                    char_type = trans_matrix :: block_char_num(*p);
                    status = trans_matrix :: block_trans[status][char_type];
                    if (pre == 0 && status == 2){
                        ++depth;
                        start = p + 1;
                    }
                    if (pre == 2 && status == 2 && char_type == 3) ++depth;
                    if (pre == 2 && status == 2 && char_type == 4) --depth;
                    if (status == 2 && depth == 0){
                        status = 5;
                        end = p;
                    }
                    ++p;
                }
                if (start && end){
                    assert(status == 5);
                    if (type == DEFDIR) current->defdir(name, start, end);
                    else if(type == NOEXTEND)current->noextend(name, start, end);
                    else if(type == RAW) current->raw(start, end);
                    else if(type == MAKE) current->make(start, end);
                    else assert(0);
                    ret = READ;
                }
                else{
                    assert(status == 4);
                    --p;
                    ret = signal_t :: ERROR;
                }
            }
            break;
        case UNDEFDIR:
            current->undefdir(name);
            ret = READ;
            break;
        case UNNOEXTEND:
            ret = READ;
            current->unnoextend(name);
            break;
        case CONTEXT:
            status = 0;
            while (status != 2 && status != 3){
                pre = status;
                char_type = trans_matrix :: context_pre_char_num(*p);
                status = trans_matrix :: context_pre_trans[status][char_type];
                ++p;
            }
            if (status == 2)
                ret = NEWCONTEXT;
            else{
                ret = signal_t :: ERROR;
                --p;
            }
            break;
        case order_t :: OERROR:
            ret = signal_t :: ERROR;
            break;
        default:
            assert(0);
            break;
    }
    return ret;
}

void
Reader :: start()
{
    // 1 end, 2 error
    char status = 0;
    signal_t input = NEWCONTEXT;
    while (!status){// status != 1 && status != 2
        // handle signal
        if (input == END) status = 1;
        else if(input == ERROR) status = 2;
        // else status = 0;
        switch (input){
            case NEWCONTEXT:
                {
                    // execute signal
                    if (current){
                        s.push(current);
                        Context * newcontext = new Context{*current}; 
                        current->add_sub_context(newcontext);
                        current = newcontext;
                    }
                    else{
                        current = new Context{};
                    }
                    // generate new signal
                    input = READ;
                }
                break;
            case READ:
                {
                    // generate new signal
                    switch (*p){
                        case '\n':
                        case ' ':
                        case '\t':
                            input = READ;
                            break;
                        case '}':
                            input = ENDCONTEXT;
                            break;
                        case '\0':
                            input = END;
                            --p;
                            break;
                        case '#':
                            input = COMMENT;
                            break;
                        default:
                            if(is_name_char(*p)){
                                input = NAME_CHAR;
                                --p;
                            }
                            else{
                                input = signal_t :: ERROR;
                                --p;
                            }
                            break;
                    }
                    ++p;
                }
                break;
            case NAME_CHAR:
                input = block_handler();
                break;
            case ENDCONTEXT:
                if (s.empty()) input = signal_t :: ERROR;
                else{
                    current = s.top();
                    s.pop();
                    input = READ;
                }
                break;
            case END:
                break;
            case ERROR:
                // for test
                std :: cout << "error" << std :: endl;
                // end
                break;
            case COMMENT:
                while(*p && *p != '\n')++p;
                if (!*p) input = END;
                else{
                    input = READ;
                    ++p;
                }
                break;
            default:
                assert (0);
                break;
        }
    }
    // TODO, wait for finish

    if (!s.empty()) std :: cout << "context is not empty" << std :: endl;

    return;
}

const char *
Reader :: get_p()
{
    return p;
}

Context :: Context(Context &parent)
:   finish{false},
    subcontexts{},
    lpd{},
    rpd{true, parent.rpd},
    lnd{},
    rnd{true, parent.rnd}
{
}

Context :: Context()
:   finish{false},
    subcontexts{},
    lpd{},
    rpd{},
    lnd{},
    rnd{}
{
}

void
Context :: add_sub_context(Context *context)
{
    assert(context);
    subcontexts.push_back(context);
    return;
}














