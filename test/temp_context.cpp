#include "../include/parser.h"
#include <stdio.h>


using namespace reader;

void
Context :: is_finish()
{
    return;
}

void
Context :: defdir(std :: string &name, const char *start, const char *end)
{
    std :: cout << "defdir" << std :: endl;
    if (!name.empty())
        std :: cout << "name: " << name << std :: endl;
    else
        std :: cout << "no name" << std :: endl;
    printf("start: %p, end %p\n", start, end);
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
    printf("start: %p, end %p\n", start, end);
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
    printf("start: %p, end %p\n", start, end);
}

void
Context :: make(const char *start, const char *end)
{
    std :: cout << "make" << std :: endl;
    printf("start: %p, end %p\n", start, end);
}

Context :: ~Context()
{
    for(auto sub : subcontexts)
    {
        delete sub;
    }
    return;
}













