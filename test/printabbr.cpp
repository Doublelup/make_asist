#include "../include/test.h"
#include <fstream>

int main()
{
    std :: string file_path = "./test.abbr";
    std :: ifstream file(file_path);
    if (!file.is_open()){
        std :: cout << "Can't open file: " << file_path << std :: endl;
        return 0;
    }
    std :: string content = {std :: istreambuf_iterator<char>(file),
                                std :: istreambuf_iterator<char>()};
    file.close();
    direct_print(content.c_str(), content.c_str() + content.size(), false);
}