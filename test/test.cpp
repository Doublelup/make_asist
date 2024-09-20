#include "../include/test.h"

void direct_print(const char *start, const char *end, bool verbose)
{
    assert((end - start) >= 0);
    const char *p = start;
    while (*p && p != end){
        if (!verbose)
        {
            switch (*p){
                case '\n': printf("[\\n]"); break;
                case '\t': printf("[\\t]"); break;
                case ' ' : printf("[ ]"); break;
                default: printf("%c", *p);break;
            }
        }
        else{
            switch (*p){
                case '\n': printf("[\\n]\n"); break;
                case '\t': printf("[\\t]"); break;
                case ' ':printf("[ ]"); break;
                default: printf("%c", *p);break;
            }
        }
        ++p;
    }
    printf("[end]");
    printf("\n");
    return;
}