#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <string>

class ParseError : public std::exception {
    std::string descr;
public:

    ParseError(const char *msg, size_t ln)
        : descr(std::to_string(ln) + ": " + msg) {}

    const char * what () const throw () {
        return descr.c_str();
    }
};

class ImmutableConfig {
    class ParseConfig {
        std::vector<uint8_t> &out_data;

        vector<size_t> pos_stack;
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
            pos_stack.push_back(-1);

            char c;
            while (c = *(data++)) {
                cout << c << " " << pos_stack.size() << endl;
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

    /*template<class Tl>
    void _get(const Tl &names, size_t pos) {
        pos_e = get_len(pos);
        while (pos < pos_e) {
            //
        }
    }*/

    auto read_len(size_t &pos) {
        auto r = *(uint32_t*)&v[pos];
        pos += sizeof(r);
        return r;
    }

    public:

    void parse(const char *str) {
        v.clear();
        ParseConfig(str, v);
    }

    void dump() {
        size_t pos = 0;
        vector<size_t> pos_stack = {v.size()};
        cout << v.size() << endl;

        while (pos_stack.size()) {
            if (pos < pos_stack.back()) {
                auto max_len = v.size() - pos;
                auto end_pos = pos;
                end_pos += read_len(pos);
                pos_stack.push_back(min(end_pos, max_len));
            } else {
                pos_stack.pop_back();
                continue;
            }
            auto pos2 = pos + strlen((char*)& v[pos]) + 1;
            if (pos2 < pos_stack.back()) {
                cout << std::string(pos_stack.size(), ' ') << &v[pos] << ": " << &v[pos2] << endl;
                pos = pos2 + strlen((char*)& v[pos2]) + 1;
            } else {
                cout << std::string(pos_stack.size(), ' ') << &v[pos] << endl;
                pos = pos2;
            }
        }
    }

public:
};

#endif /* __CONFIG_H_ */

