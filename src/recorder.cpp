/*
 * recorder.cpp
 * Implementation of FrameRecorder methods
 */

#include "recorder.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <array>            // for std::array completeness
#include <GLFW/glfw3.h>     // for glfwGetTime
#include <zlib.h>

namespace framework {

size_t FrameRecorder::estimateFrameMemory(const Frame& f) const {
    size_t total = sizeof(Frame);
    for (const auto& layer : f.layers) {
        total += sizeof(LayerFrame) + layer.wavefronts.size() * sizeof(WavefrontData);
    }
    return total;
}

bool FrameRecorder::canRecordFrame() const {
    if (!recordingEnabled_) return false;
    if (frames.size() >= maxFrames_) return false;
    if (estimatedMemoryUsage_ >= maxMemoryMB_ * 1024 * 1024) return false;
    return true;
}

void FrameRecorder::recordFrame(const std::vector<automaton::Cell>& lattice,
                                unsigned long long currentTimer,
                                int scenarioID) {
    if (!canRecordFrame()) {
        if (recordingEnabled_) {
            std::cerr << "\n⚠️ Recording stopped: Memory limit reached\n";
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

    Frame frame;
    frame.k = currentTimer;
    frame.isKeyframe = (frames.size() % 30 == 0);

    for (unsigned w = 0; w < automaton::W_USED; ++w) {
        const auto& staticCenter = automaton::lcenters[w];

        const automaton::Cell& centerCell =
            getCell(lattice, staticCenter[0], staticCenter[1], staticCenter[2], w);

        unsigned cx = centerCell.x[0];
        unsigned cy = centerCell.x[1];
        unsigned cz = centerCell.x[2];

        if (centerCell.d != 0) continue;

        std::set<std::pair<uint16_t, bool>> wavefrontSet;

        for (int dx = -static_cast<int>(automaton::EL / 2); dx <= static_cast<int>(automaton::EL / 2); ++dx) {
            unsigned x = (cx + dx + automaton::EL) % automaton::EL;

            const std::pair<int,int> neighbors[5] = {
                {0,0}, {1,0}, {-1,0}, {0,1}, {0,-1}
            };

            for (const auto& [dy,dz] : neighbors) {
                unsigned y = (cy + dy + automaton::EL) % automaton::EL;
                unsigned z = (cz + dz + automaton::EL) % automaton::EL;

                const automaton::Cell& c = getCell(lattice, x, y, z, w);
                if (c.t != c.d) continue;

                bool is_orphan = (c.a == automaton::W_USED);
                wavefrontSet.insert({c.t, is_orphan});
            }
        }

        if (!wavefrontSet.empty()) {
            LayerFrame layer;
            layer.w = w;
            layer.center_x = static_cast<uint8_t>(cx);
            layer.center_y = static_cast<uint8_t>(cy);
            layer.center_z = static_cast<uint8_t>(cz);
            layer.relocated = (cx != staticCenter[0] ||
                               cy != staticCenter[1] ||
                               cz != staticCenter[2]) ? 1 : 0;

            for (auto [t_val, is_orphan] : wavefrontSet) {
                WavefrontData wf{t_val, static_cast<uint8_t>(is_orphan ? 1 : 0)};
                layer.wavefronts.push_back(wf);
            }

            frame.layers.push_back(std::move(layer));
        }
    }

    estimatedMemoryUsage_ += estimateFrameMemory(frame);
    frames.push_back(std::move(frame));
}

void FrameRecorder::reconstructVoxels(const Frame& frame,
                                      COLORREF* voxels,
                                      unsigned w,
                                      const TomoConfig* tomo) const {
    // Clear
    std::fill(voxels, voxels + automaton::EL * automaton::EL * automaton::EL, RGB(0, 0, 0));

    for (const auto& layer : frame.layers) {
        if (layer.w != w) continue;

        int cx = static_cast<int>(layer.center_x);
        int cy = static_cast<int>(layer.center_y);
        int cz = static_cast<int>(layer.center_z);

        for (const auto& wf : layer.wavefronts) {
            int r = static_cast<int>(wf.t);
            COLORREF color = wf.orphan ? RGB(255, 0, 0)
                                       : (wf.t == 0 ? RGB(0, 255, 0) : RGB(255, 255, 255));

            for (int dx = -r - 1; dx <= r + 1; ++dx)
                for (int dy = -r - 1; dy <= r + 1; ++dy)
                    for (int dz = -r - 1; dz <= r + 1; ++dz) {
                        float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy + dz*dz));
                        if ((uint16_t)std::lround(dist) != wf.t) continue;

                        unsigned x = (cx + dx + automaton::EL) % automaton::EL;
                        unsigned y = (cy + dy + automaton::EL) % automaton::EL;
                        unsigned z = (cz + dz + automaton::EL) % automaton::EL;

                        if (tomo && !tomo->match(x, y, z)) continue;

                        unsigned idx = x * automaton::EL * automaton::EL + y * automaton::EL + z;
                        voxels[idx] = color;
                    }
        }
    }
}

void FrameRecorder::applyFrame(const Frame& frame,
                               std::vector<automaton::Cell>& lattice,
                               const TomoConfig* tomo) const {
    for (auto& cell : lattice) {
        cell.t = UINT16_MAX;
        cell.d = UINT16_MAX;
        cell.a = automaton::W_USED;
    }

    for (const auto& layer : frame.layers) {
        float cx = layer.center_x;
        float cy = layer.center_y;
        float cz = layer.center_z;
        uint8_t w = layer.w;

        automaton::Cell& centerCell = getCell(lattice, cx, cy, cz, w);
        centerCell.d = 0;

        for (const auto& wf : layer.wavefronts) {
            if (wf.t < centerCell.t) {
                centerCell.t = wf.t;
                centerCell.a = (wf.orphan ? automaton::W_USED : w);
            }

            for (unsigned x = 0; x < automaton::EL; x++)
                for (unsigned y = 0; y < automaton::EL; y++)
                    for (unsigned z = 0; z < automaton::EL; z++) {
                        if (tomo && !tomo->match(x, y, z)) continue;

                        float dx = x - cx;
                        float dy = y - cy;
                        float dz = z - cz;
                        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                        if (fabsf(dist - wf.t) < 0.5f) {
                            automaton::Cell& c = getCell(lattice, x, y, z, w);
                            c.t = wf.t;
                            c.d = wf.t;
                            c.a = (wf.orphan ? automaton::W_USED : w);
                        }
                    }
        }
    }
}

void FrameRecorder::saveToFile(const std::string& filename, bool useCompress) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open file for writing");

