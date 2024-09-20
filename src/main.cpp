#include "../include/parser.h"
#include "../include/exception.h"

struct{
    int maxThreadNum; // "-j"
    std::string abbrFilePath; // "-i"
    std::string dependenceFilePath; // "-o"
}flags;

namespace reader{
    threadPool pool{};
    bool Reader::error = false;
};

int main(int argc, char *argv[]) {
    const char *default_abbr_path = "./rule.abbr";
    const char *default_dependence_path = "./dependence.mk";
    constexpr int default_thread_num = 5;
    constexpr int thread_threshold = 10;

    flags.abbrFilePath = default_abbr_path;
    flags.dependenceFilePath = default_dependence_path;
    flags.maxThreadNum = default_thread_num;

    for (int i = 1; i < argc - 1; ++i) {
        if (!strcmp(argv[i], "-j")){
            ++i;
            flags.maxThreadNum = atoi(argv[i]);
            if (flags.maxThreadNum <= 0) {
                fprintf(stderr, "Thread num should be no less than 1.\n");
                exit(-1);
            }
            if (flags.maxThreadNum > thread_threshold)
                flags.maxThreadNum = thread_threshold;
        }
        else if (!strcmp(argv[i], "-i")) {
            ++i;
            flags.abbrFilePath = argv[i];
        }
        else if (!strcmp(argv[i], "-o")) {
            ++i;
            flags.dependenceFilePath = argv[i];
        }
        else{
            fprintf(stderr, "argv[%d] error!\n", i);
            exit(-1);
        }
    }

    reader::pool.init(flags.maxThreadNum);
    try{
        reader::Reader r(flags.abbrFilePath);
        std::ofstream output(flags.dependenceFilePath);
        if (output.is_open()){
            r.start();
            if (reader::Reader::check_error()) {
                std::cerr << "Error detected in file: " << flags.abbrFilePath << std::endl;
            }    
            else output << r.output();
            output.close();
        }
        else throw exception_with_code(1, "Main [cannot open output file]");
    }
    catch (const exception_with_code &e){
        std::cout << e.what() << std::endl;
    }
    return 0;
}







