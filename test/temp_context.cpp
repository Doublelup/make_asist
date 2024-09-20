#include "../include/parser.h"
#include "../include/test.h"

using namespace reader;

const char *context_begin;

#define PRINT \
{\
    printf("\tstart: %ld, end %ld\n", start-context_begin, end-context_begin);\
    std :: cout << "\tcontent:" << std :: endl;\
    printf("\t\t");\
    direct_print(start, end, false);\
    std :: cout << "\tend" << std :: endl;\
}

void
Context :: defdir(int order, std :: string &name, const char *start, const char *end)
{
    (void)order;
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
Context :: noextend(int order, std :: string &name, const char *start, const char *end)
{
    (void)order;
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
Context :: raw(int order, const char *start, const char *end)
{
    (void)order;
    std :: cout << "raw" << std :: endl;
    PRINT
}

void
Context :: make(int order, const char *start, const char *end)
{
    (void)order;
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













