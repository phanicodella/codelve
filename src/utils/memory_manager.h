// File: codelve/src/utils/memory_manager.h
#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace codelve {
namespace utils {

/**
 * Memory Manager class for efficient memory allocation and tracking.
 * Especially important for handling the large memory needs of LLMs.
 */
class MemoryManager {
public:
    /**
     * Get singleton instance.
     */
    static MemoryManager& getInstance();
    
    /**
     * Initialize memory manager.
     * @param maxMemoryMB Maximum memory usage in MB
     * @return true if successful, false otherwise
     */
    bool initialize(size_t maxMemoryMB);
    
    /**
     * Allocate memory block of specified size.
     * @param sizeBytes Size in bytes
     * @return Pointer to allocated memory or nullptr if failed
     */
    void* allocate(size_t sizeBytes);
    
    /**
     * Free allocated memory.
     * @param ptr Pointer to memory block
     */
    void free(void* ptr);
    
    /**
     * Get current memory usage.
     * @return Current memory usage in bytes
     */
    size_t getCurrentUsage() const;
    
    /**
     * Get maximum allowed memory usage.
     * @return Maximum memory usage in bytes
     */
    size_t getMaxAllowed() const;
    
    /**
     * Get available system memory.
     * @return Available system memory in bytes
     */
    static size_t getSystemAvailableMemory();
    
    /**
     * Get total system memory.
     * @return Total system memory in bytes
     */
    static size_t getSystemTotalMemory();
    
    /**
     * Get memory usage report.
     * @return Formatted string with memory usage information
     */
    std::string getMemoryReport() const;

private:
    // Private constructor for singleton
    MemoryManager();
    
    // Prevent copying
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    // Maximum memory usage allowed (bytes)
    size_t maxMemoryBytes_;
    
    // Current memory usage (bytes)
    size_t currentUsageBytes_;
    
    // Singleton instance
    static std::unique_ptr<MemoryManager> instance_;
};

}} // namespace codelve::utils