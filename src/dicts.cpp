#include "../include/parser.h"

using namespace dicts;

item :: item(const char *s, size_t l)
:   start{s},
    length{l}
{
}

item :: ~item()
{}

bool
item :: check()
{
    if (!start || length <= 0)return false;
    else return true;
}

regex_prefix_pair :: regex_prefix_pair(struct item &regex_str, struct item &prefix)
{
    if (!regex_str.check())
        throw std :: runtime_error("regex_prefix_pair 1 [regex_ptr error]");
    else regex = {regex_str.start, regex_str.length, std :: regex_constants :: ECMAScript};
    if (prefix.start)
        this->prefix = {prefix.start, prefix.length};
    else this->prefix = "";
}

regex_prefix_pair :: regex_prefix_pair(std :: csub_match &regex_str, std :: csub_match &prefix)
{
    if (!regex_str.first || regex_str.length() <= 0 )
        throw std :: runtime_error("regex_prefix_pair 2 [regex_ptr should not be null or empty]");
    // std :: csub_match :: length() should never return a nagtive number, so just cast it!
    regex = std :: regex{regex_str.first, (size_t)regex_str.length()};
    if (prefix.first)
        this->prefix = std :: string{prefix.first, (size_t)prefix.length()};
    else this->prefix = "";
}

regex_prefix_pair :: ~regex_prefix_pair()
{}

prefix_dict :: prefix_dict(dict_name_t &name)
:   ready{false},
    pairs{},
    name{name},
    type{NAMED_DICT}
{
    if (name.length() <= 0)
        throw std :: runtime_error("prefix_dict 1 [name error]");
    sem_init(&spark, 0, 0);
}

prefix_dict :: prefix_dict(struct item &name)
:   ready{false},
    pairs{},
    type{NAMED_DICT}
{
    if (!name.check())
        throw std :: runtime_error("prefix_dict 2 [name error]");
    else this->name = {name.start, name.length};
    sem_init(&spark, 0, 0);
}

prefix_dict :: prefix_dict()
:   ready{false},
    pairs{},
    name{},
    type{DEFAULT_DICT}
{
    sem_init(&spark, 0, 0);
}

prefix_dict :: ~prefix_dict()
{
    assert(is_ready());
    sem_wait(&spark);
    sem_destroy(&spark);
    for (auto pair : pairs)
    {
        delete pair;
    }
}

enum dict_type
prefix_dict :: get_type()
{
    return type;
}

const std :: string&
prefix_dict :: get_name()
{
    assert(type == NAMED_DICT);
    return name;
}

void
prefix_dict :: wait_for_ready()
{
    if(!is_ready())
    {
        sem_wait(&spark);
        sem_post(&spark);
    }
    return;
}

void
prefix_dict :: finish()
{
    assert(!is_ready());
    sem_post(&spark);
    ready = true;
    return;
}

bool
prefix_dict :: is_ready()
{
    bool ret = false;
    if (ready) ret = true;
    return ret;
}



const std :: string
prefix_dict :: full(struct item &item, bool &success)
{
    wait_for_ready();
    std :: string ret = {};
    std :: string sitem = {item.start, item.length};
    for (size_t i = pairs.size()-1; i >= 0; --i)
    {
        if (std :: regex_match(sitem, pairs[i]->regex))
        {
            ret = pairs[i]->prefix;
            break;
        }
    }
    if (ret.empty()){
        success = false;
    }
    else success = true;
    ret += sitem;
    return ret;
}

void
prefix_dict :: add_item(struct item &pattern_str, struct item &prefix)
{
    assert(!is_ready());
    auto pair = new struct regex_prefix_pair(pattern_str, prefix);
    pairs.push_back(pair);
    return;
}

void
prefix_dict :: add_pairs(prefix_pairs_t *newpairs)
{
    assert(!is_ready());
    if (!newpairs) return;
    pairs.insert(pairs.end(), newpairs->begin(), newpairs->end());
    return;
}

