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
#include <stdexcept>
#include <semaphore.h>

namespace dicts{
    typedef std :: string prefix_t, dict_name_t;

    struct item{
        const char *start;
        size_t length;
        item(const char *start, size_t length);
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
        regex_prefix_pair(std :: csub_match &regex_str, std :: csub_match &prefix);
        ~regex_prefix_pair();
    };

    typedef std :: vector<struct regex_prefix_pair*> prefix_pairs_t;
    class prefix_dict{
        protected:
            void finish();
        private:
            std :: atomic_bool ready;
            prefix_pairs_t pairs;
            enum dict_type type;
            dict_name_t name;
            sem_t spark;
            void wait_for_ready();
        public:
            // Promise that prefix_dict :: full check the state of dict.
            // In this case, the dict is unmodifiable, so prefix_dict :: full
            // is thread-safe.
            const std :: string full(struct item &item, bool &success);
            prefix_dict(dict_name_t &name);
            prefix_dict(struct item &name);
            prefix_dict();
            ~prefix_dict();
            enum dict_type get_type();
            const std :: string &get_name();
            bool is_ready();
            // Functions below modifies the dict, so those functions can be
            // invoked only when the dict is not ready.
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
        public:
            prefix_dicts(bool modifiable);
            ~prefix_dicts();
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
            const std :: string full(struct item &item, bool &success);
            bool exist(dict_name_t &name);
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

