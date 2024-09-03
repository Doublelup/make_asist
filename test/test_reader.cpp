#include "../include/parser.h"

int main()
{
    std :: string file_path = "./test.abbr";
    reader :: Reader r(file_path);
    r.start();
    return 0;
}