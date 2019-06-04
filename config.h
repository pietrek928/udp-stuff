#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <string>
#include <iomanip>

#include <arpa/inet.h>
#include <fcntl.h>

class ParseError : public std::exception {
    std::string descr;
public:

    ParseError(const char *msg, size_t ln)
        : descr(std::to_string(ln) + ": " + msg) {}

    ParseError(const std::string descr)
        : descr(descr) {
    }

    const char * what () const throw () {
        return descr.c_str();
    }
};

auto str2ipv4(const char *str) {
    uint32_t r;
    if (inet_pton(AF_INET, str, &r) != 1) {
        throw ParseError(std::string("Converting string '") + str + "' to ipv4 failed");
    }
    return r;
}

template<class Tout>
auto cvt_val(const char *str) {
    Tout r;
    std::istringstream(str) >> r;
    return r;
}

template<>
auto cvt_val<int>(const char *str) {
    return atoi(str);
}

template<>
auto cvt_val<uint32_t>(const char *str) {
    if (!strncmp(str, "0x", 2)) return (uint32_t)strtoul(str+2, NULL, 16);
    if (!strncmp(str, "ipv4:", 5)) return (uint32_t)str2ipv4(str+5);
    return (uint32_t)strtoul(str, NULL, 10);
}

class ImmutableConfig {
    class ParseConfig {
        std::vector<uint8_t> &out_data;

        std::vector<size_t> pos_stack;
        int nstr = 0;
        int line_num = 0;
        enum {
            INSIDE,
            QUOTED_INSIDE,
            OUTSIDE
        } state = OUTSIDE;
        const char *data;

        void throw_error(const char *msg) {
            throw ParseError(msg, line_num);
        }

        auto reserve_len() {
            auto r = out_data.size();
            uint32_t v = 0;
            out_data.insert(out_data.end(), (uint8_t*)&v, (uint8_t*)((&v)+1));
            return r;
        }

        void update_len(size_t len_pos) {
            uint32_t len = out_data.size() - len_pos;
            *(uint32_t*)(& out_data[len_pos]) = len;
        } 

        void push_str(const std::string &s) {
            // include 0 character
            out_data.insert(out_data.end(), & s[0], & s[s.size()+1]);
        }

        void push_char(const uint8_t c) {
            out_data.push_back(c);
        }

        void end_opt() {
            switch(nstr) {
                case 0:
                    break;
                case 1:
                case 2:
                    update_len(pos_stack.back());
                    nstr = 0;
                    break;
                default:
                    throw_error("Too many strings for single option");
            }
        }

        void start_str() {
            if (!nstr) {
                pos_stack.back() = reserve_len();
            }
            if (nstr == 2) {
                nstr--;
                out_data.back() = ' ';
            }
        }

        void end_str() {
            push_char(0);
            state = OUTSIDE;
            nstr ++;
        }

        public:
        ParseConfig(const char *data, std::vector<uint8_t> &out_data) 
            : data(data), out_data(out_data) {
            nstr = 0;
            line_num = 0;
            pos_stack.push_back(-1);

            char c;
            while (c = *(data++)) {
                if (c == '\n' && state != INSIDE) {
                    line_num ++;
                }
                switch(state) {
                    case OUTSIDE:
                        switch (c) {
                            case '{':
                                if (nstr != 1) {
                                    throw_error("invalid string count before '{'");
                                }
                                push_char(0);
                                nstr = 0;
                                pos_stack.push_back(-1);
                                break;
                            case '}':
                                end_opt();
                                if (pos_stack.size() <= 1) {
                                    throw_error("unmatching '}'");
                                }
                                pos_stack.pop_back();
                                nstr = 2;
                                end_opt();
                                break;
                            case ';':
                            case '\n':
                                end_opt();
                                break;
                            case '\t':
                            case ' ':
                            case ':':
                                break;
                            case '"':
                                start_str();
                                state = QUOTED_INSIDE;
                                break;
                            default:
                                start_str();
                                data--;
                                state = INSIDE;
                                break;
                        }
                        break;
                    case INSIDE:
                        switch (c) {
                            case '{':
                            case '}':
                            case ';':
                            case '\n':
                            case '"':
                                data--;
                            case '\t':
                            case ' ':
                                end_str();
                                break;
                            default:
                                push_char(c);
                                break;
                        }
                        break;
                    case QUOTED_INSIDE:
                        switch (c) {
                            case '"':
                                end_str();
                                break;
                            default:
                                push_char(c);
                                break;
                        }
                        break;
                }
            }
            if (state != OUTSIDE) {
                end_str();
            }
            end_opt();

            if (pos_stack.size() > 1) {
                throw_error("Lacking '}'");
            }
        }
    };
 
