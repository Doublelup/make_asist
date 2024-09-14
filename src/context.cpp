#include "../include/parser.h"

using namespace reader;

// Call when create subcontext.
Context :: Context(Context &parent)
:   subcontexts{},
    lpd{},
    rpd{true, parent.rpd},
    lnd{},
    rnd{true, parent.rnd},
    input{},
    output{nullptr},
    jobs_guard{},
    cv{},
    jobs{0},
    parent{nullptr}
{
    this->parent = &parent;
    parent.add_sub_context(this);
    output = parent.add_input();
}

// Call when create root context.
Context :: Context(std :: string* output)
:   subcontexts{},
    lpd{},
    rpd{},
    lnd{},
    rnd{},
    input{},
    output{output},
    jobs{0},
    jobs_guard{},
    cv{},
    parent{nullptr}
{
}

Context :: ~Context()
{
    {
        std::unique_lock<std::mutex> lock(jobs_guard);
        cv.wait(lock, [this]{return jobs == 0;});
    }
    for (Context *sub : subcontexts){
        delete sub;
    }
    for (std::string* s : input) delete s;
    if (parent) parent->jobs_dec();
}

void
Context :: add_sub_context(Context *context)
{
    assert(context);
    subcontexts.push_back(context);
    jobs_inc();
    return;
}

std::string *
Context :: add_input()
{
    std :: string *s = new std::string{""};
    input.push_back(s);
    return s;
}

void
Context :: jobs_inc()
{
    std::lock_guard<std::mutex> lock(jobs_guard);
    ++jobs;
    return;
}

void
Context :: jobs_dec()
{
    std::lock_guard<std::mutex> lock(jobs_guard);
    --jobs;
    if (jobs == 0 || jobs == 1) cv.notify_all();
    return;
}

static void sig_err()
{
    Reader::send_error();
}

static bool check_err()
{
    return Reader::check_error();
}

// restrict max string size
constexpr size_t buffer_size = 64;
// Introduce redundancy because sometimes we need to write more
// than one char into buffer in one turn, but we don't want to add more
// check. Promise that, won't write in more than (redundency_size+1)
// in one turn.
constexpr size_t redundancy_size = 2;

// namechar include [a-zA-Z0-9.-_], [{}()$] need special handle
enum class _char_t{
    SPACE, TAB, NEWLINE, COLON, BACKSLASH,
    MATHOPRATOR, NAMECHAR, COMMENT, QUOTATION, OTHER,
};

_char_t
type_interpreter(char c)
{
    _char_t ret;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') 
        || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-'
        || c == '{' || c == '}' || c == '(' || c == ')' || c == '$'
        || c == '/')
        ret = _char_t::NAMECHAR;
    else switch (c)
    {
        case ' ': ret = _char_t::SPACE; break;
        case '\t': ret = _char_t::TAB; break;
        case '\n': ret = _char_t::NEWLINE; break;
        case ':': ret = _char_t::COLON; break;
        case '\\': ret = _char_t::BACKSLASH; break;
        case '+':
        case '_':
        case '=': ret = _char_t::MATHOPRATOR; break;
        case '#': ret = _char_t::COMMENT; break;
        case '\"':
        case '\'': ret = _char_t::QUOTATION; break;
        default: ret = _char_t::OTHER; break;
    }
    return ret;
}

