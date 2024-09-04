#include "../include/parser.h"
#include "../include/test.h"

int main()
{
    std :: string file_path = "./test.abbr";
    reader :: Reader r(file_path);
    context_begin = r.get_p();
    r.start();
    return 0;
}