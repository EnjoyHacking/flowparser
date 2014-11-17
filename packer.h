// The packer packs an incremental sequence of unsigned integers (uin64_t) into
// sequence of bytes where each integer is encoded as the difference from the
// from the previous appended value. The difference occupies 1-8 bytes depending
// on how big it is. Smaller differences are stored in less bytes.

#ifndef FPARSER_PACKER_H
#define FPARSER_PACKER_H

#include <vector>
#include "common.h"

namespace flowparser {

// A packed sequence of unsigned integers.
class PackedUintSeq {
 public:
  PackedUintSeq()
      : len_(0),
        last_append_(0) {
  }

  // The amount of memory (in bytes) occupied by the sequence.
  size_t SizeBytes() const {
    return data_.size() * sizeof(char);
  }

  // Appends a value at the end of the sequence. This call can fail if the
  // difference between the last value and this value is too large (larger than
  // kEightByteLimit) or if the new value is smaller than the last appended
  // value (the sequence is not incrementing).
  Status Append(uint64_t value);

  // Copies out the sequence in a standard vector.
  void Restore(std::vector<uint64_t>* vector) const;

 private:
  // The limits on how many bytes can be used to encode an integer.
  static constexpr uint64_t kOneByteLimit = 32;  // 2 ** (8 - 3)
  static constexpr uint64_t kTwoByteLimit = 8192;  // 2 ** (16 - 3)
  static constexpr uint64_t kThreeByteLimit = 2097152;  // 2 ** (24 - 3)
  static constexpr uint64_t kFourByteLimit = 536870912;  // 2 ** (32 - 3)
  static constexpr uint64_t kFiveByteLimit = 137438953472;  // 2 ** (40 - 3)
  static constexpr uint64_t kSixByteLimit = 35184372088832;  // 2 ** (48 - 3)
  static constexpr uint64_t kSevenByteLimit = 9007199254740992;  // 2 ** (56-3)
  static constexpr uint64_t kEightByteLimit = 2305843009213693952;  // 2 ** 61

  // The value of the first 3 bits of the first byte tell us how many bytes are
  // used in encoding the integer.
  static constexpr uint8_t kOneBytePacked = 0x0;
  static constexpr uint8_t kTwoBytesPacked = 0x20;
  static constexpr uint8_t kThreeBytesPacked = 0x40;
  static constexpr uint8_t kFourBytesPacked = 0x60;
  static constexpr uint8_t kFiveBytesPacked = 0x80;
  static constexpr uint8_t kSixBytesPacked = 0xA0;
  static constexpr uint8_t kSevenBytesPacked = 0xC0;
  static constexpr uint8_t kEightBytesPacked = 0xE0;

  static constexpr uint8_t kMask = 0x1F;  // Mask for the first byte

  // Deflates a single integer at a given offset and returns the increment for
  // the next integer.
  size_t DeflateSingleInteger(size_t offset, uint64_t* value) const;

  // The sequence.
  std::vector<uint8_t> data_;

  // Length in terms of number of integers stored.
  size_t len_;

  // The last appended integer.
  uint64_t last_append_;

  DISALLOW_COPY_AND_ASSIGN(PackedUintSeq);
};

// Compresses and decompresses a sequence of elements to a series of sequences
// (X1, X1 + t1, X1 + 2t1, X1 + 3t1 ...), (X2, X2 + t2, X2 + 2t2, X2 + 3t2 ...),
// ... When a new element is inserted it is first checked if it is part of the
// current sub-sequence (stride) and if it is not a new stride is created.
template<typename T>
class RLEField {
 private:
   // A stride is a sequence X, X + t, X + 2t, X + 3t ...
   class Stride {
    private:
     Stride(T value) : value_(value), increment_(0), len_(0) {}

     const T value_;  // The base of the stride (X).
     T increment_;  // The increment (t).
     size_t len_;  // How many elements there are in the sequence.

     friend class RLEField<T>;
   };

 public:
   RLEField() {}

  // Appends a new value to the sequence. "Well-behaved" sequences will occupy
  // less memory and will be faster to add elements.
  void Append(T value) {
    if (strides_.empty()) {
      strides_.push_back(Stride(value));
      return;
    }

    Stride& last_stride = strides_.back();
    if (last_stride.len_ == 0) {
      last_stride.increment_ = value - last_stride.value_;
      last_stride.len_++;
      return;
    }

    T expected = last_stride.value_ +
        (last_stride.len_ + 1) * last_stride.increment_;

    if (value == expected) {
      last_stride.len_++;
      return;
    }

    strides_.push_back(Stride(value));
  }

  // The amount of memory (in terms of bytes) used to store the sequence.
  size_t SizeBytes() const {
    return strides_.size() * sizeof(Stride);
  }

  // Copies out the sequence to a standard vector.
  void Restore(std::vector<T>* vector) const {
    for (const auto& stride : strides_) {
      for (size_t i = 0; i < stride.len_; i++) {
        vector->push_back(stride.value_ + i * stride.increment_);
      }
    }
  }

 private:
  // The entire sequence is stored as a sequence of strides.
  std::vector<Stride> strides_;

  DISALLOW_COPY_AND_ASSIGN(RLEField<T>);
};

}  // namespace flowparser

#endif  /* FPARSER_PACKER_H */
