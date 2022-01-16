#include <stdint.h>
#include <stdio.h>

static int32_t encode_sample(int32_t sample, unsigned shift, unsigned filter,
                             int32_t prev1, int32_t prev2) {
  prev2 >>= 1;
  sample = sample >> 1;

  switch (filter) {
  default:
    break;
  case 1:
    sample -= (-prev1) >> 5;
    sample -= prev1 >> 1;
    break;
  case 2:
    sample -= (prev1 * -3) >> 6;
    sample -= prev2 >> 4;
    sample += prev2;
    sample -= prev1;
    break;
  case 3:
    sample -= (prev2 * 3) >> 4;
    sample -= (prev1 * -13) >> 7;
    sample += prev2;
    sample -= prev1;
    break;
  }

  if (shift >= 13)
    sample = (sample >> 11) << 25;
  sample = (sample << 1) >> shift;

  if (sample >= 0)
    sample &= 0x7;
  else
    sample |= ~0x7;

  return sample;
}

static int32_t decode_sample(int32_t sample, unsigned shift, unsigned filter,
                             int32_t prev1, int32_t prev2) {
  prev2 >>= 1;
  sample = (sample << shift) >> 1;
  if (shift >= 13)
    sample = (sample >> 25) << 11;

  switch (filter) {
  default:
    break;
  case 1:
    sample += prev1 >> 1;
    sample += (-prev1) >> 5;
    break;
  case 2:
    sample += prev1;
    sample -= prev2;
    sample += prev2 >> 4;
    sample += (prev1 * -3) >> 6;
    break;
  case 3:
    sample += prev1;
    sample -= prev2;
    sample += (prev1 * -13) >> 7;
    sample += (prev2 * 3) >> 4;
    break;
  }

  if (sample >= 0)
    sample &= 0x7fff;
  else
    sample |= ~0x7fff;

  return (int16_t)(sample << 1);
}

static void brute_force_block_encode(const int16_t in[16], uint8_t out[9],
                                     int16_t wout[16], int32_t *prev1,
                                     int32_t *prev2, int8_t first) {
  int32_t min_err = INT32_MAX;
  unsigned bestF = 0, bestR = 0;
  for (unsigned f = 0, ef = first ? 1 : 4; f < ef; ++f) {
    for (unsigned r = 0; r < 16; ++r) {
      int32_t cum_err = 0;
      int32_t tprev1 = *prev1, tprev2 = *prev2;
      for (unsigned s = 0; s < 16; ++s) {
        // printf("%d", in[s]);
        // fflush(stdout);
        int32_t enc = encode_sample(in[s], r, f, tprev1, tprev2);
        // printf(" -> %d", enc);
        // fflush(stdout);
        int32_t result = decode_sample(enc, r, f, tprev1, tprev2);
        tprev2 = tprev1;
        tprev1 = result;
        // printf(" -> %d\n", result);
        int32_t err = result - in[s];
        cum_err += err < 0 ? -err : err;
      }
      // printf("f: %u r: %u err: %d\n", f, r, cum_err);
      if (cum_err < min_err) {
        // printf("new best\n");
        min_err = cum_err;
        bestF = f;
        bestR = r;
      }
    }
  }

  //printf("bestF: %u bestR: %u err: %d\n", bestF, bestR, min_err);
  out[0] = bestR << 4 | bestF << 2;

  int32_t tprev1 = *prev1, tprev2 = *prev2;
  for (unsigned s = 0; s < 16; ++s) {
    //printf("%d", in[s]);
    //fflush(stdout);
    int32_t enc = encode_sample(in[s], bestR, bestF, tprev1, tprev2);
    //printf(" -> %d", enc);
    //fflush(stdout);
    int32_t result = decode_sample(enc, bestR, bestF, tprev1, tprev2);
    //printf(" -> %d\n", result);
    tprev2 = tprev1;
    tprev1 = result;
    out[s / 2 + 1] |= (s & 1) ? enc & 0xf : enc << 4;
    wout[s] = (int16_t)result;
  }
  *prev1 = tprev1;
  *prev2 = tprev2;
}

