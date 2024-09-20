#include <vector>


template<class Tn>
class BitBufferWriter {
    std::vector<Tn> out;
    Tn out_v = 0;
    int bits_out = 0;

    inline int elem_bit_size() {
        return sizeof(Tn) * 8;
    }

    public:

    std::size_t get_bit_pos() {
        return out.size() * elem_bit_size() + bits_out;
    }

    template<class Ti>
    void write_uint(Ti v, int n) {
        while (n > 0) {
            if (n <= elem_bit_size() - bits_out) {
                out_v = (out_v << n) | v;
                bits_out -= n;
                return;
            } else {
                int b = elem_bit_size() - bits_out;
                out_v = (out_v << b) | (v & ((Ti(1) << (b+1)) - 1));
                v >>= b;
                n -= b;

                out.push_back(out_v);
                bits_out = 0;
            }
        }
    }

    template<class Ti>
    void write_uint_at(std::size_t bit_pos, Ti v, int n) {
        std::size_t item_pos = bit_pos / elem_bit_size();
        int bits = bit_pos % elem_bit_size();
        while (n > 0) {
            //
        }
    }

    std::vector<Tn> &&pick_data() {
        if (bits_out) {
            out.push_back(out_v);
            bits_out = 0;
        }
        return std::move(out);
    }
};
