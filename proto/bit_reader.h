#include <cstddef>
#include <exception>


class BitStreamFinished : public std::exception {};

template<class Tn, bool free_buf>
class BitBufferReader {
    const Tn *buf = NULL;
    size_t items_read = 0;
    size_t bit_size = 0;

    Tn cur_item = 0;
    int cur_item_bits = 0;

    inline int elem_bit_size() {
        return sizeof(Tn) * 8;
    }

    template<class To>
    To read_bits(int n) {
        To r = 0;
        while (n > 0) {
            if (n <= cur_item_bits) {
                r = (r << n) | (cur_item & ((Tn(1) << (n+1)) - 1));
                cur_item >>= n;
                cur_item_bits -= n;
                return r;
            } else {
                r = (r << cur_item_bits) | cur_item;
                n -= cur_item_bits;

                auto bits_read = items_read * elem_bit_size();
                if (bits_read < bit_size) {
                    cur_item = buf[items_read++];
                    if (bits_read + elem_bit_size() <= bit_size) {
                        cur_item_bits = elem_bit_size();
                    } else {
                        cur_item_bits = bit_size - bits_read;
                    }
                } else {
                    throw BitStreamFinished();
                }
            }
        }
        return r;
    }

    public:

    BitBufferReader(const Tn *buf, size_t bit_size)
        : buf(buf), bit_size(bit_size) {
            if (bit_size > 0) {
                cur_item = buf[items_read++];
                cur_item_bits = bit_size > elem_bit_size() ? elem_bit_size() : bit_size;
            }
        }

    BitBufferReader(const Tn *buf, size_t bit_size_, int bit_shift)
        : buf(buf), bit_size(bit_size_ + bit_shift) {
            if (bit_size > 0) {
                cur_item = buf[items_read++] >> bit_shift;
                cur_item_bits = (bit_size > elem_bit_size() ? elem_bit_size() : bit_size) - bit_shift;
            }
        }

    template<class To>
    void read_uint(int bit_length, To *v) {
        *v = read_bits<To>(bit_length);
    }

    size_t get_bit_pos() {
        return items_read * elem_bit_size() + cur_item_bits;
    }

    void read_fragment(size_t bit_size) {
        return BitBufferReader(buf + items_read, bit_size, cur_item_bits);
    }

    ~BitBufferReader() {
        if (free_buf && buf != NULL) free(buf);
    }
};
