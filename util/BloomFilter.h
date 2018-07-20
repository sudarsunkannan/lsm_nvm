#ifndef BLOOMFILTER_H_
#define BLOOMFILTER_H_

#include <stdlib.h>
#include <vector>
#include <stdint.h>

//13MB Bloom filter
#define BLOOMSIZE 13631488
#define BLOOMHASH 13


class BloomFilter {

public:
  BloomFilter(uint64_t size, uint8_t numHashes);
  BloomFilter();
  void add(const uint8_t *data, size_t len);
  bool possiblyContains(const uint8_t *data, size_t len) const;

private:
  uint8_t m_numHashes;
  std::vector<bool> m_bits;
};

#endif
