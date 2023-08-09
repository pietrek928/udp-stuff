#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <iterator>
#include <string.h>
#include <wmmintrin.h>
#include <xmmintrin.h>

class SymCrypt {
  virtual std::size_t min_size();
  virtual std::size_t checksum_size();
  virtual std::size_t state_size();
  virtual void encrypt(void *out, const void *in, std::size_t size);
  virtual void decrypt(void *out, const void *in, std::size_t size);
  virtual void get_checksum(void *out);
  //   virtual void load_state(DataLoader &d);
  //   virtual void store_state(DataStorer &d);
};

inline void AES_KEY_256_ASSIST_1(__m128i *temp1, __m128i *temp2) {
  __m128i temp4;
  *temp2 = _mm_shuffle_epi32(*temp2, 0xff);
  temp4 = _mm_slli_si128(*temp1, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  *temp1 = _mm_xor_si128(*temp1, *temp2);
}
inline void AES_KEY_256_ASSIST_2(__m128i *temp1, __m128i *temp3) {
  __m128i temp2, temp4;
  temp4 = _mm_aeskeygenassist_si128(*temp1, 0x0);
  temp2 = _mm_shuffle_epi32(temp4, 0xaa);
  temp4 = _mm_slli_si128(*temp3, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  *temp3 = _mm_xor_si128(*temp3, temp2);
}

inline void AES_KEY_256_LOAD(__m128i *expanded_key, const __m128i *user_key) {
  __m128i temp1, temp2, temp3;

  temp1 = _mm_loadu_si128(user_key);
  temp3 = _mm_loadu_si128(user_key + 1);
  expanded_key[0] = temp1;
  expanded_key[1] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x01);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[2] = temp1;
  AES_KEY_256_ASSIST_2(&temp1, &temp3);
  expanded_key[3] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x02);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[4] = temp1;
  AES_KEY_256_ASSIST_2(&temp1, &temp3);
  expanded_key[5] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x04);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[6] = temp1;
  AES_KEY_256_ASSIST_2(&temp1, &temp3);
  expanded_key[7] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x08);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[8] = temp1;
  AES_KEY_256_ASSIST_2(&temp1, &temp3);
  expanded_key[9] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x10);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[10] = temp1;
  AES_KEY_256_ASSIST_2(&temp1, &temp3);
  expanded_key[11] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x20);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[12] = temp1;
  AES_KEY_256_ASSIST_2(&temp1, &temp3);
  expanded_key[13] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x40);
  AES_KEY_256_ASSIST_1(&temp1, &temp2);
  expanded_key[14] = temp1;
}

inline __m128i AES_ECB_encrypt(__m128i block, const __m128i *expanded_key,
                               int nrounds) {
  auto tmp = block;
  tmp = _mm_xor_si128(tmp, expanded_key[0]);

  int j = 1;
  for (; j < nrounds; j++) {
    tmp = _mm_aesenc_si128(tmp, expanded_key[j]);
  }
  tmp = _mm_aesenclast_si128(tmp, expanded_key[j]);

  return tmp;
}

inline __m128i AES_ECB_decrypt(__m128i block, const __m128i *expanded_key,
                               int nrounds) {
  auto tmp = block;
  tmp = _mm_xor_si128(tmp, expanded_key[0]);

  int j = 1;
  for (; j < nrounds; j++) {
    tmp = _mm_aesdec_si128(tmp, expanded_key[j]);
  }
  tmp = _mm_aesdeclast_si128(tmp, expanded_key[j]);

  return tmp;
}

// TODO: faster code, py generate ?
class AES256SUM128Crypt : SymCrypt {
  constexpr static int block_size = 128 / 8;

  __m128i expanded_key[15];
  __m128i feedback;

  void load_key(const __m128i *user_key) {
    AES_KEY_256_LOAD(expanded_key, user_key);
  }

  void encrypt(void *out, const void *in, std::size_t size) {
    auto feedback_buf = feedback;

    std::size_t tail_size = 0;
    if (size % block_size) {
      tail_size = size % block_size;
      size -= tail_size;
    }

    for (std::size_t i = 0; i < size; i += block_size) {
      auto tmp = _mm_loadu_si128((__m128i *)(((uint8_t *)in) + i));
      tmp = feedback_buf = _mm_add_epi64(feedback_buf, tmp);
      tmp = AES_ECB_encrypt(tmp, expanded_key, 14);
      _mm_storeu_si128((__m128i *)(((uint8_t *)out) + i), tmp);
    }
    if (tail_size) {
      __m128i *b1 =
          (__m128i *)(((uint8_t *)in) + (size + tail_size - block_size));
      __m128i *b2 = (__m128i *)(((uint8_t *)in) + size);

      auto tmp = _mm_setzero_si128();
      memcpy(&tmp, b2, tail_size);
      tmp = feedback_buf = _mm_add_epi64(feedback_buf, tmp);
      memcpy(b2, &tmp, tail_size);

      tmp = _mm_loadu_si128(b1);
      tmp = AES_ECB_encrypt(tmp, expanded_key, 14);
      _mm_storeu_si128(b1, tmp);
    }

    feedback = feedback_buf;
  }

  void decrypt(void *out, const void *in, std::size_t size) {
    auto feedback_buf = feedback;

    std::size_t tail_size = 0;
    if (size % block_size) {
      tail_size = block_size + (size % block_size);
      size -= tail_size;
    }

    for (std::size_t i = 0; i < size; i += block_size) {
      auto tmp = _mm_loadu_si128((__m128i *)(((uint8_t *)in) + i));
      tmp = AES_ECB_decrypt(tmp, expanded_key, 14);
      tmp = feedback_buf = _mm_sub_epi64(tmp, feedback_buf);
      _mm_storeu_si128((__m128i *)(((uint8_t *)out) + i), tmp);
    }
    if (tail_size) {
      __m128i *b1 = (__m128i *)(((uint8_t *)in) + size);
      __m128i *b2 =
          (__m128i *)(((uint8_t *)in) + (size + tail_size - block_size));
      __m128i *b3 = (__m128i *)(((uint8_t *)in) + (size + block_size));

      auto tmp = _mm_loadu_si128(b2);
      tmp = AES_ECB_decrypt(tmp, expanded_key, 14);
      _mm_storeu_si128(b2, tmp);

      tmp = _mm_loadu_si128(b1);
      tmp = AES_ECB_decrypt(tmp, expanded_key, 14);
      tmp = feedback_buf = _mm_sub_epi64(tmp, feedback_buf);
      _mm_storeu_si128(b1, tmp);

      tmp = _mm_setzero_si128();
      memcpy(&tmp, b3, tail_size - block_size);
      tmp = feedback_buf = _mm_sub_epi64(tmp, feedback_buf);
      memcpy(b3, &tmp, tail_size - block_size);
    }

    feedback = feedback_buf;
  }

  void get_checksum(void *out) {
    auto tmp = feedback;
    tmp = AES_ECB_decrypt(tmp, expanded_key, 14);
    _mm_storeu_si128((__m128i *)out, tmp);
  }
};