static void encode_brr(FILE *fp, FILE *ofp, FILE *owfp, uint32_t num_samples,
                       int8_t loop) {
  int32_t prev1 = 0, prev2 = 0;
  int8_t first = 1;
  while (num_samples) {
    int16_t samples[16] = {};
    uint32_t num_read = num_samples > 16 ? 16 : num_samples;
    fread(samples, 2, num_read, fp);
    num_samples -= num_read;

    uint8_t encoded[9] = {};
    int16_t enc_samples[16] = {};
    brute_force_block_encode(samples, encoded, enc_samples, &prev1, &prev2,
                             first);
    first = 0;

    if (loop)
      encoded[0] |= 2;
    if (num_samples == 0)
      encoded[0] |= 1;

    fwrite(encoded, 1, 9, ofp);
    if (owfp)
      fwrite(enc_samples, 2, num_read, owfp);
  }
}

typedef struct {
  uint16_t audio_fmt;
  uint16_t num_chan;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
} fmt_t;

int main(int argc, const char **argv) {
  if (argc < 3) {
    fprintf(stderr, "brr-enc <in-wav> <out-brr> [<out-wav>]\n");
    return 1;
  }

  FILE *fp = fopen(argv[1], "rb");

  uint32_t chunk_id;
  if (fread(&chunk_id, 1, 4, fp) != 4 || chunk_id != 'FFIR') {
    fprintf(stderr, "Bad RIFF\n");
    return 1;
  }

  int32_t riff_size;
  if (fread(&riff_size, 1, 4, fp) != 4) {
    fprintf(stderr, "Bad size\n");
    return 1;
  }

  uint32_t fmt;
  if (fread(&fmt, 1, 4, fp) != 4 || fmt != 'EVAW') {
    fprintf(stderr, "Bad WAVE\n");
    return 1;
  }
  riff_size -= 4;

  int8_t seen_fmt = 0;
  while (1) {
    if (fread(&chunk_id, 1, 4, fp) != 4) {
      fprintf(stderr, "Bad chunk id\n");
      return 1;
    }

    int32_t chunk_size;
    if (fread(&chunk_size, 1, 4, fp) != 4) {
      fprintf(stderr, "Bad chunk size\n");
      return 1;
    }

    if (chunk_id == ' tmf') {
      fmt_t audio_fmt;
      if (fread(&audio_fmt, 1, sizeof(audio_fmt), fp) != sizeof(audio_fmt)) {
        fprintf(stderr, "Bad audio fmt\n");
        return 1;
      }

      if (audio_fmt.audio_fmt != 1) {
        fprintf(stderr, "must be PCM\n");
        return 1;
      }

      if (audio_fmt.num_chan != 1) {
        fprintf(stderr, "must be mono\n");
        return 1;
      }

      if (audio_fmt.bits_per_sample != 16) {
        fprintf(stderr, "must be 16 bit\n");
        return 1;
      }

      chunk_size -= sizeof(audio_fmt);
      seen_fmt = 1;
    } else if (chunk_id == 'atad') {
      if (seen_fmt == 0) {
        fprintf(stderr, "fmt not seen\n");
        return 1;
      }
      FILE *ofp = fopen(argv[2], "wb");
      FILE *owfp = NULL;
      uint8_t cpy_buf[32];
      if (argc >= 4) {
        owfp = fopen(argv[3], "wb");
        unsigned header_len = ftell(fp);
        unsigned rem_len = header_len;
        fseek(fp, 0, SEEK_SET);
        while (rem_len) {
          unsigned read_len = rem_len > 32 ? 32 : rem_len;
          fread(cpy_buf, 1, read_len, fp);
          fwrite(cpy_buf, 1, read_len, owfp);
          rem_len -= read_len;
        }
      }
      encode_brr(fp, ofp, owfp, chunk_size / 2, 0);
      fclose(ofp);
      if (owfp) {
        unsigned read_len;
        while ((read_len = fread(cpy_buf, 1, 32, fp)))
          fwrite(cpy_buf, 1, read_len, owfp);
        fclose(owfp);
      }
      return 0;
    }

    fseek(fp, chunk_size, SEEK_CUR);
  }

  return 0;
}
