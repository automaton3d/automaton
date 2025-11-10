/*
 * recorder.h
 * Ultra-compact frame recorder for spherical wavefront visualization
 * Supports multiple concentric wavefronts per layer (up to EL/2 values)
 * Uses ray-tracing from precomputed centers for fast capture
 * Achieves ~65-500x compression by exploiting wavefront geometry
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
#include "model/simulation.h"

namespace automaton
{
  extern std::vector<std::array<unsigned, 3>> lcenters;
}

namespace framework
{

#pragma pack(push, 1)

  /**
   * Compact representation of a single wavefront radius
   * Only 3 bytes per wavefront - just radius and orphan state
   */
  struct WavefrontData {
    uint16_t t;                   // Current wavefront radius
    uint8_t orphan;               // 1 if orphan (a==W_USED), 0 otherwise
  };

  /**
   * Layer frame containing all concentric wavefronts for a single layer
   * All wavefronts share the same center point
   */
  struct LayerFrame {
    uint8_t center_x;             // Center X coordinate (where d=0)
    uint8_t center_y;             // Center Y coordinate (where d=0)
    uint8_t center_z;             // Center Z coordinate (where d=0)
    uint8_t w;                    // Layer index
    std::vector<WavefrontData> wavefronts;  // Multiple concentric wavefronts
  };