    const char magic[4] = {'C','M','P','T'};
    out.write(magic, 4);

    uint16_t version = 4; // relocation-aware format
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&savedTimer), sizeof(savedTimer));
    out.write(reinterpret_cast<const char*>(&savedScenario), sizeof(savedScenario));

    uint32_t frameCount = static_cast<uint32_t>(frames.size());
    out.write(reinterpret_cast<const char*>(&frameCount), sizeof(frameCount));

    for (const auto& frame : frames) {
        // Serialize frame to buffer
        std::vector<char> buffer;
        size_t bufferSize = sizeof(frame.k) + 1 + 1; // k + keyframe + layerCount
        for (const auto& layer : frame.layers) {
            bufferSize += 5 + 1; // center_x,y,z,w,relocated + wfCount
            bufferSize += layer.wavefronts.size() * sizeof(WavefrontData);
        }
        buffer.resize(bufferSize);
        size_t offset = 0;

        std::memcpy(buffer.data() + offset, &frame.k, sizeof(frame.k));
        offset += sizeof(frame.k);
        buffer[offset++] = frame.isKeyframe ? 1 : 0;
        buffer[offset++] = static_cast<uint8_t>(frame.layers.size());

        for (const auto& layer : frame.layers) {
            buffer[offset++] = layer.center_x;
            buffer[offset++] = layer.center_y;
            buffer[offset++] = layer.center_z;
            buffer[offset++] = layer.w;
            buffer[offset++] = layer.relocated;
            buffer[offset++] = static_cast<uint8_t>(layer.wavefronts.size());
            for (const auto& wf : layer.wavefronts) {
                std::memcpy(buffer.data() + offset, &wf, sizeof(wf));
                offset += sizeof(wf);
            }
        }

        if (useCompress) {
            uLongf compSize = compressBound(buffer.size());
            std::vector<Bytef> compressed(compSize);
            int result = compress(compressed.data(), &compSize,
                                  reinterpret_cast<const Bytef*>(buffer.data()),
                                  buffer.size());
            if (result != Z_OK) throw std::runtime_error("Compression failed");
            uint16_t uncompSize = static_cast<uint16_t>(buffer.size());
            uint16_t compSizeU16 = static_cast<uint16_t>(compSize);
            out.write(reinterpret_cast<const char*>(&uncompSize), sizeof(uncompSize));
            out.write(reinterpret_cast<const char*>(&compSizeU16), sizeof(compSizeU16));
            out.write(reinterpret_cast<const char*>(compressed.data()), compSize);
        } else {
            uint16_t uncompSize = static_cast<uint16_t>(buffer.size());
            out.write(reinterpret_cast<const char*>(&uncompSize), sizeof(uncompSize));
            out.write(buffer.data(), buffer.size());
        }
    }

    out.close();
    std::cout << "\n✅ Compact recording saved successfully!\n";
    std::cout << "   File: " << filename << "\n";
    std::cout << "   Frames: " << frames.size() << "\n";
    std::cout << "   Format version: " << 4 << "\n";
    std::cout << "   Estimated size: " << (estimatedMemoryUsage_ / 1024) << " KB\n";
}

