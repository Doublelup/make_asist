#include "../include/parser.h"
#include "../include/test.h"

using namespace reader;

const char *context_begin;

void
Context :: is_finish()
{
    return;
}

#define PRINT \
{\
    printf("\tstart: %ld, end %ld\n", start-context_begin, end-context_begin);\
    std :: cout << "\tcontent:" << std :: endl;\
    printf("\t\t");\
    direct_print(start, end, false);\
    std :: cout << "\tend" << std :: endl;\
}

void
Context :: defdir(std :: string &name, const char *start, const char *end)
{
    std :: cout << "defdir" << std :: endl;
    if (!name.empty())
        std :: cout << "name: " << name << std :: endl;
    else
        std :: cout << "no name" << std :: endl;
    PRINT
}

void
Context :: undefdir(std :: string &name)
{
    std :: cout << "undefdir" << std :: endl;
    if (!name.empty())
        std :: cout << "name: " << name << std :: endl;
    else
        std :: cout << "no name" << std :: endl;
}

void
Context :: noextend(std :: string &name, const char *start, const char *end)
{
    std :: cout << "noextend" << std :: endl;
    if (!name.empty())
        std :: cout << "name: " << name << std :: endl;
    else
        std :: cout << "no name" << std :: endl;
    PRINT
}

void
Context :: unnoextend(std :: string &name)
{
    std :: cout << "unnoextend" << std :: endl;
    if (!name.empty())
        std :: cout << "name: " << name << std :: endl;
    else
        std :: cout << "no name" << std :: endl;
}

void
Context :: raw(const char *start, const char *end)
{
    std :: cout << "raw" << std :: endl;
    PRINT
}

void
Context :: make(const char *start, const char *end)
{
    std :: cout << "make" << std :: endl;
    PRINT
}

Context :: ~Context()
{
    for(auto sub : subcontexts)
    {
        delete sub;
    }
    return;
}













