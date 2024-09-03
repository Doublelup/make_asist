#include "../include/parser.h"

using namespace dicts;

item :: item(const char *s, size_t l)
:   start{s},
    length{l}
{
}

item :: item(const std :: csub_match &match)
:   start{match.first},
    length{(size_t)(match.second - match.first)}
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
    assert(regex_str.check());
    regex = {regex_str.start, regex_str.length, std :: regex_constants :: ECMAScript};
    if (prefix.start)
        this->prefix = {prefix.start, prefix.length};
    else this->prefix = "";
}

regex_prefix_pair :: regex_prefix_pair(const std :: csub_match &regex_str, const std :: csub_match &prefix)
{
    assert(regex_str.first && regex_str.length() > 0);
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
    type{NAMED_DICT},
    mut{},
    spark{}
{
    assert(name.length() > 0);
}

prefix_dict :: prefix_dict(struct item &name)
:   ready{false},
    pairs{},
    type{NAMED_DICT},
    mut{},
    spark{}
{
    assert(name.check());
    this->name = {name.start, name.length};
}

prefix_dict :: prefix_dict()
:   ready{false},
    pairs{},
    name{},
    type{DEFAULT_DICT},
    mut{},
    spark{}
{
}

prefix_dict :: ~prefix_dict()
{
    assert(is_ready());
    std :: unique_lock<std :: mutex> lock(mut);
    for (auto &pair : pairs)
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
    std :: unique_lock<std :: mutex> lock{mut};
    spark.wait(lock, [this]{return is_ready();});
    return;
}

void
prefix_dict :: finish()
{
    assert(!is_ready());
    std :: unique_lock<std :: mutex> lock{mut};
    ready = true;
    spark.notify_all();
    return;
}

bool
prefix_dict :: is_ready()
{
    bool ret = false;
    if (ready) ret = true;
    return ret;
}



void
prefix_dict :: get_prefix(struct item &item, bool &success, std :: string **rvl)
{
    assert(!*rvl);
    assert(!success);
    wait_for_ready();
    std :: string sitem = {item.start, item.length};
    int dict_size = pairs.size();
    for (int i = 0; i < dict_size; ++i)
    {
        if (std :: regex_match(sitem, pairs[i]->regex))
        {
            *rvl = new std :: string{pairs[i]->prefix};
            success = true;
            break;
        }
    }
    return;
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
        const std :: string &name = dict->get_name();
        auto result = name_index_map->find(name);
        if (result == name_index_map->end()) 
        {
            // not exist in map, insert dict into shelf, and create name-index pair
            shelf->push_back({dict, true});
            name_index_map->insert({name, shelf->size()-1});
        }
        else{
            // exist in map, insert dict into shelf, and update name-index pair.
            shelf->push_back({dict, true});
            result->second = shelf->size() - 1;
        }
    }
    else{
        // default dict, just insert to shelf.
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
    for (auto &dict : *shelf)
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

// ref_prefix_dicts shouldn't be shared by threads, so just use referrence.
ref_prefix_dicts :: ref_prefix_dicts(bool modifiable, ref_prefix_dicts &source)
:   prefix_dicts{modifiable}
{
    assert(!shelf && !name_index_map);

    size_t ssize = source.shelf->size();
    shelf = new std :: vector<struct dict_with_state>{};
    shelf->reserve(ssize + ADDITION_SPACE);
    // Copy the dict list, pick 'undefdir'ed out.
    std :: vector<size_t> asist(ssize);
    for (size_t i = 0, j = 0; i < ssize; ++i){
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
    for (auto &pair : *name_index_map)
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
        name_index_map->erase(result);
    }
    return ret;
}

bool
ref_prefix_dicts :: undefdir(struct item &name)
{
    assert(modifiable);
    bool ret = false;
    std :: string sname = {name.start, name.length};
    auto result = name_index_map->find(sname);
    if (result != name_index_map->end())
    {
        assert ((*shelf)[result->second].state == true);
        (*shelf)[result->second].state = false;
        ret = true;
        name_index_map->erase(result);
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

const std :: string*
ref_prefix_dicts :: full(struct item &item, bool &success)
{
    assert(item.check());
    assert(!modifiable);
    assert(!success);
    std :: string *ret = NULL;
    for (int i = shelf->size()-1; i >= 0; --i)
    {
        if ((*shelf)[i].state)
        {
            (*shelf)[i].dict->get_prefix(item, success, &ret);
            if(success)break;
        }
    }
    if (!ret){
        ret = new std :: string{item.start, item.length};
    }
    else{
        *ret += std :: string{item.start, item.length};
    }
    return ret;
}

noextend_dict :: noextend_dict()
:   ready{false},
    list{},
    name{},
    type{DEFAULT_DICT},
    spark{},
    mut{}
{
}

noextend_dict :: noextend_dict(dict_name_t &name)
:   ready{false},
    list{},
    name{name},
    type{NAMED_DICT},
    spark{},
    mut{}
{
    assert(name.length() > 0);
}

noextend_dict :: noextend_dict(struct item &name)
:   ready{false},
    list{},
    type{NAMED_DICT},
    mut{},
    spark{}
{
    assert(name.check());
    this->name = {name.start, name.length};
}

noextend_dict :: ~noextend_dict()
{
    assert(is_ready());
    std :: unique_lock<std :: mutex> lock{mut};
    // std :: vector is in charge of regex.
}

enum dict_type
noextend_dict :: get_type()
{
    return type;
}

const std :: string&
noextend_dict :: get_name()
{
    assert(type == NAMED_DICT);
    return name;
}

void
noextend_dict :: wait_for_ready()
{
    std :: unique_lock<std :: mutex> lock{mut};
    spark.wait(lock, [this]{return is_ready();});
    return;
}

void
noextend_dict :: finish()
{
    assert(!is_ready());
    std :: unique_lock<std :: mutex> lock{mut};
    ready = true;
    spark.notify_all();
    return;
}

bool
noextend_dict :: is_ready()
{
    bool ret = false;
    if (ready) ret = true;
    return ret;
}

bool
noextend_dict :: match(struct item &item)
{
    wait_for_ready();
    bool ret = false;
    std :: string sitem = {item.start, item.length};
    int dict_size = list.size();
    for (int i = 0; i < dict_size; ++i)
    {
        if (std :: regex_match(sitem, list[i]))
        {
            ret = true;
            break;
        }
    }
    return ret;
}

void
noextend_dict :: add_item(struct item &item)
{
    assert(!is_ready());
    assert(item.check());
    list.push_back(std :: regex{item.start, item.length, std :: regex_constants :: ECMAScript});
    return;
}

void
noextend_dict :: add_list(noextend_list_t *newlist)
{
    assert(!is_ready());
    if (!newlist) return;
    list.insert(list.end(), newlist->begin(), newlist->end());
    return;
}

void
noextend_dict :: add_list(noextend_list_t &newlist)
{
    assert(!is_ready());
    list.insert(list.end(), newlist.begin(), newlist.end());
    return;
}

noextend_dicts :: noextend_dicts(bool modifiable)
:   shelf{NULL},
    name_index_map{NULL},
    modifiable{modifiable}
{
}

noextend_dicts :: ~noextend_dicts()
{
}

void
noextend_dicts :: noextend(noextend_dict *dict)
{
    assert(shelf && name_index_map);
    assert(modifiable);
    if (dict->get_type() == NAMED_DICT)
    {
        const std :: string &name = dict->get_name();
        auto result = name_index_map->find(name);
        if (result == name_index_map->end())
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

loc_noextend_dicts :: loc_noextend_dicts()
:   noextend_dicts{true}
{
    assert(!shelf && !name_index_map);
    shelf = new std :: vector<struct dict_with_state>{};
    name_index_map = new std :: map<dict_name_t, size_t>{};
}

loc_noextend_dicts :: ~loc_noextend_dicts()
{
    for (auto &dict : *shelf)
    {
        delete dict.dict;
    }
    delete shelf;
    delete name_index_map;
}

ref_noextend_dicts :: ref_noextend_dicts()
:   noextend_dicts{true}
{
    shelf = new std :: vector<struct dict_with_state>{};
    name_index_map = new std :: map<dict_name_t, size_t>{};
}

ref_noextend_dicts :: ref_noextend_dicts(bool modifiable, ref_noextend_dicts &source)
:   noextend_dicts{modifiable}
{
    assert(!shelf && !name_index_map);

    size_t ssize = source.shelf->size();
    shelf = new std :: vector<struct dict_with_state>{};
    shelf->reserve(ssize + ADDITION_SPACE);
    std :: vector<size_t> asist(ssize);
    for (size_t i = 0, j = 0; i < ssize; ++i)
    {
        if ((*source.shelf)[i].state)
        {
            assert(j == shelf->size());
            shelf->push_back((*source.shelf)[i]);
            asist[i] = j;
            ++j;
        }
    }

    assert(source.name_index_map);
    name_index_map = new std :: map<dict_name_t, size_t>{*source.name_index_map};
    for (auto &pair : *name_index_map)
    {
        pair.second = asist[pair.second];
        assert(pair.first == (*shelf)[pair.second].dict->get_name());
    }
}

ref_noextend_dicts :: ~ref_noextend_dicts()
{
    delete shelf;
    delete name_index_map;
}

bool
ref_noextend_dicts :: unnoextend(dict_name_t &name)
{
    assert(modifiable);
    bool ret = false;
    auto result = name_index_map->find(name);
    if (result != name_index_map->end())
    {
        assert((*shelf)[result->second].state == true);
        (*shelf)[result->second].state = false;
        ret = true;
        name_index_map->erase(result);
    }
    return ret;
}

bool
ref_noextend_dicts :: unnoextend(struct item &name)
{
    assert(modifiable);
    bool ret = false;
    std :: string sname = {name.start, name.length};
    auto result = name_index_map->find(sname);
    if (result != name_index_map->end())
    {
        assert((*shelf)[result->second].state == true);
        (*shelf)[result->second].state = false;
        ret = true;
        name_index_map->erase(result);
    }
    return ret;
}

bool
ref_noextend_dicts :: match(struct item &item)
{
    assert(item.check());
    assert(!modifiable);
    bool ret = false;
    for (int i = shelf->size()-1; i >= 0; --i)
    {
        if ((*shelf)[i].state)
        {
            ret = (*shelf)[i].dict->match(item);
            if(ret) break;
        }
    }
    return ret;
}


















