#pragma once

#include "byte_stream.hh"

#include <string>
#include <map> 

class Reassembler {
public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly
   * out-of-order and possibly overlapping) back into the original ByteStream.
   * As soon as the Reassembler learns the next byte in the stream, it should
   * write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's
   * available capacity but can't yet be written (because earlier bytes remain
   * unknown), it should store them internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's
   * available capacity (i.e., bytes that couldn't be written even if earlier
   * gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert(uint64_t first_index, std::string data, bool is_last_substring,
              Writer &output);

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

private:
  enum segment_relation {
    intersect, 
    noncoincidence, 
    contain,
  }; 

  std::pair<uint64_t, std::string> string_splitter(uint64_t first_index, 
                                                   const std::string& data, 
                                                   uint64_t available_capacity); 
  segment_relation get_segment_relation(uint64_t left_index, uint64_t left_len, 
                                        uint64_t right_index, uint64_t right_len); 
  void merge_segments(uint64_t index, uint64_t segment_len); 
  std::string merge_segment(uint64_t left_index, uint64_t right_index); 
  void checkout_write(Writer &output); 

  std::map<uint64_t, std::string> segments_map_; 
  uint64_t first_index_ = 0; 
  uint64_t bytes_pending_ = 0; 
  uint64_t last_index_ = UINT64_MAX; 
};
