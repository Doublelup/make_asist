#include <iostream>
#include <vector>
#include <stack>
#include <map>
#include <thread>
#include <regex>
#include <sstream>
#include <atomic>
#include <assert.h>
#include <cstring>
// #include <stdexcept>
#include <condition_variable>
#include <mutex>

namespace dicts{
    typedef std :: string prefix_t, dict_name_t;

    struct item{
        // start point at the begin of the string.
        // length is the length of the string.
        const char *start;
        size_t length;
        item(const char *start, size_t length);
        item(const std :: csub_match &match);
        ~item();
        bool check(); // check only when need
    };
    
    enum dict_type{
        NAMED_DICT,
        DEFAULT_DICT,
    };

    struct regex_prefix_pair{
        std :: regex regex;
        prefix_t prefix;
        regex_prefix_pair(struct item &regex_str, struct item &prefix);
        regex_prefix_pair(const std :: csub_match &regex_str, const std :: csub_match &prefix);
        ~regex_prefix_pair();
    };

    typedef std :: vector<struct regex_prefix_pair*> prefix_pairs_t;
    class prefix_dict{
        private:
            std :: atomic_bool ready;
            prefix_pairs_t pairs;
            enum dict_type type;
            dict_name_t name;
            std :: condition_variable spark;
            std :: mutex mut;
            void wait_for_ready();
        public:
            // Promise that prefix_dict :: full check the state of dict.
            // In this case, the dict is unmodifiable, so prefix_dict :: full
            // is thread-safe.
            void get_prefix(struct item &item, bool &success, std :: string **rvl);
            prefix_dict(dict_name_t &name);
            prefix_dict(struct item &name);
            prefix_dict();
            ~prefix_dict();
            enum dict_type get_type();
            const std :: string &get_name();
            bool is_ready();
            // Functions below modifies the dict, so those functions can be
            // invoked only when the dict is not ready.
            void finish();
            void add_item(struct item &pattern_str, struct item &prefix);
            void add_pairs(prefix_pairs_t *newpairs); // Don't forget to free newpairs!
            void add_pairs(prefix_pairs_t &newpairs);
    };

    // Both ref_prefix_dicts and loc_prefix_dicts can NOT be shared among threads!
    // Conflicts show up if you want to use defdir and export_ref_dicts at once!
    
    const static size_t ADDITION_SPACE = 5;
    class prefix_dicts{
        protected:
            struct dict_with_state{
                prefix_dict *dict;
                bool state;
            };
            std :: vector<struct dict_with_state> *shelf;
            std :: map<dict_name_t, size_t> *name_index_map;
            const bool modifiable;
            prefix_dicts(bool modifiable);
            ~prefix_dicts();
        public:
            // When invoke prefix_dicts::defdir, the instance take over
            // duty to manage the dict.
            void defdir(prefix_dict *dict);
    };

    class loc_prefix_dicts : public prefix_dicts{
        public:
            loc_prefix_dicts();
            ~loc_prefix_dicts();
    };

    class ref_prefix_dicts : public prefix_dicts{
        public:
            ref_prefix_dicts();
            ref_prefix_dicts(bool modifiable, ref_prefix_dicts &source);
            ~ref_prefix_dicts();
            bool undefdir(dict_name_t &name);
            bool undefdir(struct item &name);
            const std :: string *full(struct item &item, bool &success);
            bool exist(dict_name_t &name);
    };

    class noextend_dict{
        typedef std :: vector<std :: regex> noextend_list_t;
        private:
            std :: atomic_bool ready;
            noextend_list_t list;
            dict_name_t name;
            enum dict_type type;
            std :: condition_variable spark;
            std :: mutex mut;
            void wait_for_ready();
        public:
            bool match(struct item &item);
            noextend_dict();
            noextend_dict(dict_name_t &name);
            noextend_dict(struct item &name);
            ~noextend_dict();
            enum dict_type get_type();
            const std :: string &get_name();
            bool is_ready();
            void finish();
            void add_item(struct item &item);
            void add_list(noextend_list_t *list);
            void add_list(noextend_list_t &list);
    };
    class noextend_dicts{
        protected:
            struct dict_with_state{
                noextend_dict *dict;
                bool state;
            };
            std :: vector<struct dict_with_state> *shelf;
            std :: map<dict_name_t, size_t> *name_index_map;
            const bool modifiable;
        public:
            noextend_dicts(bool modifiable);
            ~noextend_dicts();
            void noextend(noextend_dict *dict);
    };

    class loc_noextend_dicts : public noextend_dicts{
        public:
            loc_noextend_dicts();
            ~loc_noextend_dicts();
    };

    class ref_noextend_dicts : public noextend_dicts{
        public:
            ref_noextend_dicts();
            ref_noextend_dicts(bool modifiable, ref_noextend_dicts &source);
            ~ref_noextend_dicts();
            bool unnoextend(dict_name_t &name);
            bool unnoextend(struct item &name);
            bool match(struct item &item);
    };
};

class Context{
    private:
        std :: atomic_bool finish;
        std :: vector<class Context *> subcontexts;
        dicts :: loc_prefix_dicts lpd;
        dicts :: ref_prefix_dicts rpd;
        void is_finish();
    public:
        void defdir(std :: string &name, const char *start, const char *end);
        void undefdir(std :: string &name);
        void noextend(std :: string &name, const char *start, const char *end);
        void unnoextend(std :: string &name);
        void raw(const char *start, const char *end);

};

class Reader{
    private:
        std :: string file_name;
        class Context *root;
        class Context *crtcontext;
        std :: stack<class Context *> s;
    public:
        Reader(std :: string &file_name);
        ~Reader();
        void start();
        bool is_finish();
};

