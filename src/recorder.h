/*
 * recorder.h
 * Ultra-compact frame recorder for spherical wavefront visualization
 * Now supports relocation-aware centers per frame
 */

#ifndef RECORDER_H
#define RECORDER_H

#include <vector>
#include <fstream>
#include <cstring>
#include <iostream>
#include <cmath>
#include <set>
#include <zlib.h>
#include <cstdint>
#include "model/simulation.h"

// Tomography configuration (decoupled from GUI globals)
struct TomoConfig {
  bool enabled = false;
  // 0: XY (fix Z), 1: YZ (fix X), 2: ZX (fix Y)
  int axis = 0;
  unsigned slice = 0;
  inline bool match(unsigned x, unsigned y, unsigned z) const {
    if (!enabled) return true;
    if (axis == 0) return z == slice;
    if (axis == 1) return x == slice;
    if (axis == 2) return y == slice;
    return true;
  }
};

namespace automaton {
  extern std::vector<std::array<unsigned, 3>> lcenters;
}

namespace framework {

#pragma pack(push, 1)

struct WavefrontData {
  uint16_t t;       // Current wavefront radius
  uint8_t orphan;   // 1 if orphan (a==W_USED), 0 otherwise
};

struct LayerFrame {
  uint8_t center_x;
  uint8_t center_y;
  uint8_t center_z;
  uint8_t w;          // Layer index
  uint8_t relocated;  // relocation flag
  std::vector<WavefrontData> wavefronts;
};

#pragma pack(pop)

struct Frame {
  uint32_t k;                   // Global simulation tick
  std::vector<LayerFrame> layers;
  bool isKeyframe;
  Frame() : k(0), isKeyframe(false) {}
};

extern bool recordFrames;
extern bool toastActive;
extern double toastStartTime;
extern std::string toastMessage;

class FrameRecorder {
public:
  std::vector<Frame> frames;
  unsigned long long savedTimer = 0;
  int savedScenario = 0;

  size_t maxFrames_ = 50000;
  size_t maxMemoryMB_ = 100;
  bool recordingEnabled_ = true;

private:
  size_t estimatedMemoryUsage_ = 0;

public:
  size_t estimateFrameMemory(const Frame& f) const;
  bool canRecordFrame() const;

  void recordFrame(const std::vector<automaton::Cell>& lattice,
                   unsigned long long currentTimer,
                   int scenarioID);

  void applyFrame(const Frame& frame,
                  std::vector<automaton::Cell>& lattice,
                  const TomoConfig* tomo = nullptr) const;

  void reconstructVoxels(const Frame& frame,
                         COLORREF* voxels,
                         unsigned w,
                         const TomoConfig* tomo = nullptr) const;

  void saveToFile(const std::string& filename, bool useCompress=true) const;
  void loadFromFile(const std::string& filename);

  void clearFrames();
  long long getTimer();
  void printStats() const;

  size_t getFrameCount() const { return frames.size(); }
  size_t getMemoryUsageKB() const { return estimatedMemoryUsage_/1024; }
  bool isRecordingEnabled() const { return recordingEnabled_; }
};

} // namespace framework

#endif // RECORDER_H
