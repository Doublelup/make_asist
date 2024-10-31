#include "parser.h"
#include "test.h"

constexpr size_t buffer_size = 64;
constexpr size_t redundancy_size = 2;

static
const char *
get_str_end(const char * const start){
    const char *p = start;
    while (*p)++p;
    return p;
}

void
test_regex_reader()
{
    const char *sample1 = "\n\t#\n\\\n\t\\#\\s*[.hrsl]{2,5}[a-zA-Z]\\\nhello^";
    const char *sample2 = "";
    const char *sample = sample2;
    char buffer[buffer_size + redundancy_size];
    int buf_ind = 0;
    buffer[buf_ind] = 0;
    const char *end = get_str_end(sample);
    if(!test.test_regex_reader(sample, end, buf_ind, buffer)){
        printf("test error\n");
    }
    direct_print(buffer, get_str_end(buffer), false);
}

void
test_name_reader()
{
    const char *sample1 = "\t\\\n\t\t\t\t\\ \\\t\\:$(SRC)/hello.c";
    const char *sample2 = "\t\\\n\" $(SRC)/hello.c\\\t:\\\"";
    const char *sample3 = "\t";
    const char *sample = sample3;
    char buffer[buffer_size + redundancy_size];
    int buf_ind = 0;
    buffer[buf_ind] = 0;
    const char *end = get_str_end(sample);
    printf("sample: ");
    direct_print(sample, end, false);
    if (!test.test_name_reader(sample, end, buf_ind, buffer)){
        printf("test_name_reader error\n");
    }
    direct_print(buffer, get_str_end(buffer), false);
}

void
test_read_mark()
{
    const char *samples[] = {
        "\t\t\t\t\tmark\t\n",
        "\n\t\t\t#comment\t\n\t\t\t\t\there",
        "\n\t\t\t\\\n\t\tmark  here\n\t",
        "#comment sloooow\t\t\n\t\t\t#comment\n\t\\ mark hello",
        "mark\t\t\t\n",
        "\t\t\t \tmark\n"
    };
    const char *end;
    int ret = 0;
    const char *start;
    for (int i = 0; i < sizeof(samples)/sizeof(const char*); ++i){
        start = samples[i];
        end = get_str_end(start);
        printf("sample%d: ", i);
        direct_print(start, end, false);
        if ((ret = test.test_read_mark(start, end)) == -1){
            printf("test_read_mark error: sample%d\n", i);
        }
        else{
            printf("sample%d: indent = %d, p stop at %ld\n", i, ret, start - samples[i]);
            printf("rest of sample: ");
            direct_print(start, end, false);
        }
    }
}

namespace reader{
    threadPool pool{};
    bool Reader::error = false;
};

void test_defdir(reader::Context *context)
{
    const char *samples[] = {
        "\t\t#comment\n\t\t .*.c:$(SRC)\n\t\t\"hello[a-z]\\.cpp\" \\\n\t\t:\\\n whoami #comment\n\t\t",
        "#comment \t\n#comment\n \"[hels]*.?\"\\\n  :\"helloxx$(SRC)\"#right\n.cc:nono\t\n#comment",
    };
    for (int i = 0; i < sizeof(samples)/sizeof(const char*); ++i){
        dicts::prefix_dict dict{};
        context->jobs_inc();
        test.defdir(dict, samples[i], get_str_end(samples[i]), context);
        assert(dict.is_ready());
        dicts::prefix_pairs_t *pairs = dict.access_pairs();
        for (dicts::regex_prefix_pair *pair : *pairs){
            std::cout << "prefix: " << pair->prefix << std::endl;
        }
    }
    assert(!reader::Reader::check_error());
}