void
prefix_dict :: add_pairs(prefix_pairs_t &newpairs)
{
    assert(!is_ready());
    pairs.insert(pairs.end(), newpairs.begin(), newpairs.end());
    return;
}

prefix_dicts :: prefix_dicts(bool modifiable)
:   shelf{NULL},
    name_index_map{NULL},
    modifiable{modifiable}
{
}

prefix_dicts :: ~prefix_dicts()
{
}

void 
prefix_dicts :: defdir(prefix_dict *dict)
{
    assert(shelf && name_index_map);
    assert(modifiable);
    if(dict->get_type() == NAMED_DICT)
    {
        // not exist in map
        const std :: string &name = dict->get_name();
        auto result = name_index_map->find(name);
        if (result != name_index_map->end()) 
        {
            shelf->push_back({dict, true});
            name_index_map->insert({name, shelf->size()-1});
        }
        else{
            shelf->push_back({dict, true});
            result->second = shelf->size() - 1;
        }
    }
    else{
        shelf->push_back({dict, true});
    }
}

loc_prefix_dicts :: loc_prefix_dicts()
:   prefix_dicts{true}
{
    assert(!shelf && !name_index_map);
    shelf = new std :: vector<struct dict_with_state>{};
    name_index_map = new std :: map<dict_name_t, size_t>{};
}

loc_prefix_dicts :: ~loc_prefix_dicts()
{
    for (auto dict : *shelf)
    {
        delete dict.dict;
    }
    delete shelf;
    delete name_index_map;
}

ref_prefix_dicts :: ref_prefix_dicts()
:   prefix_dicts{true}
{
    shelf = new std :: vector<struct dict_with_state>{};
    name_index_map = new std :: map<dict_name_t, size_t>{};
}

ref_prefix_dicts :: ref_prefix_dicts(bool modifiable, ref_prefix_dicts &source)
:   prefix_dicts{modifiable}
{
    assert(!shelf && !name_index_map);

    shelf = new std :: vector<struct dict_with_state>{};
    shelf->reserve(source.shelf->size() + ADDITION_SPACE);
    // Copy the dict list, pick 'undefdir'ed out.
    std :: vector<size_t> asist(source.shelf->size());
    for (size_t i = 0, j = 0; i < source.shelf->size(); ++i){
        if ((*source.shelf)[i].state)
        {
            assert(j == shelf->size());
            shelf->push_back((*source.shelf)[i]);
            asist[i] = j;
            ++j;
        }
    }
    // Refresh map.
    assert(source.name_index_map);
    name_index_map = new std :: map<dict_name_t, size_t>{*source.name_index_map};
    for (auto pair : *name_index_map)
    {
        pair.second = asist[pair.second];
        assert(pair.first == (*shelf)[pair.second].dict->get_name());
    }
}

ref_prefix_dicts :: ~ref_prefix_dicts()
{
    delete shelf;
    delete name_index_map;
}

bool
ref_prefix_dicts :: undefdir(dict_name_t &name)
{
    assert(modifiable);
    bool ret = false;
    auto result = name_index_map->find(name);
    if (result != name_index_map->end())
    {
        assert ((*shelf)[result->second].state == true);
        (*shelf)[result->second].state = false;
        ret = true;
        name_index_map->erase(name);
    }
    return ret;
}

bool
ref_prefix_dicts :: exist(dict_name_t &name)
{
    assert(!modifiable);
    auto result = name_index_map->find(name);
    bool ret = false;
    if (result != name_index_map->end()) ret = true;
    return ret;
}

const std :: string
ref_prefix_dicts :: full(struct item &item, bool &success)
{
    assert(item.check());
    assert(!modifiable);
    assert(!success);
    std :: string ret;
    for (size_t i = shelf->size()-1; i >= 0; --i)
    {
        if ((*shelf)[i].state)
        {
            ret = (*shelf)[i].dict->full(item, success);
            if(success)break;
        }
    }
    return ret;
}






