void FrameRecorder::loadFromFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file for reading");

    char magic[4]; in.read(magic, 4);
    if (std::string(magic, 4) != "CMPT") throw std::runtime_error("Invalid compact file format");

    uint16_t version; in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&savedTimer), sizeof(savedTimer));
    in.read(reinterpret_cast<char*>(&savedScenario), sizeof(savedScenario));
    uint32_t frameCount; in.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));

    frames.clear(); frames.reserve(frameCount); estimatedMemoryUsage_ = 0;

    if (version == 4) {
        for (uint32_t i = 0; i < frameCount; ++i) {
            uint16_t uncompSize, compSize;
            in.read(reinterpret_cast<char*>(&uncompSize), sizeof(uncompSize));
            in.read(reinterpret_cast<char*>(&compSize), sizeof(compSize));
            std::vector<Bytef> compressed(compSize);
            in.read(reinterpret_cast<char*>(compressed.data()), compSize);
            std::vector<char> uncompressed(uncompSize);
            uLongf destLen = uncompSize;
            int result = ::uncompress(reinterpret_cast<Bytef*>(uncompressed.data()), &destLen,
                                      compressed.data(), compSize);
            if (result != Z_OK) throw std::runtime_error("Decompression failed");

            Frame frame; size_t offset = 0;
            std::memcpy(&frame.k, uncompressed.data() + offset, sizeof(frame.k));
            offset += sizeof(frame.k);
            frame.isKeyframe = (uncompressed[offset++] != 0);
            uint8_t layerCount = uncompressed[offset++];
            frame.layers.resize(layerCount);

            for (uint8_t j = 0; j < layerCount; ++j) {
                frame.layers[j].center_x = uncompressed[offset++];
                frame.layers[j].center_y = uncompressed[offset++];
                frame.layers[j].center_z = uncompressed[offset++];
                frame.layers[j].w        = uncompressed[offset++];
                frame.layers[j].relocated= uncompressed[offset++];
                uint8_t wfCount          = uncompressed[offset++];
                frame.layers[j].wavefronts.resize(wfCount);
                for (uint8_t k = 0; k < wfCount; ++k) {
                    std::memcpy(&frame.layers[j].wavefronts[k],
                                uncompressed.data() + offset, sizeof(WavefrontData));
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

void FrameRecorder::clearFrames() {
    frames.clear();
    estimatedMemoryUsage_ = 0;
    recordingEnabled_ = true;
}

long long FrameRecorder::getTimer() { return savedTimer; }

void FrameRecorder::printStats() const {
    if (frames.empty()) return;
    size_t totalLayers=0,totalWavefronts=0,minLayers=SIZE_MAX,maxLayers=0,maxWF=0;

    for (const auto& f : frames) {
        totalLayers += f.layers.size();
        minLayers = std::min(minLayers, f.layers.size());
        maxLayers = std::max(maxLayers, f.layers.size());
        for (const auto& l : f.layers) {
            totalWavefronts += l.wavefronts.size();
            maxWF = std::max(maxWF, l.wavefronts.size());
        }
    }

    std::cout << "\n=== Compact Recording Statistics ===\n";
    std::cout << "Total frames: " << frames.size() << "\n";
    std::cout << "Avg layers per frame: " << (totalLayers / frames.size()) << "\n";
    std::cout << "Min/Max layers: " << minLayers << " / " << maxLayers << "\n";
    std::cout << "Total wavefronts: " << totalWavefronts << "\n";
    std::cout << "Avg wavefronts per frame: " << (totalWavefronts / frames.size()) << "\n";
    std::cout << "Max wavefronts per layer: " << maxWF << "\n";
    std::cout << "Memory usage: " << (estimatedMemoryUsage_ / 1024) << " KB\n";
    std::cout << "Avg bytes per frame: " << (estimatedMemoryUsage_ / frames.size()) << "\n";
    std::cout << "====================================\n\n";
}

} // namespace framework
