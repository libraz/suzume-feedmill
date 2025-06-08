/**
 * @file progress_buffer.h
 * @brief Thread-safe progress buffer implementation
 */

#ifndef SUZUME_CORE_PROGRESS_BUFFER_H_
#define SUZUME_CORE_PROGRESS_BUFFER_H_

#include <atomic>
#include <cstdint>

namespace suzume {
namespace core {

/**
 * @brief Thread-safe progress buffer for inter-thread communication
 */
class ProgressBuffer {
public:
  /**
   * @brief Update progress atomically
   * @param phase Current phase
   * @param current Current progress value
   * @param total Total progress value
   */
  void updateProgress(uint32_t phase, uint32_t current, uint32_t total) {
    phase_.store(phase, std::memory_order_release);
    current_.store(current, std::memory_order_release);
    total_.store(total, std::memory_order_release);
  }

  /**
   * @brief Read progress atomically
   * @param phase Output phase value
   * @param current Output current value
   * @param total Output total value
   */
  void readProgress(uint32_t& phase, uint32_t& current, uint32_t& total) const {
    phase = phase_.load(std::memory_order_acquire);
    current = current_.load(std::memory_order_acquire);
    total = total_.load(std::memory_order_acquire);
  }

  /**
   * @brief Copy values to legacy buffer format
   * @param buffer Legacy uint32_t[3] buffer
   */
  void copyToLegacyBuffer(uint32_t* buffer) const {
    if (buffer) {
      buffer[0] = phase_.load(std::memory_order_acquire);
      buffer[1] = current_.load(std::memory_order_acquire);
      buffer[2] = total_.load(std::memory_order_acquire);
    }
  }

private:
  std::atomic<uint32_t> phase_{0};
  std::atomic<uint32_t> current_{0};
  std::atomic<uint32_t> total_{0};
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_PROGRESS_BUFFER_H_