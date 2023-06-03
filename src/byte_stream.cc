#include <stdexcept>
#include <cmath>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity) : 
  capacity_(capacity), 
  bytes_() {
}

void Writer::push(string data) {
  for (size_t i = 0; i < data.size() && bytes_.size() < capacity_; ++i) {
    bytes_.push_back(data[i]); 
    bytes_pushed_++; 
  }
}

void Writer::close() {
  closed_ = true; 
}

void Writer::set_error() {
  has_error_ = true; 
}

bool Writer::is_closed() const {
  return closed_; 
}

uint64_t Writer::available_capacity() const {
  return capacity_ - bytes_.size(); 
}

uint64_t Writer::bytes_pushed() const {
  return bytes_pushed_;
}

string Reader::peek() const {
  return string(bytes_.begin(), bytes_.end());
}

bool Reader::is_finished() const {
  if (!closed_) {
    return false; 
  }
  return bytes_.empty(); 
}

bool Reader::has_error() const {
  return has_error_; 
}

void Reader::pop(uint64_t len) {
  for (uint64_t i = 0; i < len && !bytes_.empty(); ++i) {
    bytes_.pop_front();
    ++bytes_popped_; 
  }
}

uint64_t Reader::bytes_buffered() const {
  return bytes_.size(); 
}

uint64_t Reader::bytes_popped() const {
  return bytes_popped_; 
}
