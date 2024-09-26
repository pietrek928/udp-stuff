#include <cstddef>
#include <vector>


template<class Tn>
class BitBufferWriter {
    std::vector<Tn> out;
    Tn out_v = 0;
    int bits_out = 0;

    inline int elem_bit_size() {
        return sizeof(Tn) * 8;
    }

    inline void put(Tn v) {
        out.push_back(v);
    }

    inline void set(std::size_t pos, Tn v) {
        out[pos] = v;
    }

    inline Tn get(std::size_t pos) {
        return out[pos];
    }

    public:

    std::size_t get_bit_pos() {
        return out.size() * elem_bit_size() + bits_out;
    }

    template<class Ti>
    void write_uint(Ti v, int n) {
        if (n <= elem_bit_size() - bits_out) {
            out_v |= v << bits_out;
            bits_out += n;
            return;
        }
        int b = elem_bit_size() - bits_out;
        out_v |= v << bits_out;
        v >>= b;
        n -= b;
        put(out_v);
        bits_out = 0;
        out_v = 0;
        while (n > elem_bit_size()) {
            put(v);
            v >>= elem_bit_size();
            n -= elem_bit_size();
        }
        out_v = v;
        bits_out = n;
    }

    template<class Ti>
    void write_uint_at(std::size_t bit_pos, Ti v, int n) {
        std::size_t item_pos = bit_pos / elem_bit_size();
        int bits = bit_pos % elem_bit_size();
        if (n < elem_bit_size() - bits) {
            set(item_pos, get(item_pos) & ~(((Tn(1) << n) - 1)) << bits | (v << bits));
            return;
        }
        int b = elem_bit_size() - bits;
        set(item_pos++, get(item_pos) & ~(Tn(-1) << bits) | (v << bits));
        v >>= b;
        n -= b;
        while (n > elem_bit_size()) {
            set(item_pos++, v);
            v >>= elem_bit_size();
            n -= elem_bit_size();
        }
        set(item_pos, get(item_pos) & ~((Tn(1) << n) - 1) | v);
    }

    std::vector<Tn> &&pick_data() {
        if (bits_out) {
            out.push_back(out_v);
            bits_out = 0;
        }
        return std::move(out);
    }
};
