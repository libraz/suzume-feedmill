/**
 * @file managed_buffer.h
 * @brief RAII-based buffer management for memory safety
 */

#ifndef SUZUME_CORE_MANAGED_BUFFER_H_
#define SUZUME_CORE_MANAGED_BUFFER_H_

#include <memory>
#include <cstdint>

namespace suzume {
namespace core {

/**
 * @brief RAII wrapper for raw memory buffers
 */
class ManagedBuffer {
public:
  /**
   * @brief Create buffer with specified size
   * @param size Buffer size in bytes
   */
  explicit ManagedBuffer(size_t size) 
    : data_(std::make_unique<uint8_t[]>(size)), size_(size) {}

  /**
   * @brief Get raw pointer (for compatibility with C APIs)
   * @return Raw pointer to buffer data
   */
  uint8_t* get() { return data_.get(); }

  /**
   * @brief Get const raw pointer
   * @return Const raw pointer to buffer data
   */
  const uint8_t* get() const { return data_.get(); }

  /**
   * @brief Get buffer size
   * @return Buffer size in bytes
   */
  size_t size() const { return size_; }

  /**
   * @brief Release ownership and return raw pointer
   * @return Raw pointer (caller must delete[] it)
   */
  uint8_t* release() { 
    size_ = 0;
    return data_.release(); 
  }

  /**
   * @brief Check if buffer is valid
   * @return True if buffer contains data
   */
  bool valid() const { return data_ != nullptr; }

private:
  std::unique_ptr<uint8_t[]> data_;
  size_t size_;
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_MANAGED_BUFFER_H_