#ifndef HASHES_H
#define HASHES_H
/*
*/

/*appropriate for 64bit size_t, not sure how it will be for 32bit
stolen from: http://www.concentric.net/~Ttwang/tech/inthash.htm
*/
inline size_t distribute_bits(size_t key){
  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

#endif //HASHES_H

