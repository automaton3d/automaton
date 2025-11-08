/*
 * recorder.h
 * Frame recorder with delta encoding + zlib compression
 */

#ifndef FRAME_RECORDER_H
#define FRAME_RECORDER_H

#include <vector>
#include <fstream>
#include <unordered_map>
#include <cstring>
#include <zlib.h>
#include "model/simulation.h"

namespace framework
{

#pragma pack(push, 1)

struct FrameCell
{
  uint8_t x, y, z, w;
  uint8_t flags;

  FrameCell() : x(0), y(0), z(0), w(0), flags(0) {}

  FrameCell(unsigned x_, unsigned y_, unsigned z_, unsigned w_, const Cell& cell) {
    x = x_; y = y_; z = z_; w = w_;
    flags = 0;
    if (cell.t == cell.d)       flags |= 0x01;
    if (cell.a != W_USED)       flags |= 0x02;
    if (cell.sB)                flags |= 0x04;
    if (cell.pB)                flags |= 0x08;
    if (cell.d == 0)            flags |= 0x10;
  }

  // Generate unique key for cell position
  uint32_t getKey() const {
    return (static_cast<uint32_t>(w) << 24) |
           (static_cast<uint32_t>(x) << 16) |
           (static_cast<uint32_t>(y) << 8) |
           static_cast<uint32_t>(z);
  }

  bool operator==(const FrameCell& other) const {
    return x == other.x && y == other.y && z == other.z &&
           w == other.w && flags == other.flags;
  }
};
#pragma pack(pop)

struct Frame {
  std::vector<FrameCell> cells;
  bool isKeyframe = false;

  // Capture full frame
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

  // Capture only changes from previous frame
  void captureDelta(const std::vector<Cell>& lattice, const Frame* previous) {
    cells.clear();

    if (!previous) {
      isKeyframe = true;
      capture(lattice);
      return;
    }

    isKeyframe = false;

    // Build map of previous frame for O(1) lookup
    std::unordered_map<uint32_t, FrameCell> prevMap;
    prevMap.reserve(previous->cells.size());
    for (const auto& fc : previous->cells) {
      prevMap[fc.getKey()] = fc;
    }

    // Check all potentially active cells
    for (unsigned w = 0; w < W_USED; ++w)
      for (unsigned x = 0; x < EL; ++x)
        for (unsigned y = 0; y < EL; ++y)
          for (unsigned z = 0; z < EL; ++z) {
            const Cell& cell = getCell(lattice, x, y, z, w);

            // Compute current state
            bool isActive = (cell.t == cell.d || cell.a != W_USED ||
                           cell.sB || cell.pB || cell.d == 0);

            if (!isActive) continue;  // Skip inactive cells

            FrameCell current(x, y, z, w, cell);
            uint32_t key = current.getKey();

            // Check if this cell differs from previous frame
            auto it = prevMap.find(key);
            bool changed = (it == prevMap.end() || !(it->second == current));

            if (changed) {
              cells.push_back(current);
            }
          }
  }

  void apply(std::vector<Cell>& lattice) const {
    for (const FrameCell& fc : cells) {
      Cell& cell = getCell(lattice, fc.x, fc.y, fc.z, fc.w);
      cell.t = (fc.flags & 0x01) ? cell.d : cell.t;
      cell.a = (fc.flags & 0x02) ? cell.a : W_USED;
      cell.sB = fc.flags & 0x04;
      cell.pB = fc.flags & 0x08;
      cell.d = (fc.flags & 0x10) ? 0 : cell.d;
    }
  }

  // Save frame with zlib compression
  void save(std::ostream& out) const {
    // Serialize to buffer
    uint32_t count = static_cast<uint32_t>(cells.size());
    size_t uncompSize = sizeof(count) + count * sizeof(FrameCell);
    std::vector<char> uncompressed(uncompSize);

    // Write count
    std::memcpy(uncompressed.data(), &count, sizeof(count));

    // Write cells
    if (count > 0) {
      std::memcpy(uncompressed.data() + sizeof(count),
                  cells.data(),
                  count * sizeof(FrameCell));
    }

    // Compress
    uLongf compSize = compressBound(static_cast<uLong>(uncompSize));
    std::vector<Bytef> compressed(compSize);

    int result = compress(compressed.data(), &compSize,
                         reinterpret_cast<const Bytef*>(uncompressed.data()),
                         static_cast<uLong>(uncompSize));

    if (result != Z_OK) {
      throw std::runtime_error("Compression failed");
    }

    // Write keyframe flag
    uint8_t keyFlag = isKeyframe ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&keyFlag), sizeof(keyFlag));

    // Write uncompressed size
    uint32_t uncompSizeU32 = static_cast<uint32_t>(uncompSize);
    out.write(reinterpret_cast<const char*>(&uncompSizeU32), sizeof(uncompSizeU32));

    // Write compressed size
    uint32_t compSizeU32 = static_cast<uint32_t>(compSize);
    out.write(reinterpret_cast<const char*>(&compSizeU32), sizeof(compSizeU32));