    std::vector<uint8_t> v;

    auto read_len(size_t &pos) const {
        auto r = *(uint32_t*)&v[pos];
        pos += sizeof(r);
        return r;
    }

    template<class Tp>
    auto find(Tp be, const char *str) const {
        size_t pos = std::get<0>(be);
        size_t pos_e = std::get<1>(be);
        while (pos < pos_e) {
            auto new_pos = pos;
            new_pos += read_len(pos);
            if (!strcmp((char*)&v[pos], str)) {
                return std::make_tuple(pos + strlen(str) + 1, new_pos);
            }
            pos = new_pos;
        }
        return std::make_tuple(pos_e, pos_e);
    }

    template<class Tout>
    inline auto _get(const char *name, size_t fb, size_t fe) const {
        auto fnd = find(std::make_tuple(fb, fe), name);
        auto b = std::get<0>(fnd);
        auto e = std::get<1>(fnd);
        if (b == e || !v[b]) { // not found
            throw std::out_of_range("Value not found");
        }
        return cvt_val<Tout>((const char *)&v[b]);
    }

    template<class Tout>
    auto _get(const char *name, Tout default_, size_t fb, size_t fe) const {
        auto fnd = find(std::make_tuple(fb, fe), name);
        auto b = std::get<0>(fnd);
        auto e = std::get<1>(fnd);
        if (b == e || !v[b]) { // not found
            return default_;
        }
        return cvt_val<Tout>((const char *)&v[b]);
    }

    class SubConfig {
        ImmutableConfig *config;
        size_t b, e;

        public:

        SubConfig(ImmutableConfig *config, size_t b, size_t e)
            : config(config), b(b), e(e) {}

        template<class Tout, class ... Targs>
        auto get(Targs ... args) const {
            return config->_get<Tout>(args..., b, e);
        }
    };


public:

    template<class Tout>
    auto get(const char *name) const {
        return _get<Tout>(name, 0, v.size());
    }

    template<class Tout>
    auto get(const char *name, Tout default_) const {
        return _get<Tout>(name, default_, 0, v.size());
    }

    void parse(const char *str) {
        v.clear();
        ParseConfig(str, v);
    }

    ImmutableConfig(const char *str) {
        parse(str);
    }

    void dump() const {
        size_t pos = 0;
        auto vsize = v.size();
        std::vector<size_t> pos_stack = {v.size()};

        while (pos_stack.size()) {
            if (pos < pos_stack.back()) {
                auto end_pos = pos;
                end_pos += read_len(pos);
                pos_stack.push_back(std::min(end_pos, vsize));
            } else {
                pos_stack.pop_back();
                continue;
            }
            auto pos2 = pos + strlen((char*)& v[pos]) + 1;
            if (pos2 < pos_stack.back()) {
                std::cout << std::string(pos_stack.size(), ' ') << &v[pos] << ": " << &v[pos2] << std::endl;
                pos = pos2 + strlen((char*)& v[pos2]) + 1;
            } else {
                std::cout << std::string(pos_stack.size(), ' ') << &v[pos] << std::endl;
                pos = pos2;
            }
        }
    }

};

typedef ImmutableConfig Config;

#define CONFIGURE(vname) {vname = cfg->get<decltype(vname)>(#vname);}

#endif /* __CONFIG_H_ */