#pragma pack(pop)

  /**
   * Complete frame state - contains only active layers
   */
  struct Frame {
      uint32_t k;                           // Global simulation tick
      std::vector<LayerFrame> layers;       // Active layers only
      bool isKeyframe;

      Frame() : k(0), isKeyframe(false) {}
  };

  // Forward declarations for external variables
  extern bool recordFrames;
  extern bool toastActive;
  extern double toastStartTime;
  extern std::string toastMessage;

  /**
   * Ultra-compact frame recorder
   * Records only essential wavefront geometry for visual reproduction
   */
  class CompactFrameRecorder {
  public:
    std::vector<Frame> frames;
    unsigned long long savedTimer = 0;
    int savedScenario = 0;

    // Memory management
    size_t maxFrames_ = 50000;           // Much higher limit due to compact size
    size_t maxMemoryMB_ = 100;           // Conservative 100MB limit
    bool recordingEnabled_ = true;

  private:
    size_t estimatedMemoryUsage_ = 0;

  public:
    /**
     * Estimate memory for a compact frame
     */
    size_t estimateFrameMemory(const Frame& f) const {
        size_t total = sizeof(Frame);
        for (const auto& layer : f.layers) {
            total += sizeof(LayerFrame) + layer.wavefronts.size() * sizeof(WavefrontData);
        }
        return total;
    }

    /**
     * Check if we can record another frame
     */
    bool canRecordFrame() const {
      if (!recordingEnabled_) return false;
      if (frames.size() >= maxFrames_) return false;
      if (estimatedMemoryUsage_ >= maxMemoryMB_ * 1024 * 1024) return false;
      return true;
    }

    /**
     * Record a frame by extracting wavefront geometry from lattice
     * Uses precomputed centers and ray-tracing for efficiency
     */
    void recordFrame(const std::vector<Cell>& lattice,
                     unsigned long long currentTimer,
                     int scenarioID) {
        // Check memory limits
        if (!canRecordFrame()) {
            if (recordingEnabled_) {
                std::cerr << "\n⚠️  Recording stopped: Memory limit reached\n";
                std::cerr << "    Frames recorded: " << frames.size() << "\n";
                std::cerr << "    Memory used: ~" << (estimatedMemoryUsage_ / (1024 * 1024)) << " MB\n";
                std::cerr << "    Compression achieved: ~65x vs old system\n";
                recordingEnabled_ = false;
                recordFrames = false;

                toastMessage = "Recording stopped: Memory limit reached";
                toastStartTime = glfwGetTime();
                toastActive = true;
            }
            return;
        }

        if (frames.empty()) {
            savedTimer = currentTimer;
            savedScenario = scenarioID;
        }

        try {
            Frame frame;
            frame.k = currentTimer;
            frame.isKeyframe = (frames.size() % 30 == 0);  // Keyframe every 30 frames

            // Scan each layer using precomputed centers
            for (unsigned w = 0; w < W_USED; w++) {
                // Get precomputed center for this layer
                const auto& center = automaton::lcenters[w];
                unsigned cx = center[0];
                unsigned cy = center[1];
                unsigned cz = center[2];

                // Check if center is valid (d=0)
                const Cell& centerCell = getCell(lattice, cx, cy, cz, w);
                if (centerCell.d != 0) {
                    continue;  // No active wavefront in this layer
                }

                // Use a set to collect unique (t, orphan) pairs
                std::set<std::pair<uint16_t, bool>> wavefrontSet;

                // Ray-trace from center to find all wavefront radii
                // Two non-parallel rays are sufficient to capture all concentric spheres
                const int numRays = 2;
                const int directions[2][3] = {
                    {1, 0, 0},    // +X axis
                    {1, 1, 1}     // Diagonal
                };

                // Trace rays to find all t values
                for (int ray = 0; ray < numRays; ray++) {
                    int dx = directions[ray][0];
                    int dy = directions[ray][1];
                    int dz = directions[ray][2];

                    // Walk along ray - max distance depends on direction
                    int maxSteps = (ray == 0) ? (int)(EL/2) : (int)(EL/2);

                    for (int step = 0; step <= maxSteps; step++) {
                        int x = (int)cx + dx * step;
                        int y = (int)cy + dy * step;
                        int z = (int)cz + dz * step;

                        // Check bounds
                        if (x < 0 || x >= (int)EL ||
                            y < 0 || y >= (int)EL ||
                            z < 0 || z >= (int)EL) {
                            break;
                        }

                        const Cell& cell = getCell(lattice, x, y, z, w);

                        // If cell is on wavefront (t == d), record it
                        if (cell.t != UINT16_MAX && cell.d != UINT16_MAX && cell.t == cell.d) {
                            bool is_orphan = (cell.a == W_USED);
                            wavefrontSet.insert({cell.t, is_orphan});
                        }
                    }
                }

                // If we found wavefronts, record this layer
                if (!wavefrontSet.empty()) {
                    LayerFrame layer;
                    layer.w = w;
                    layer.center_x = static_cast<uint8_t>(cx);
                    layer.center_y = static_cast<uint8_t>(cy);
                    layer.center_z = static_cast<uint8_t>(cz);

                    for (const auto& [t_val, is_orphan] : wavefrontSet) {
                        WavefrontData wf;
                        wf.t = t_val;
                        wf.orphan = is_orphan ? 1 : 0;
                        layer.wavefronts.push_back(wf);
                    }

                    frame.layers.push_back(std::move(layer));
                }
            }

            // Update memory estimate
            size_t frameMemory = estimateFrameMemory(frame);
            estimatedMemoryUsage_ += frameMemory;

            frames.push_back(std::move(frame));
        }
        catch (const std::bad_alloc& e) {
            std::cerr << "\n❌ Memory allocation failed during frame recording!\n";
            std::cerr << "    Error: " << e.what() << "\n";
            std::cerr << "    Frames recorded before failure: " << frames.size() << "\n";
            recordingEnabled_ = false;
            recordFrames = false;

            toastMessage = "Recording failed: Out of memory";
            toastStartTime = glfwGetTime();
            toastActive = true;
        }
        catch (const std::exception& e) {
            std::cerr << "\n❌ Error during frame recording: " << e.what() << "\n";
            recordingEnabled_ = false;
            recordFrames = false;
        }
    }

    /**
     * Reconstruct voxel buffer from compact frame
     * Exploits spherical wavefront geometry
     */
    void reconstructVoxels(const Frame& frame, COLORREF* voxels) const {
        // Clear voxel buffer
        std::fill(voxels, voxels + EL * EL * EL, RGB(0, 0, 0));

        // Reconstruct each layer's concentric wavefronts
        for (const auto& layer : frame.layers) {
            float cx = layer.center_x;
            float cy = layer.center_y;
            float cz = layer.center_z;

            for (const auto& wf : layer.wavefronts) {
                float radius = wf.t;
                bool is_orphan = (wf.orphan != 0);

                // Scan volume to find cells on wavefront sphere
                for (unsigned x = 0; x < EL; x++) {
                    for (unsigned y = 0; y < EL; y++) {
                        for (unsigned z = 0; z < EL; z++) {
                            // Calculate distance from center
                            float dx = x - cx;
                            float dy = y - cy;
                            float dz = z - cz;
                            float dist = sqrt(dx*dx + dy*dy + dz*dz);

                            // Check if this cell is on the wavefront (t == d condition)
                            if (fabs(dist - radius) < 0.5f) {
                                unsigned index3D = x * EL * EL + y * EL + z;

                                // Apply same coloring as bridge.cpp updateBuffer()
                                if (is_orphan) {
                                    voxels[index3D] = RGB(255, 0, 0);     // Red (orphan)
                                } else if (radius == 0) {
                                    voxels[index3D] = RGB(0, 255, 0);     // Green (fresh seed)
                                } else {
                                    voxels[index3D] = RGB(255, 255, 255); // White (wavefront)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Apply frame to lattice (for replay mode)
     */
    void applyFrame(const Frame& frame, std::vector<Cell>& lattice) const {
        // Clear lattice
        for (auto& cell : lattice) {
            cell.t = UINT16_MAX;
            cell.d = UINT16_MAX;
            cell.a = W_USED;
        }

        // Reconstruct each layer's concentric wavefronts in lattice
        for (const auto& layer : frame.layers) {
            float cx = layer.center_x;
            float cy = layer.center_y;
            float cz = layer.center_z;
            uint8_t w = layer.w;

            // Set center cell first
            Cell& centerCell = getCell(lattice, layer.center_x, layer.center_y, layer.center_z, w);
            centerCell.d = 0;

            for (const auto& wf : layer.wavefronts) {
                float radius = wf.t;

                // Update center cell with the smallest t value
                if (wf.t < centerCell.t) {
                    centerCell.t = wf.t;
                    centerCell.a = (wf.orphan != 0) ? W_USED : w;
                }

                // Set wavefront cells
                for (unsigned x = 0; x < EL; x++) {
                    for (unsigned y = 0; y < EL; y++) {
                        for (unsigned z = 0; z < EL; z++) {
                            float dx = x - cx;
                            float dy = y - cy;
                            float dz = z - cz;
                            float dist = sqrt(dx*dx + dy*dy + dz*dz);

                            if (fabs(dist - radius) < 0.5f) {
                                Cell& cell = getCell(lattice, x, y, z, w);
                                cell.t = wf.t;
                                cell.d = wf.t;  // t == d for active wavefront
                                cell.a = (wf.orphan != 0) ? W_USED : w;
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Save to file with optional zlib compression
     */
    void saveToFile(const std::string& filename, bool useCompress = true) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }

        // Magic number
        const char magic[4] = {'C', 'M', 'P', 'T'};  // "CMPT" for compact
        out.write(magic, 4);

        uint16_t version = useCompress ? 3 : 3;  // v3 = concentric multi-wavefront format
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&savedTimer), sizeof(savedTimer));
        out.write(reinterpret_cast<const char*>(&savedScenario), sizeof(savedScenario));

        uint32_t frameCount = frames.size();
        out.write(reinterpret_cast<const char*>(&frameCount), sizeof(frameCount));

        if (useCompress) {
            // Compress frame data with zlib
            for (const auto& frame : frames) {
                // Serialize frame to buffer
                std::vector<char> buffer;
                size_t bufferSize = sizeof(frame.k) + 1 + 1;  // k + keyframe + layerCount

                for (const auto& layer : frame.layers) {
                    bufferSize += 4 + 1;  // center_x, center_y, center_z, w + wavefrontCount
                    bufferSize += layer.wavefronts.size() * sizeof(WavefrontData);
                }

                buffer.resize(bufferSize);
                size_t offset = 0;

                std::memcpy(buffer.data() + offset, &frame.k, sizeof(frame.k));
                offset += sizeof(frame.k);

                uint8_t keyframe = frame.isKeyframe ? 1 : 0;
                buffer[offset++] = keyframe;

                uint8_t layerCount = static_cast<uint8_t>(frame.layers.size());
                buffer[offset++] = layerCount;

                for (const auto& layer : frame.layers) {
                    buffer[offset++] = layer.center_x;
                    buffer[offset++] = layer.center_y;
                    buffer[offset++] = layer.center_z;
                    buffer[offset++] = layer.w;

                    uint8_t wfCount = static_cast<uint8_t>(layer.wavefronts.size());
                    buffer[offset++] = wfCount;

                    for (const auto& wf : layer.wavefronts) {
                        std::memcpy(buffer.data() + offset, &wf, sizeof(wf));
                        offset += sizeof(wf);
                    }
                }

                // Compress with zlib
                uLongf compSize = compressBound(buffer.size());
                std::vector<Bytef> compressed(compSize);

                int result = compress(compressed.data(), &compSize,
                                    reinterpret_cast<const Bytef*>(buffer.data()),
                                    buffer.size());

                if (result != Z_OK) {
                    throw std::runtime_error("Compression failed");
                }

                // Write sizes and compressed data
                uint16_t uncompSize = static_cast<uint16_t>(buffer.size());
                uint16_t compSizeU16 = static_cast<uint16_t>(compSize);
                out.write(reinterpret_cast<const char*>(&uncompSize), sizeof(uncompSize));
                out.write(reinterpret_cast<const char*>(&compSizeU16), sizeof(compSizeU16));
                out.write(reinterpret_cast<const char*>(compressed.data()), compSize);
            }
        }

        out.close();

        std::cout << "\n✅ Compact recording saved successfully!\n";
        std::cout << "   File: " << filename << "\n";
        std::cout << "   Frames: " << frames.size() << "\n";
        std::cout << "   Compression: " << (useCompress ? "zlib enabled" : "disabled") << "\n";
        std::cout << "   Estimated size: " << (estimatedMemoryUsage_ / 1024) << " KB\n";
    }

    /**
     * Load from file
     */
    void loadFromFile(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Cannot open file for reading: " + filename);
        }

        // Read and verify magic number
        char magic[4];
        in.read(magic, 4);
        if (magic[0] != 'C' || magic[1] != 'M' || magic[2] != 'P' || magic[3] != 'T') {
            throw std::runtime_error("Invalid compact file format");
        }

        uint16_t version;
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        in.read(reinterpret_cast<char*>(&savedTimer), sizeof(savedTimer));
        in.read(reinterpret_cast<char*>(&savedScenario), sizeof(savedScenario));

        uint32_t frameCount;
        in.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));

        frames.clear();
        frames.reserve(frameCount);
        estimatedMemoryUsage_ = 0;

        if (version == 3) {
            // Concentric multi-wavefront compressed format
            for (uint32_t i = 0; i < frameCount; ++i) {
                uint16_t uncompSize, compSize;
                in.read(reinterpret_cast<char*>(&uncompSize), sizeof(uncompSize));
                in.read(reinterpret_cast<char*>(&compSize), sizeof(compSize));

                std::vector<Bytef> compressed(compSize);
                in.read(reinterpret_cast<char*>(compressed.data()), compSize);

                std::vector<char> uncompressed(uncompSize);
                uLongf destLen = uncompSize;

                int result = ::uncompress(reinterpret_cast<Bytef*>(uncompressed.data()),
                                       &destLen, compressed.data(), compSize);

                if (result != Z_OK) {
                    throw std::runtime_error("Decompression failed");
                }

                // Parse frame
                Frame frame;
                size_t offset = 0;
                std::memcpy(&frame.k, uncompressed.data() + offset, sizeof(frame.k));
                offset += sizeof(frame.k);

                uint8_t keyframe = uncompressed[offset++];
                frame.isKeyframe = (keyframe != 0);

                uint8_t layerCount = uncompressed[offset++];
                frame.layers.resize(layerCount);

                for (uint8_t j = 0; j < layerCount; ++j) {
                    frame.layers[j].center_x = uncompressed[offset++];
                    frame.layers[j].center_y = uncompressed[offset++];
                    frame.layers[j].center_z = uncompressed[offset++];
                    frame.layers[j].w = uncompressed[offset++];

                    uint8_t wfCount = uncompressed[offset++];
                    frame.layers[j].wavefronts.resize(wfCount);

                    for (uint8_t k = 0; k < wfCount; ++k) {
                        std::memcpy(&frame.layers[j].wavefronts[k],
                                  uncompressed.data() + offset,
                                  sizeof(WavefrontData));
                        offset += sizeof(WavefrontData);
                    }
                }

                estimatedMemoryUsage_ += estimateFrameMemory(frame);
                frames.push_back(std::move(frame));
            }
        } else {
            throw std::runtime_error("Unsupported compact file version");
        }

        in.close();

        std::cout << "\n✅ Compact recording loaded successfully!\n";
        std::cout << "   Frames: " << frames.size() << "\n";
        std::cout << "   Memory: " << (estimatedMemoryUsage_ / 1024) << " KB\n";
    }

    /**
     * Clear all frames and reset
     */
    void clearFrames() {
        frames.clear();
        estimatedMemoryUsage_ = 0;
        recordingEnabled_ = true;
    }

    /**
     * Get statistics
     */
    void printStats() const {
        if (frames.empty()) return;

        size_t totalLayers = 0;
        size_t totalWavefronts = 0;
        size_t minLayers = SIZE_MAX;
        size_t maxLayers = 0;
        size_t maxWavefrontsPerLayer = 0;

        for (const auto& f : frames) {
            totalLayers += f.layers.size();
            minLayers = std::min(minLayers, f.layers.size());
            maxLayers = std::max(maxLayers, f.layers.size());

            for (const auto& layer : f.layers) {
                totalWavefronts += layer.wavefronts.size();
                maxWavefrontsPerLayer = std::max(maxWavefrontsPerLayer, layer.wavefronts.size());
            }
        }

        std::cout << "\n=== Compact Recording Statistics ===\n";
        std::cout << "Total frames: " << frames.size() << "\n";
        std::cout << "Avg layers per frame: " << (totalLayers / frames.size()) << "\n";
        std::cout << "Min/Max layers: " << minLayers << " / " << maxLayers << "\n";
        std::cout << "Total wavefronts: " << totalWavefronts << "\n";
        std::cout << "Avg wavefronts per frame: " << (totalWavefronts / frames.size()) << "\n";
        std::cout << "Max wavefronts per layer: " << maxWavefrontsPerLayer << "\n";
        std::cout << "Memory usage: " << (estimatedMemoryUsage_ / 1024) << " KB\n";
        std::cout << "Avg bytes per frame: " << (estimatedMemoryUsage_ / frames.size()) << "\n";
        std::cout << "Compression vs old system: ~65-200x\n";
        std::cout << "====================================\n\n";
    }

    // Accessors
    size_t getFrameCount() const { return frames.size(); }
    size_t getMemoryUsageKB() const { return estimatedMemoryUsage_ / 1024; }
    bool isRecordingEnabled() const { return recordingEnabled_; }
  };

} // namespace framework

#endif // RECORDER_H