static bool
regex_reader(const char* &p, const char* end, int &len, char *buffer)
{
    enum status{
        AHEADREGEX, REGEX, ESCAPE, ERROR, END, COMMENT, QUOTATION,
    };
    char state = AHEADREGEX;
    char pre = ERROR;
    len = 0;
    bool ret = true;
    char input;
    _char_t type;
    while(1){
        if (len >= buffer_size)state = ERROR;
        if (p == end){
            switch (state){
                case AHEADREGEX:
                case ESCAPE:
                case ERROR:
                case COMMENT:
                case QUOTATION:
                    state = ERROR;
                    break;
                case REGEX:
                    state = END;
                    break;
                case END:
                    break;
                default:
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case AHEADREGEX:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::SPACE:
                    case _char_t::TAB:
                    case _char_t::NEWLINE:
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::COMMENT:
                        pre = state;
                        state = COMMENT;
                        break;
                    case _char_t::QUOTATION:
                        state = QUOTATION;
                        break;
                    default:
                        state = REGEX;
                        buffer[len++] = input;
                        break;
                }
            }
            break;
            case REGEX:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::COLON:
                    case _char_t::SPACE:
                    case _char_t::TAB:
                        --p;
                        state = END;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::NEWLINE:
                    case _char_t::COMMENT:
                    case _char_t::QUOTATION:
                        pre = state;
                        state = ERROR;
                        break;
                    case _char_t::MATHOPRATOR:
                    case _char_t::NAMECHAR:
                    case _char_t::OTHER:
                        buffer[len++] = input;
                        break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case ESCAPE:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (pre){
                    case AHEADREGEX:
                    switch (type){
                        case _char_t::NEWLINE:
                            state = pre;
                            break;
                        default:
                            buffer[len++] = input;
                            state = REGEX;
                            break;
                    }
                    break;
                    case REGEX:
                    switch (type){
                        case _char_t::NEWLINE:
                            state = pre;
                            break;
                        case _char_t::SPACE:
                        case _char_t::TAB:
                        case _char_t::COLON:
                        case _char_t::BACKSLASH:
                        case _char_t::COMMENT:
                        case _char_t::QUOTATION:
                            buffer[len++] = input;
                            state = pre;
                            break;
                        case _char_t::NAMECHAR:
                            buffer[len++] = '\\';
                            buffer[len++] = input;
                            state = pre;
                            break;
                        default:
                            pre = state;
                            state = ERROR;
                            break;
                    }
                    break;
                    case QUOTATION:
                    switch (type){
                        case _char_t::QUOTATION:
                            buffer[len++] = input;
                            state = pre;
                            break;
                        default:
                            buffer[len++] = '\\';
                            buffer[len++] = input;
                            state = pre;
                            break;
                    }
                    break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case COMMENT:
                while(p != end && state == COMMENT){
                    input = *p;
                    ++p;
                    type = type_interpreter(input);
                    switch (type){
                        case _char_t::NEWLINE:
                            state = pre;
                            break;
                        default:
                            break;
                    }
                }
                break;
            case QUOTATION:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                // backslash doesn't mean escape in "", so write regex in a
                // real physical line.
                switch (type){
                    case _char_t::QUOTATION:
                        state = END;
                        break;
                    case _char_t::NEWLINE:
                        state = ERROR;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    default:
                        buffer[len++] = input;
                        break;
                }
            }
            break;
            case ERROR:
                ret = false;
                return ret;
                break;
            case END:
                buffer[len] = '\0';
                ret = true;
                return ret;
                break;
            default:
                pre = state;
                state = ERROR;
                break;
        }
    }
    return ret;
}