    // Write compressed data
    out.write(reinterpret_cast<const char*>(compressed.data()), compSize);
  }

  // Load frame with zlib decompression
  void load(std::istream& in) {
    // Read keyframe flag
    uint8_t keyFlag;
    in.read(reinterpret_cast<char*>(&keyFlag), sizeof(keyFlag));
    isKeyframe = (keyFlag == 1);

    // Read sizes
    uint32_t uncompSize, compSize;
    in.read(reinterpret_cast<char*>(&uncompSize), sizeof(uncompSize));
    in.read(reinterpret_cast<char*>(&compSize), sizeof(compSize));

    // Read compressed data
    std::vector<Bytef> compressed(compSize);
    in.read(reinterpret_cast<char*>(compressed.data()), compSize);

    // Decompress
    std::vector<char> uncompressed(uncompSize);
    uLongf destLen = uncompSize;

    int result = uncompress(reinterpret_cast<Bytef*>(uncompressed.data()),
                           &destLen,
                           compressed.data(),
                           compSize);

    if (result != Z_OK) {
      throw std::runtime_error("Decompression failed");
    }

    // Read count
    uint32_t count;
    std::memcpy(&count, uncompressed.data(), sizeof(count));

    // Read cells
    cells.resize(count);
    if (count > 0) {
      std::memcpy(cells.data(),
                  uncompressed.data() + sizeof(count),
                  count * sizeof(FrameCell));
    }
  }
};

class FrameRecorder {
public:
  std::vector<Frame> frames;
  unsigned long long savedTimer = 0;
  int savedScenario = 0;
  int keyframeInterval = 30;  // Full frame every 30 frames

private:
  size_t totalUncompressed_ = 0;
  size_t totalCompressed_ = 0;

public:
  void recordFrame(const std::vector<Cell>& lattice,
                   unsigned long long currentTimer,
                   int scenarioID) {
    if (frames.empty()) {
      savedTimer = currentTimer;
      savedScenario = scenarioID;
    }

    Frame f;

    // Make keyframe periodically or on first frame
    bool makeKeyframe = (frames.size() % keyframeInterval == 0) || frames.empty();

    if (makeKeyframe) {
      f.isKeyframe = true;
      f.capture(lattice);
    } else {
      f.isKeyframe = false;
      f.captureDelta(lattice, &frames.back());
    }

    frames.push_back(std::move(f));
  }

  void saveToFile(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
      throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    // Write header
    const char magic[4] = {'F', 'R', 'E', 'C'};  // Magic number
    out.write(magic, 4);

    uint16_t version = 2;  // Version 2 = compressed
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    out.write(reinterpret_cast<const char*>(&savedTimer), sizeof(savedTimer));
    out.write(reinterpret_cast<const char*>(&savedScenario), sizeof(savedScenario));
    out.write(reinterpret_cast<const char*>(&keyframeInterval), sizeof(keyframeInterval));

    uint32_t total = static_cast<uint32_t>(frames.size());
    out.write(reinterpret_cast<const char*>(&total), sizeof(total));

    // Write frames
    for (const Frame& f : frames) {
      f.save(out);
    }

    out.close();
  }

  void loadFromFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
      throw std::runtime_error("Cannot open file for reading: " + filename);
    }

    // Read and verify magic number
    char magic[4];
    in.read(magic, 4);
    if (magic[0] != 'F' || magic[1] != 'R' || magic[2] != 'E' || magic[3] != 'C') {
      throw std::runtime_error("Invalid file format");
    }

    // Read version
    uint16_t version;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (version == 1) {
      // Old uncompressed format - would need backward compatibility code
      throw std::runtime_error("Old format not supported, please re-record");
    } else if (version == 2) {
      // New compressed format
      in.read(reinterpret_cast<char*>(&savedTimer), sizeof(savedTimer));
      in.read(reinterpret_cast<char*>(&savedScenario), sizeof(savedScenario));
      in.read(reinterpret_cast<char*>(&keyframeInterval), sizeof(keyframeInterval));

      uint32_t total;
      in.read(reinterpret_cast<char*>(&total), sizeof(total));

      frames.clear();
      frames.reserve(total);

      for (uint32_t i = 0; i < total; ++i) {
        Frame f;
        f.load(in);
        frames.push_back(std::move(f));
      }
    } else {
      throw std::runtime_error("Unsupported file version");
    }

    in.close();
  }

  // Get compression statistics
  void printStats() const {
    if (frames.empty()) return;

    size_t totalCells = 0;
    size_t keyframeCells = 0;
    size_t deltaCells = 0;
    int keyframeCount = 0;

    for (const auto& f : frames) {
      totalCells += f.cells.size();
      if (f.isKeyframe) {
        keyframeCells += f.cells.size();
        keyframeCount++;
      } else {
        deltaCells += f.cells.size();
      }
    }

    std::cout << "\n=== Frame Recording Statistics ===\n";
    std::cout << "Total frames: " << frames.size() << "\n";
    std::cout << "Keyframes: " << keyframeCount
              << " (" << (100.0 * keyframeCount / frames.size()) << "%)\n";
    std::cout << "Avg cells per frame: " << (totalCells / frames.size()) << "\n";
    std::cout << "Avg cells per keyframe: "
              << (keyframeCount > 0 ? keyframeCells / keyframeCount : 0) << "\n";
    std::cout << "Avg cells per delta: "
              << ((frames.size() - keyframeCount) > 0 ? deltaCells / (frames.size() - keyframeCount) : 0) << "\n";

    // Estimate compression ratio
    size_t uncompressed = totalCells * sizeof(FrameCell);
    std::cout << "Estimated uncompressed: " << (uncompressed / 1024) << " KB\n";
    std::cout << "==================================\n\n";
  }
};

} // namespace framework

#endif
