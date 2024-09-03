#include "../include/parser.h"
#include <stdio.h>
#include <random>

using namespace dicts;
void item_print(struct item &it)
{
    if (it.check())
    {
        printf("start: %p, ", it.start);
        printf("content: %.*s, ", (int)it.length, it.start);
        printf("length: %ld\n", it.length);
    }
    else{
        printf("Invalid item: start %p, length %ld\n", it.start, it.length);
    }
}

bool test_for_item()
{
    bool ret = false;
    const char hello[] = "Hello";
    item it1{hello, strlen(hello)};
    item it2{NULL, 0};
    char s[] = "^He.*$";
    std :: regex re{s};
    std :: cmatch matches;
    std :: regex_match(hello, matches, re);
    item it3{matches[0]};
    if(it1.start == it3.start && it1.length == it3.length && !it2.check()){
        printf("struct item: [Pass]\n");
        ret = true;
    }
    else{
        printf("struct item: [Fail]\n");
    }
    return ret;
}

bool test_for_regex_prefix_pair()
{
    bool ret = false;
    const char *r = "^H.*$";
    const char *s = "Hello";
    item it1 = {r, strlen(r)};
    item it2 = {s, strlen(s)};
    regex_prefix_pair it3 = {it1, it2};
    const char *p = "^H.*$ : Hello";
    std :: regex re{"(.*) : (.*)"};
    std :: cmatch matches;
    std :: regex_match(p, matches, re);
    regex_prefix_pair it4 = {matches[1], matches[2]};
    if(std :: regex_match(s, it3.regex) && std :: regex_match(s, it4.regex) && it3.prefix == it4.prefix){
        printf("struct regex_prefix_pair: [Pass]\n");
        ret = true;
    }
    else{
        printf("struct regex_prefix_pair: [Fail]\n");
    }
    return ret;
}

void sub_test_for_prefix_dict1(prefix_dict &dict, std :: vector<std :: string*> &rvls)
{
    const static int max_length = 20;
    const static int test_size = 2;
    std :: random_device rd;
    std :: mt19937 gen(rd());
    std :: uniform_int_distribution<> distri_length(1,max_length-1);
    std :: uniform_int_distribution<> distri_alpha(0, 25);

    char test_strs[test_size][max_length];
    item *its[test_size];
    bool successes[test_size];
    std :: string *_rvls[test_size];
    for (int i = 0; i < test_size; ++i){
        for (int j = 0; j < max_length; ++j){
            test_strs[i][j] = 'a' + (char)distri_alpha(gen);
        }
        test_strs[i][distri_length(gen)] = '\0';
        its[i] = new item{test_strs[i], strlen(test_strs[i])};
        successes[i] = false;
        _rvls[i] = NULL;
    }
    std :: thread *ts[test_size];
    for (int i = 0; i < test_size; ++i)
    {
        std :: string **rvl = _rvls + i;
        ts[i] = new std :: thread{&prefix_dict :: get_prefix, std :: ref(dict), std ::ref(*its[i]), std :: ref(successes[i]), rvl};
    }
    for (int i = 0; i < test_size; ++i)
    {
        ts[i]->join();
        rvls.push_back(_rvls[i]);
        delete ts[i];
        delete its[i];
    }
}

void sub_test_for_prefix_dict2(prefix_dict &dict)
{
    const char *patterns[] = {
        "^h[a-z]*e?$",
        "^\\.h+[a-z]?$",
        "^a.*\\.e$",
        "^[A-Z]?\\.C+$",
        "^[a-zA-Z0-9]{2}\\..*[a-z]$",
        "^[A-Z]*\\.c{2,3}$",
        "^\\.a*[0-9]?\\.?$",
        "^d\\.e\\.F{1,2}$",
        "^[a-z]+\\.M+\\.$",
        "^[0-9]{3,}\\.?[a-z]*$",
        "^\\.p{1,3}[A-Z]?\\.$",
        "^[A-Z]?\\.b*X$",
        "^[A-Z]+\\.T*\\.?$",
        "^[A-Z]{2,3}\\.G{1,2}\\.$",
        "^[a-zA-Z]\\.q{2}l$",
        "^\\.[a-z]{1,2}\\.9{2}$",
        "^[a-z]+\\.O{1,3}[0-9]*$",
        "^[A-Z]?\\.D+e?$",
        "^[0-9]{2}\\.r{1}[A-Z]+$",
        "^[a-z]*\\.T*8$"
    };
    const char *prefixs[]={
        "Smith",
        "Johnson",
        "Williams",
        "Brown",
        "Jones",
        "Miller",
        "Davis",
        "Garcia",
        "Rodriguez",
        "Wilson",
        "Martinez",
        "Anderson",
        "Taylor",
        "Thomas",
        "Hernandez",
        "Moore",
        "Martin",
        "Jackson",
        "Thompson",
        "White"
    };
    for (int i = 0; i < 20; ++i){
        item a1 = {patterns[i], strlen(patterns[i])};
        item a2 = {prefixs[i], strlen(prefixs[i])};
        dict.add_item(a1, a2);
    }
    // std :: cout << dict.is_ready() << std :: endl;
    dict.finish();
    // std :: cout << dict.is_ready() << std :: endl;
}

