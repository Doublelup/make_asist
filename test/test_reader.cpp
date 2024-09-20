#include "../include/parser.h"
#include "../include/test.h"

static
const char *
get_str_end(const char * const start){
    const char *p = start;
    while (*p)++p;
    return p;
}

namespace reader{
    threadPool pool{};
    bool Reader::error = false;
};

int main()
{
    reader::pool.init(5);
    std :: string file_path = "./test.abbr";
    reader :: Reader r(file_path);
    // context_begin = r.get_p();
    r.start();
    const char *p = r.output().c_str();
    direct_print(p, get_str_end(p), true);
    return 0;
}