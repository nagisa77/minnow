#include <cmath>
#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity) : capacity_(capacity), bytes_() {}

void Writer::push(string data) {
  int64_t bytes_pushed =
      std::min(static_cast<int64_t>(data.size()),
               static_cast<int64_t>(capacity_ - bytes_.size()));
  bytes_pushed_ += bytes_pushed;
  bytes_.insert(bytes_.end(), data.begin(), data.begin() + bytes_pushed);
}

void Writer::close() { closed_ = true; }

void Writer::set_error() { has_error_ = true; }

bool Writer::is_closed() const { return closed_; }

uint64_t Writer::available_capacity() const {
  return capacity_ - bytes_.size();
}

uint64_t Writer::bytes_pushed() const { return bytes_pushed_; }

string Reader::peek() const { return string(bytes_.begin(), bytes_.end()); }

bool Reader::is_finished() const {
  if(!closed_) {
    return false;
  }
  return bytes_.empty();
}

bool Reader::has_error() const { return has_error_; }

void Reader::pop(uint64_t len) {
  uint64_t bytes_popped = std::min(len, bytes_.size());
  bytes_.erase(bytes_.begin(),
               bytes_.begin() + static_cast<int64_t>(bytes_popped));
  bytes_popped_ += bytes_popped;
}

uint64_t Reader::bytes_buffered() const { return bytes_.size(); }

uint64_t Reader::bytes_popped() const { return bytes_popped_; }
