#include "reassembler.hh"
#include <set>
#include <cassert>

using namespace std;

void Reassembler::insert(uint64_t first_index, string data,
                         bool is_last_substring, Writer &output) {
  if (is_last_substring) {
    last_index_ = first_index + data.size();
  }
  auto pair = string_splitter(first_index, data, output.available_capacity());
  first_index = pair.first;
  std::string cut_data = pair.second;
  do {
    if (cut_data.empty()) {
      break;
    }

    if (segments_map_.find(first_index) != segments_map_.end() &&
        segments_map_[first_index].size() >= cut_data.size()) {
      break;
    }

    if (segments_map_.find(first_index) != segments_map_.end()) {
      bytes_pending_ -= segments_map_[first_index].size(); 
    }
    segments_map_[first_index] = cut_data;

    bytes_pending_ += cut_data.size();
    merge_segments(first_index, cut_data.size());
  } while (false);

  checkout_write(output);
}

std::pair<uint64_t, std::string>
Reassembler::string_splitter(uint64_t first_index, const std::string &data,
                             uint64_t available_capacity) {
  available_capacity = min(last_index_ - first_index_, available_capacity);
  if (first_index + data.size() <= first_index_ ||
      first_index >= first_index_ + available_capacity) {
    return {first_index_, ""};
  }
  string f_data;
  if (first_index < first_index_) {
    if (first_index + data.size() > first_index_ + available_capacity) {
      f_data = string{data.begin() + first_index_ - first_index,
                      data.begin() + first_index_ - first_index +
                          available_capacity};
    } else {
      f_data = string{data.begin() + first_index_ - first_index, data.end()};
    }
  } else {
    if (first_index + data.size() > first_index_ + available_capacity) {
      f_data = string{data.begin(), data.begin() + first_index_ - first_index +
                                        available_capacity};
    } else {
      f_data = string{data.begin(), data.end()};
    }
  }
  return {std::max(first_index, first_index_), f_data};
}

Reassembler::segment_relation
Reassembler::get_segment_relation(uint64_t left_index, uint64_t left_len,
                                  uint64_t right_index, uint64_t right_len) {
  if (left_index + left_len < right_index) {
    return Reassembler::noncoincidence;
  } else if (left_index + left_len >= right_index &&
             left_index + left_len <= right_index + right_len) {
    return Reassembler::intersect;
  } else {
    return Reassembler::contain;
  }
}

std::string Reassembler::merge_segment(uint64_t left_index,
                                       uint64_t right_index) {
  std::string &left_segment = segments_map_[left_index];
  std::string &right_segment = segments_map_[right_index];
  return left_segment +
         string{right_segment.begin() +
                    (left_index + left_segment.size() - right_index),
                right_segment.end()};
}

void Reassembler::merge_segments(uint64_t index, uint64_t segment_len) {
  auto iter = segments_map_.find(index);
  assert(iter != segments_map_.end()); 
  if (iter != segments_map_.begin()) {
    --iter; 
  }
  std::vector<uint64_t> possible_keys; 
  for (; iter != segments_map_.end() && iter->first <= index + segment_len; ++iter) {
    if (iter->first == index) {
      continue; 
    }
    possible_keys.push_back(iter->first); 
  }
  for (uint64_t cur_index : possible_keys) {
    uint64_t left_index = 0, left_len = 0, right_index = 0, right_len = 0;    
    if (index < cur_index) {
      left_index = index; 
      left_len = segment_len; 
      right_index = cur_index;
      right_len = segments_map_[cur_index].size();
    } else {
      right_index = index; 
      right_len = segment_len; 
      left_index = cur_index;
      left_len = segments_map_[cur_index].size();
    }
    Reassembler::segment_relation relation = get_segment_relation(left_index,
                                                                  left_len,
                                                                  right_index,
                                                                  right_len);
    if (relation == Reassembler::contain) {
      bytes_pending_ -= segments_map_[right_index].size();
      segments_map_.erase(right_index); 

      index = left_index; 
      segment_len = segments_map_[index].size(); 
    } else if (relation == Reassembler::intersect) {
      std::string merged_seg = merge_segment(left_index, right_index); 

      bytes_pending_ -= segments_map_[left_index].size();
      segments_map_.erase(left_index); 

      bytes_pending_ -= segments_map_[right_index].size();
      segments_map_.erase(right_index); 

      segments_map_[left_index] = merged_seg; 
      bytes_pending_ += segments_map_[left_index].size();

      index = left_index; 
      segment_len = segments_map_[index].size(); 
    }   
  }
}

void Reassembler::checkout_write(Writer &output) {
  if (segments_map_.find(first_index_) != segments_map_.end()) {
    string segment = segments_map_[first_index_];
    uint64_t bytes_write = min(output.available_capacity(), segment.size());
    if (bytes_write == 0) {
      return;
    }
    segments_map_.erase(first_index_);
    if (bytes_write == segment.size()) {
      output.push(segment);
    } else {
      output.push(string(segment.begin(), segment.begin() + bytes_write));
    }
    first_index_ += bytes_write;
    bytes_pending_ -= bytes_write;
    if (bytes_write != segment.size()) {
      string cut_segment{segment.begin() + bytes_write, segment.end()};
      segments_map_[first_index_] = cut_segment;
    }
  }
  if (first_index_ == last_index_) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const { return bytes_pending_; }