bool test_for_prefix_dict()
{
    dict_name_t s = "hello";
    const static int thread_num = 10;
    prefix_dict hello(s);
    std :: thread *t[thread_num];
    std :: vector<std :: string*> rvls[thread_num];
    for (int i = 0; i < thread_num; ++i)
    {
        t[i] = new std :: thread{sub_test_for_prefix_dict1, std :: ref(hello), std :: ref(rvls[i])};
    }
    std :: thread constructor{sub_test_for_prefix_dict2, std :: ref(hello)};
    for (int i = 0; i < thread_num; ++i)
    {
        t[i]->join();
    }
    constructor.join();
    for (int i = 0; i < thread_num; ++i)
    {
        // std :: cout << "Result from thread " << i << std :: endl;
        for(int j = 0; j < rvls[i].size(); ++j)
        {
            // std :: cout << "\t" << *rvls[i][j] << std :: endl;
            if(rvls[i][j]) delete rvls[i][j];
        }
    }
    for (int i = 0; i < thread_num; ++i)
    {
        delete t[i];
    }
    std :: cout << "prefix_dict [Pass]" << std :: endl;
    // std :: cout << hello.get_name() << hello.get_type() << std :: endl;
    return true;
}

bool test_for_loc_prefix_dicts()
{
    static const int empty_dict_num = 10;
    dict_name_t s = "hello";
    prefix_dict *d0 = new prefix_dict{s};
    std :: thread constructor{sub_test_for_prefix_dict2, std :: ref(*d0)};
    prefix_dict *empty_dicts[empty_dict_num];
    for(int i = 0; i < empty_dict_num; ++i)
    {
        empty_dicts[i] = new prefix_dict{};
        empty_dicts[i]->finish();
    }
    loc_prefix_dicts dicts{};
    dicts.defdir(d0);
    for(int i = 0; i < empty_dict_num; ++i)
    {
        dicts.defdir(empty_dicts[i]);
    }
    constructor.join();
    std :: cout << "loc_prefix_dicts [Pass]" << std :: endl;
    return true;
}

bool test_for_ref_prefix_dicts()
{
    static const int empty_dict_num = 10;
    dict_name_t s = "hello";
    dict_name_t t = "world";
    prefix_dict *d0 = new prefix_dict{s};
    prefix_dict *d1 = new prefix_dict{t};
    std :: thread constructor1{sub_test_for_prefix_dict2, std :: ref (*d0)};
    std :: thread constructor2{sub_test_for_prefix_dict2, std :: ref (*d1)};
    prefix_dict *empty_dicts[empty_dict_num];
    for(int i = 0; i < empty_dict_num; ++i)
    {
        empty_dicts[i] = new prefix_dict();
        empty_dicts[i]->finish();
    }
    loc_prefix_dicts loc_dicts{};
    ref_prefix_dicts ref_dicts{};
    loc_dicts.defdir(d0);
    loc_dicts.defdir(d1);
    ref_dicts.defdir(d0);
    ref_dicts.defdir(d1);
    for(int i = 0; i < empty_dict_num; ++i)
    {
        loc_dicts.defdir(empty_dicts[i]);
        ref_dicts.defdir(empty_dicts[i]);
    }
    ref_prefix_dicts cp_ref_dicts1{false, ref_dicts};
    ref_dicts.undefdir(s);
    ref_prefix_dicts cp_ref_dicts2{false, ref_dicts};
    const char *itstr = "hello";
    item it{itstr, strlen(itstr)};
    bool success1 = false, success2 = false;
    const std :: string *str1 = cp_ref_dicts1.full(it, success1);
    const std :: string *str2 = cp_ref_dicts2.full(it, success2);
    
    assert(success1 && !success2);
    // std :: cout << *str1 << std :: endl;
    // std :: cout << *str2 << std :: endl;
    constructor1.join();
    constructor2.join();
    delete str1;
    delete str2;
    std :: cout << "ref_prefix_dicts [Pass]" << std :: endl;
    return true;
}

int main()
{
    test_for_item();
    test_for_regex_prefix_pair();
    test_for_prefix_dict();
    test_for_loc_prefix_dicts();
    test_for_ref_prefix_dicts();
    return 0;
}