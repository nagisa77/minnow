#pragma once

#include <vector>

#include "buffer.hh"
#include "ethernet_header.hh"
#include "parser.hh"

struct EthernetFrame {
  EthernetHeader header{};
  std::vector<Buffer> payload{};

  void parse(Parser& parser) {
    header.parse(parser);
    parser.all_remaining(payload);
  }

  void serialize(Serializer& serializer) const {
    header.serialize(serializer);
    serializer.buffer(payload);
  }
};
