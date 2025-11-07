#ifndef FRAME_RECORDER_H
#define FRAME_RECORDER_H

#include <vector>
#include <fstream>
#include "model/simulation.h"

namespace framework
{

#pragma pack(push, 1)

struct FrameCell
{
  uint8_t x, y, z, w;
  uint8_t flags;

  FrameCell() : x(0), y(0), z(0), w(0), flags(0) {}  // ← Add this

  FrameCell(unsigned x_, unsigned y_, unsigned z_, unsigned w_, const Cell& cell) {
    x = x_; y = y_; z = z_; w = w_;
    flags = 0;
    if (cell.t == cell.d)       flags |= 0x01;
    if (cell.a != W_USED)       flags |= 0x02;
    if (cell.sB)                flags |= 0x04;
    if (cell.pB)                flags |= 0x08;
    if (cell.d == 0)            flags |= 0x10;
  }
};
#pragma pack(pop)

struct Frame {
  std::vector<FrameCell> cells;

  void capture(const std::vector<Cell>& lattice) {
    cells.clear();
    for (unsigned w = 0; w < W_USED; ++w)
      for (unsigned x = 0; x < EL; ++x)
        for (unsigned y = 0; y < EL; ++y)
          for (unsigned z = 0; z < EL; ++z) {
            const Cell& cell = getCell(lattice, x, y, z, w);
            if (cell.t == cell.d || cell.a != W_USED || cell.sB || cell.pB || cell.d == 0)
              cells.emplace_back(x, y, z, w, cell);
          }
  }

  void apply(std::vector<Cell>& lattice) const
  {
    for (const FrameCell& fc : cells)
    {
      Cell& cell = getCell(lattice, fc.x, fc.y, fc.z, fc.w);
      cell.t = (fc.flags & 0x01) ? cell.d : cell.t;
      cell.a = (fc.flags & 0x02) ? cell.a : W_USED;
      cell.sB = fc.flags & 0x04;
      cell.pB = fc.flags & 0x08;
      cell.d = (fc.flags & 0x10) ? 0 : cell.d;
    }
  }

  void save(std::ostream& out) const {
    uint32_t count = static_cast<uint32_t>(cells.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    out.write(reinterpret_cast<const char*>(cells.data()), count * sizeof(FrameCell));
  }

  void load(std::istream& in) {
    uint32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    cells.resize(count);
    in.read(reinterpret_cast<char*>(cells.data()), count * sizeof(FrameCell));
  }
};

class FrameRecorder {
public:
  std::vector<Frame> frames;
  unsigned long long savedTimer = 0;  // ← Store timer when recording begins
  int savedScenario = 0;  // Stores scenario ID during recording

  void recordFrame(const std::vector<Cell>& lattice, unsigned long long currentTimer, int scenarioID) {
    if (frames.empty()) {
      savedTimer = currentTimer;
      savedScenario = scenarioID;
    }
    Frame f;
    f.capture(lattice);
    frames.push_back(std::move(f));
  }

  void saveToFile(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&savedTimer), sizeof(savedTimer));
    out.write(reinterpret_cast<const char*>(&savedScenario), sizeof(savedScenario));  // Save scenario ID

    uint32_t total = static_cast<uint32_t>(frames.size());
    out.write(reinterpret_cast<const char*>(&total), sizeof(total));
    for (const Frame& f : frames) {
      f.save(out);
    }
  }

  void loadFromFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    in.read(reinterpret_cast<char*>(&savedTimer), sizeof(savedTimer));
    in.read(reinterpret_cast<char*>(&savedScenario), sizeof(savedScenario));  // Load scenario ID

    uint32_t total;
    in.read(reinterpret_cast<char*>(&total), sizeof(total));
    frames.resize(total);
    for (Frame& f : frames) {
      f.load(in);
    }
  }
};


} // namespace automaton

#endif
