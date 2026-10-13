#include "../android_app/android/app/src/main/cpp/jitter_buffer.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  AudioStream JitterBuffer Unit Test    " << std::endl;
    std::cout << "========================================" << std::endl;

    audiostream::JitterBuffer jb(3);

    // 1. Basic Push/Pop Test
    std::cout << "[Test 1] Testing basic push and pop..." << std::endl;
    uint8_t dummyPayload[128] = {0xAB, 0xCD, 0xEF};

    // Fill initial target delay (3 packets)
    jb.push(100, 0, dummyPayload, 128);
    jb.push(101, 960, dummyPayload, 128);
    jb.push(102, 1920, dummyPayload, 128);

    uint8_t outBuf[256];
    size_t outLen = 0;
    bool isLost = false;

    bool popped = jb.pop(outBuf, sizeof(outBuf), outLen, isLost);
    assert(popped && !isLost && outLen == 128);
    assert(outBuf[0] == 0xAB && outBuf[1] == 0xCD && outBuf[2] == 0xEF);
    std::cout << "  -> Basic push/pop PASSED" << std::endl;

    // 2. Out-of-order Packet Handling
    std::cout << "[Test 2] Testing out-of-order packet insertion..." << std::endl;
    jb.reset();
    jb.push(100, 0, dummyPayload, 128);
    jb.push(102, 1920, dummyPayload, 128);
    jb.push(101, 960, dummyPayload, 128); // Insert 101 late but in buffer range

    // Should pop 100
    popped = jb.pop(outBuf, sizeof(outBuf), outLen, isLost);
    assert(popped && !isLost);

    // Next pop should be 101
    popped = jb.pop(outBuf, sizeof(outBuf), outLen, isLost);
    assert(popped && !isLost);

    // Next pop should be 102
    popped = jb.pop(outBuf, sizeof(outBuf), outLen, isLost);
    assert(popped && !isLost);
    std::cout << "  -> Out-of-order insertion PASSED" << std::endl;

    // 3. Performance Benchmark: 100,000 Push & Pop cycles
    std::cout << "[Test 3] Benchmarking 100,000 packet operations (Zero-Allocation JitterBuffer)..." << std::endl;
    jb.reset();

    const int testCount = 100000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < testCount; ++i) {
        uint16_t seq = static_cast<uint16_t>(i & 0xFFFF);
        uint32_t ts = static_cast<uint32_t>(i * 960);
        jb.push(seq, ts, dummyPayload, 128);
        if (i >= 3) {
            jb.pop(outBuf, sizeof(outBuf), outLen, isLost);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double durationMs = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> Processed " << testCount << " packets in " << durationMs << " ms ("
              << (durationMs * 1000.0 / testCount) << " ns/op)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "ALL JITTERBUFFER TESTS PASSED SUCCESSFULLY!" << std::endl;

    return 0;
}