void test_noextend(reader::Context *context)
{
    const char *samples[] = {
        "\t\t#comment\n\t\t .*.c #heeee\t\t\nnono\t\t",
    };
    for (int i = 0; i < sizeof(samples)/sizeof(const char*); ++i){
        dicts::noextend_dict dict{};
        context->jobs_inc();
        test.noextend(dict, samples[i], get_str_end(samples[i]), context);
        assert(dict.is_ready());
        
    }
    assert(!reader::Reader::check_error());
}

void test_raw(reader::Context *context)
{
    printf("[test_raw]: begin\n");
    const char *samples[] = {
        "#sample\t\t\n\t\t\t\tmark\t\t#mark\t\t\n\t\t\t\there is me  \\\n nononono#nono\n\t\t\t\t\t\toh yes\n\t\t",
        "\t\t#sample \n\t\t\t\t\t\there is me\n\n\t\t\t\t\t\t\t\tno yes #no yes\t\n\t\t\tyes",
        "test_packer : test_packer.cpp connect_info_table.h\n"
    };
    for (int i = 0; i < sizeof(samples)/sizeof(const char*); ++i){
        std::string *output = context->add_input();
        context->jobs_inc();
        test.raw(output, samples[i], get_str_end(samples[i]), context);
        direct_print((*output).c_str(), get_str_end((*output).c_str()), true);
    }
    assert(!reader::Reader::check_error());
    printf("[test_raw]: finish\n\n");
}

void test_make(reader::Context *context)
{
    printf("[test_make]: begin\n");
    dicts::prefix_dict *pd = new dicts::prefix_dict{};
    dicts::noextend_dict *nd = new dicts::noextend_dict{};
    context->add_prefix_dict(pd);
    context->add_noextend_dict(nd);
    const char *defdir_lines = "\t\t\n\".*\\.c\":\"$(SRC)\"\t\t\n\t\".*\\.h\":\"$(INCLUDE)\"\t\t\n";
    const char *noextend_lines = "\t\t\n \"^hello.c$\" \"^[yhs]o.h$\"";
    context->jobs_inc();
    test.defdir(*pd, defdir_lines, get_str_end(defdir_lines), context);
    context->jobs_inc();
    test.noextend(*nd, noextend_lines, get_str_end(noextend_lines), context);
    const char *sample = "\t\t#this is a sample\t\n\t\t\t\t mark #this is a mark\n\t\t\t\thello.c:hello.o hello.h #test\n\t\t\t\t\tho.c : ho.h\\\n\t\t\t\t\t\t\t\tho.o";
    context->jobs_inc();
    std::string *output = context->add_input();
    dicts::ref_prefix_dicts *rpd = new dicts::ref_prefix_dicts{false, context->access_rpd()};
    dicts::ref_noextend_dicts *rnd = new dicts::ref_noextend_dicts{false, context->access_rnd()};
    test.make(output, rpd, rnd, sample, get_str_end(sample), context);
    direct_print((*output).c_str(), get_str_end((*output).c_str()), true);
    assert(!reader::Reader::check_error());
    printf("[test_make]: finish\n\n");
}

// void test_context()
// {
//     printf("[test_context]: begin\n");
// 
//     std::string output;
//     reader::Context context(&output);
//     const char *defdir_sample = "\t\t#comment\n\t\t .*\\.c:$(SRC)\n\t\t\"hello[a-z]\\.cpp\" \\\n\t\t:\\\n whoami #comment\n\t\t";
//     const char *noectend_sample = "\t\t\t\t\t\n ^hello.c$\n";
//     const char *make_sample = 
//     
// 
//     printf("[test_context]: finish\n\n");
// }

int main(){
    // test basic units.
    // test_regex_reader();
    // test_name_reader();
    // test_read_mark();
    // test context.
    std::string output;
    reader::Context *context = new reader::Context{&output};
    //test_defdir(&context);
    //test_noextend(&context);
    test_raw(context);
    test_make(context);
    context->jobs_inc();
    reader::Context::_end(context);
    delete context;
    printf("result: \n");
    direct_print(output.c_str(), get_str_end(output.c_str()), true);
    return 0;
}