static bool
name_reader(const char* &p, const char* end, int &len, char *buffer)
{
    enum status{
        AHEADNAME, NAME, ESCAPE, QUOTATION, END, ERROR,
    };
    char state = AHEADNAME;
    char pre = ERROR;
    len = 0;
    bool ret = true;
    char input;
    _char_t type;
    while(1){
        if (len >= buffer_size) state = ERROR;
        if (p == end){
            switch (state){
                case AHEADNAME:
                case ESCAPE:
                case QUOTATION:
                case ERROR:
                    state = ERROR;
                    break;
                case END:
                case NAME:
                    state = END;
                    break;
                default:
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case AHEADNAME:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::SPACE:
                    case _char_t::TAB:
                    case _char_t::NEWLINE:
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::QUOTATION:
                        state = QUOTATION;
                        break;
                    case _char_t::NAMECHAR:
                        buffer[len++] = input;
                        state = NAME;
                        break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case NAME:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::NAMECHAR:
                        buffer[len++] = input;
                        break;
                    default:
                        pre = state;
                        state = END;
                        --p;
                        break;
                }
            }
            break;
            case ESCAPE:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (pre){
                    case AHEADNAME:
                    switch (type){
                        case _char_t::NEWLINE:
                            state = pre;
                            break;
                        case _char_t::NAMECHAR:
                            pre = state;
                            state = ERROR;
                            break;
                        default:
                            state = NAME;
                            buffer[len++] = '\\';
                            buffer[len++] = input;
                            break;
                    }
                    break;
                    case NAME:
                    switch (type){
                        case _char_t::NEWLINE:
                            state = END;
                            p-=2;
                            break;
                        case _char_t::NAMECHAR:
                            pre = state;
                            state = ERROR;
                            break;
                        default:
                            state = pre;
                            buffer[len++] = '\\';
                            buffer[len++] = input;
                            break;
                    }
                    break;
                    case QUOTATION:
                    switch (type){
                        case _char_t::QUOTATION:
                        buffer[len++] = input;
                        state = pre;
                        break;
                        default:
                            buffer[len++] = '\\';
                            buffer[len++] = '\\';
                            --p;
                            state = pre;
                            break;
                    }
                    break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case QUOTATION:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::QUOTATION:
                        state = END;
                        break;
                    case _char_t::NEWLINE:
                        state = ERROR;
                        break;
                    case _char_t::NAMECHAR:
                        buffer[len++] = input;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    default:
                        buffer[len++] = '\\';
                        buffer[len++] = input;
                        break;
                }
            }
            break;
            case END:
                buffer[len] = '\0';
                ret = true;
                return ret;
                break;
            case ERROR:
                ret = false;
                return ret;
                break;
            default:
                pre = state;
                state = ERROR;
                break;
        }
    }
    return ret;
}
/*
1. Each logical line is a pattern.
2. Syntax to define a pattern: "regex : prefix".
    both regex and prefix CAN'T include \s unless precede a '\' or
    use "" or '' to wrap your pattern.
3. '#' indicates content after that is comment.
4. '\n' is physical line delimiter. If you want to 
    devide a logical line into several physical
    lines, use '\' right before '\n'.
*/
static void 
_defdir(dicts::prefix_dict &dict, const char *start, const char *end, Context *context)
{
    enum status{
        AHEADPATTERN, PATTERNEND, AHEADPREFIX, PREFIXEND, COMMENT, ESCAPE,
        END, ERROR,
    };
    char buffer[buffer_size + redundancy_size];
    int len = 0;
    int state = AHEADPATTERN;
    int pre = ERROR;
    const char *p = start;
    char input;
    _char_t type;
    std::string pattern;
    dicts::item ipt, ipf;
    bool ret;
    if (check_err()) state = END;
    while(1){
        if (p == end){
            switch (state){
                case PATTERNEND:
                case AHEADPREFIX:
                case ESCAPE:
                case ERROR:
                    pre = state;
                    state = ERROR;
                    break;
                case AHEADPATTERN:
                case PREFIXEND:
                case COMMENT:
                case END:
                    state = END;
                    break;
                default:
                    pre = state;
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case AHEADPATTERN:
                ret = regex_reader(p, end, len, buffer);
                if (ret){
                    state = PATTERNEND;
                    pattern = std::string(buffer, len);
                }
                else{
                    if (len == 0){
                        state = END;
                    }
                    else{
                        pre = state;
                        state = ERROR;
                    }
                }
                break;
            case PATTERNEND:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::SPACE:
                    case _char_t::TAB:
                        break;
                    case _char_t::COLON:
                        state = AHEADPREFIX;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state= ESCAPE;
                        break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case AHEADPREFIX:
                ret = name_reader(p, end, len, buffer);
                if (ret){
                    ipt = dicts::item{pattern};
                    ipf = dicts::item{buffer, static_cast<size_t>(len)};
                    dict.add_item(ipt, ipf);
                    state = PREFIXEND;
                }
                else{
                    pre = state;
                    state = ERROR;
                }
                break;
            case PREFIXEND:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::SPACE:
                    case _char_t::TAB:
                        break;
                    case _char_t::NEWLINE:
                        state = AHEADPATTERN;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::COMMENT:
                        pre = state;
                        state = COMMENT;
                        break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case COMMENT:
            {
                while (state == COMMENT && p != end){
                    input = *p;
                    ++p;
                    type = type_interpreter(input);
                    switch (type){
                        case _char_t::NEWLINE:
                            state = AHEADPATTERN;
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
            case ESCAPE:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (pre){
                    case PATTERNEND:
                        switch (type){
                            case _char_t::NEWLINE:
                                state = pre;
                                break;
                            default:
                                pre = state;
                                state = ERROR;
                                break;
                        }
                        break;
                    case PREFIXEND:
                        switch (type){
                            case _char_t::NEWLINE:
                                state = pre;
                                break;
                            default:
                                pre = state;
                                state = ERROR;
                                break;
                        }
                        break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case ERROR:
                sig_err();
            case END:
                dict.finish();
                context->jobs_dec();
                return;
                break;
            default:
                pre = state;
                state = ERROR;
                break;
        }
    }
    return;
}

void
Context :: defdir(int order, std :: string &name, const char *start, const char*end)
{
    dicts::prefix_dict *dict = nullptr;
    if (name.empty()) dict = new dicts::prefix_dict{};
    else dict = new dicts::prefix_dict{name};
    lpd.defdir(dict);
    rpd.defdir(dict);
    jobs_inc();
    pool.enqueue(static_cast<int>(Priority::MAKE_DICT),
        order, _defdir, std::ref(*dict), start, end, this);
    return;
}

/*
1. \s are seen as delimiters.
2. each patten CAN'T contain \s unless precede a '\' or use "" or ''
    to wrap the pattern.
*/
static void
_noextend(dicts::noextend_dict &dict, const char *start, const char *end, Context *context)
{
    enum status{
        AHEADPATTERN, END, ERROR,
    };
    char buffer[buffer_size + redundancy_size];
    int len = 0;
    int state = AHEADPATTERN;
    const char *p = start;
    char input;
    _char_t type;
    dicts::item ipt;
    bool ret;
    if (check_err()) state = END;
    while (1){
        if (p == end){
            switch (state){
                case ERROR:
                    state = ERROR;
                    break;
                case AHEADPATTERN:
                case END:
                    state = END;
                    break;
                default:
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case AHEADPATTERN:
                ret = regex_reader(p, end, len, buffer);
                if (ret){
                    ipt = dicts::item{buffer, static_cast<size_t>(len)};
                    dict.add_item(ipt);
                }
                else{
                    if (len == 0) state = END;
                    else state = ERROR;
                }
                break;
            case ERROR:
                sig_err();
            case END:
                dict.finish();
                context->jobs_dec();
                return;
            default:
                state = ERROR;
                break;
        }
    }
    return;
}

void
Context :: noextend(int order, std :: string &name, const char *start, const char *end)
{
    dicts::noextend_dict *dict = nullptr;
    if (name.empty()) dict = new dicts::noextend_dict{};
    else dict = new dicts::noextend_dict{name};
    lnd.noextend(dict);
    rnd.noextend(dict);
    jobs_inc();
    pool.enqueue(static_cast<int>(Priority::MAKE_DICT),
        order, _noextend, std::ref(*dict), start, end, this);
    return;
}

void
Context :: undefdir(std :: string &name)
{
    if (name.empty())return;
    rpd.undefdir(name);
    return;
}

void
Context :: unnoextend(std :: string &name)
{
    if (name.empty())return;
    rnd.unnoextend(name);
    return;
}

/*
Since make require '\t' before recipe, so here we introduce 'mark' to
indicate left side align.
The first char which is not '\t' indicates mark of rest of text in block.
*/
/*
Figure out amount of '\t' should be ignore.
If the mark is given explicitly, stop p at the line after it.
Other wise, stop p at the first none '\t' char.
If fail to figure out '\t', return -1;
*/
static int
read_mark(const char *&p, const char * const end){
    enum status{
        AHEADMARK, SUSPICIOUS, COMMENT, MARKHANDLE, ESCAPE, END, ERROR,
    };
    char state = AHEADMARK;
    char pre = ERROR;
    int ret = 0;
    char input;
    _char_t type;
    const char *tip = nullptr; // point at the place need handle.
    while(1){
        if (p == end){
            switch (state){
                case AHEADMARK:
                case SUSPICIOUS:
                case END:
                case COMMENT:
                    state = END;
                    break;
                case ESCAPE:
                case ERROR:
                case MARKHANDLE:
                    pre = state;
                    state = ERROR;
                    break;
                default:
                    pre = state;
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case AHEADMARK:
            {
                input = *p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::TAB:
                        ++ret;
                        break;
                    case _char_t::NEWLINE:
                        ret = 0;
                        break;
                    case _char_t::COMMENT:
                        pre = state;
                        state = COMMENT;
                        break;
                    case _char_t::BACKSLASH:
                        tip = p;
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::NAMECHAR:
                        tip = p;
                        if (end - p >= 4 && strncmp(p, "mark", 4) == 0){
                            p += 3;
                            state = MARKHANDLE;
                        }
                        else state = END;
                        break;
                    case _char_t::SPACE:
                        tip = p;
                        state = SUSPICIOUS;
                        break;
                    default:
                        state = END;
                        tip = p;
                        break;
                }
                ++p;
            }
            break;
            case SUSPICIOUS:
            {
                input = *p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::COMMENT:
                        pre = state;
                        state = COMMENT;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    case _char_t::NAMECHAR:
                        if (end - p >= 4 && strncmp(p, "mark", 4) == 0){
                            p += 3;
                            state = MARKHANDLE;
                        }
                        else state = END;
                        break;
                    case _char_t::SPACE:
                    case _char_t::TAB:
                        break;
                    case _char_t::NEWLINE:
                        state = AHEADMARK;
                        break;
                    default:
                        state = END;
                        break;
                }
                ++p;
            }
            break;
            case COMMENT:
            {
                while(state == COMMENT && p != end){
                    input = *p;
                    ++p;
                    type = type_interpreter(input);
                    switch (type){
                        case _char_t::NEWLINE:
                            switch (pre){
                                case AHEADMARK:
                                    ret = 0;
                                    state = pre;
                                    break;
                                case SUSPICIOUS:
                                    ret = 0;
                                    state = AHEADMARK;
                                    break;
                                case MARKHANDLE:
                                    tip = p;
                                    state = END;
                                    break;
                                default:
                                    pre = state;
                                    state = ERROR;
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
            case MARKHANDLE:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::TAB:
                    case _char_t::SPACE:
                        break;
                    case _char_t::NEWLINE:
                        tip = p;
                        state = END;
                        break;
                    case _char_t::COMMENT:
                        pre = state;
                        state = COMMENT;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        break;
                    default:
                        state = END;
                        break;
                }
            }
            break;
            case ESCAPE:
            {
                input = *p;
                ++p;
                type = type_interpreter(input);
                switch (pre){
                    case AHEADMARK:
                    case SUSPICIOUS:
                    case MARKHANDLE:
                        switch (type){
                            case _char_t::NEWLINE:
                                state = pre;
                                break;
                            default:
                                state = END;
                                break;
                        }
                        break;
                    default:
                        pre = state;
                        state = ERROR;
                        break;
                }
            }
            break;
            case END:
                if(tip) p = tip;
                return ret;
                break;
            case ERROR:
                return -1;
                break;
            default:
                pre = state;
                state = ERROR;
                break;
        }
    }
    return -1;
}

static bool
read_tabs(const char* &p, const char * const end, int indent){
    for(int i = 0; i < indent; ++i){
        if (*p != '\t') return false;
        ++p;
    }
    return true;
}

constexpr size_t line_size = 511;
// If indent is not enough, add no indent
static void
_raw(std::string *output, const char *start, const char *end, Context *context)
{
    enum status{
        READ, END, ERROR,
    };
    const char *p = start;
    char state = READ;
    char buffer[line_size+1];
    int ind = 0;
    bool ret;
    int indent = read_mark(p, end);
    if (indent == -1) state = END;
    if (check_err()) state = END;
    while(1){
        if (p == end){
            switch (state){
                case READ:
                case END:
                    state = END;
                    break;
                case ERROR:
                    break;
                default:
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case READ:
                ret = read_tabs(p, end, indent);
                for (ind = 0; ind < line_size && p != end 
                        && *p != '\n'; ++ind, ++p){
                    buffer[ind] = *p;
                }
                if (ind < line_size){
                    if (ret == false && ind == 0);
                    else{
                        buffer[ind++] = '\n';
                        buffer[ind] = '\0';
                        *output += buffer;
                    }
                }
                else{
                    state = ERROR;
                    break;
                }
                if (*p == '\n') ++p;
                if (p == end)state = END;
                break;
            case ERROR:
                sig_err();
                goto OUTGATE;
            case END:
                *output += '\n';
OUTGATE:
                context->jobs_dec();
                return;
                break;
            default:
                state = ERROR;
                break;

        }
    }
    return;
}

void
Context :: raw(int order, const char *start, const char *end)
{
    std :: string *output = add_input();
    jobs_inc();
    pool.enqueue(static_cast<int>(Priority::USE_DICT),
        order, _raw, output, start, end, this);
    return;
}

static void
_make(std::string *output, dicts::ref_prefix_dicts *rpd, dicts::ref_noextend_dicts *rnd,
        const char *start, const char *end, Context *context)
{
    enum status{
        BOL, WORD, OTHER, ESCAPE, COMMENT, END, ERROR,
    };
    char line_buf[line_size+1];
    constexpr size_t word_size = buffer_size + redundancy_size;
    char word_buf[word_size];
    int line_ind = 0;
    int word_ind = 0;
    line_buf[0] = '\0';
    word_buf[0] = '\0';
    const char *p = start;
    char input;
    char state = BOL;
    char pre = ERROR;
    _char_t type;
    const std::string *fullword;
    bool success;
    dicts::item bareword;
    bool ret;
    int indent = read_mark(p, end);
    if (indent == -1) state = END;
    if (check_err()) state = END;
    auto writeinline = [&line_buf, &line_ind, &output](char input){
        assert(!line_buf[line_ind]);
        assert(line_ind <= line_size);
        if (line_ind == line_size){
            *output += line_buf;
            line_ind = 0;
            line_buf[line_ind] = '\0';
        }
        line_buf[line_ind++] = input;
        line_buf[line_ind] = 0;
    };
    while (1){
        if (p == end){
            switch (state){
                case BOL:
                case OTHER:
                case COMMENT:
                case END:
                    state = END;
                    break;
                case WORD:
                case ESCAPE:
                case ERROR:
                    state = ERROR;
                    break;
                default:
                    state = ERROR;
                    break;
            }
        }
        switch (state){
            case BOL:
                read_tabs(p, end, indent);
                if (p == end){
                    state = END;
                    break;
                }
                input = *p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::NEWLINE:
                        ++p;
                        break;
                    case _char_t::NAMECHAR:
                        state = WORD;
                        break;
                    case _char_t::BACKSLASH:
                        pre = state;
                        state = ESCAPE;
                        ++p;
                        break;
                    case _char_t::COMMENT:
                        state = COMMENT;
                        ++p;
                        break;
                    case _char_t::QUOTATION:
                        state = WORD;
                        break;
                    default:
                        state = OTHER;
                        break;
                }
                break;
            case WORD:
                ret = name_reader(p, end, word_ind, word_buf);
                if (ret){
                    bareword = dicts::item{word_buf, static_cast<size_t>(word_ind)};
                    success = rnd->match(bareword);
                    if (!success){
                        fullword = rpd->full(bareword, success);
                        if (fullword->length() >= line_size){
                            *output += *fullword;
                        }
                        else if (line_size - line_ind - fullword->length() >= 0){
                            assert(!line_buf[line_ind]);
                            strncat(line_buf, fullword->c_str(), line_size);
                            line_ind += fullword->length();
                            assert(!line_buf[line_ind]);
                        }
                        else{
                            assert(!line_buf[line_ind]);
                            *output += line_buf;
                            strncpy(line_buf, fullword->c_str(), line_size);
                            line_ind = fullword->length();
                        }
                        delete fullword;
                        fullword = nullptr;
                    }
                    else{
                        if (word_ind >= line_size){
                            *output += word_buf;
                        }
                        else if (line_size - line_ind - word_ind >= 0){
                            assert (!line_buf[line_ind] && !word_buf[word_ind]);
                            strncat(line_buf, word_buf, line_size);
                            line_ind += word_ind;
                            assert (line_buf[line_ind] == '\0');
                        }
                        else{
                            assert(!line_buf[line_ind]);
                            *output += line_buf;
                            strncpy(line_buf, word_buf, line_size);
                            line_ind = word_ind;
                        }
                    }
                }
                if (p == end){
                    writeinline('\n');
                    state = END;
                }
                else{
                    input = *p;
                    type = type_interpreter(input);
                    switch (type){
                        case _char_t::NEWLINE:
                            state = BOL;
                            ++p;
                            break;
                        case _char_t::BACKSLASH:
                            pre = state;
                            state = ESCAPE;
                            ++p;
                            break;
                        case _char_t::NAMECHAR:
                            break;
                        case _char_t::COMMENT:
                            pre = state;
                            state = COMMENT;
                            ++p;
                            break;
                        case _char_t::QUOTATION:
                            break;
                        default:
                            state = OTHER;
                            break;
                    }
                }
                break;
            case OTHER:
                writeinline(*p);
                ++p;
                if (p == end) state = END;
                else{
                    input = *p;
                    type = type_interpreter(input);
                    switch (type){
                        case _char_t::NEWLINE:
                            state = BOL;
                            ++p;
                            break;
                        case _char_t::BACKSLASH:
                            pre = state;
                            state = ESCAPE;
                            ++p;
                            break;
                        case _char_t::NAMECHAR:
                        case _char_t::QUOTATION:
                            state = WORD;
                            break;
                        case _char_t::COMMENT:
                            pre = state;
                            state = COMMENT;
                            ++p;
                            break;
                        default:
                            break;
                    }
                }
                break;
            case ESCAPE:
            {
                input = *p;
                type = type_interpreter(input);
                switch (type){
                    case _char_t::NEWLINE:
                    {
                        writeinline(' ');
                        while(state == ESCAPE && ++p != end){
                            input = *p;
                            type = type_interpreter(input);
                            switch (type){
                                case _char_t::TAB:
                                case _char_t::SPACE:
                                    break;
                                case _char_t::NEWLINE:
                                    state = BOL;
                                    ++p;
                                    break;
                                case _char_t::BACKSLASH:
                                    state = ESCAPE;
                                    ++p;
                                    break;
                                case _char_t::NAMECHAR:
                                case _char_t::QUOTATION:
                                    state = WORD;
                                    break;
                                case _char_t::COMMENT:
                                    state = COMMENT;
                                    ++p;
                                    break;
                                default:
                                    state = OTHER;
                                    break;
                            }
                            if (type == _char_t::BACKSLASH)break;
                        }
                    }
                    break;
                    case _char_t::NAMECHAR:
                        pre = state;
                        state = ERROR;
                        break;
                    default:
                        --p;
                        state = WORD;
                        break;
                }
            }
            case COMMENT:
            {
                while (state == COMMENT && p != end){
                    input = *p;
                    ++p;
                    type = type_interpreter(input);
                    switch (type){
                        case _char_t::NEWLINE:
                            if (pre != BOL)writeinline('\n');
                            state = BOL;
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
            case ERROR:
                sig_err();
                goto OUTGATE;
            case END:
                writeinline('\n');
                assert(!line_buf[line_ind]);
                *output += line_buf;
OUTGATE:
                context->jobs_dec();
                return;
                break;
            default:
                pre = state;
                state = ERROR;
                break;
        }
    }
    return;
}
        
void
Context :: make(int order, const char *start, const char *end)
{
    std :: string *output = add_input();
    jobs_inc();
    dicts::ref_prefix_dicts *rpd_copy = new dicts::ref_prefix_dicts{rpd};
    dicts::ref_noextend_dicts *rnd_copy = new dicts::ref_noextend_dicts{rnd};
    pool.enqueue(static_cast<int>(Priority::USE_DICT),
        order, _make, output, rpd_copy, rnd_copy, start, end, this);
    return;
}

void
Context :: _end(Context *context){
    assert(context);
    {
        std::unique_lock<std::mutex> lock(context->jobs_guard);
        context->cv.wait(lock, [context]{return context->jobs == 1;});
        for (std::string *s : context->input){
            *context->output += *s;
        }
    }
    context->jobs_dec();
    return;
}

void
Context :: end(int order){
    jobs_inc();
    pool.enqueue(static_cast<int>(Priority::FINISH), order, [this]{Context::_end(this);});
    return;
}

#ifdef TEST

TestSuit test = {
    regex_reader,
    name_reader,
    read_mark,
    read_tabs,
    _defdir,
    _noextend,
    _raw,
    _make,
};

#endif




