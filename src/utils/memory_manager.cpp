// File: codelve/src/utils/memory_manager.cpp
#include "utils/memory_manager.h"
#include "utils/logger.h"
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#endif

namespace codelve {
namespace utils {

// Initialize static instance
std::unique_ptr<MemoryManager> MemoryManager::instance_ = nullptr;

MemoryManager::MemoryManager()
    : maxMemoryBytes_(0),
      currentUsageBytes_(0) {
}

MemoryManager& MemoryManager::getInstance() {
    if (!instance_) {
        instance_ = std::unique_ptr<MemoryManager>(new MemoryManager());
    }
    return *instance_;
}

bool MemoryManager::initialize(size_t maxMemoryMB) {
    maxMemoryBytes_ = maxMemoryMB * 1024 * 1024;
    currentUsageBytes_ = 0;
    
    LOG_INFO("Memory Manager initialized with " + std::to_string(maxMemoryMB) + "MB limit");
    
    // Check if we have enough memory available
    size_t availableMemory = getSystemAvailableMemory();
    if (maxMemoryBytes_ > availableMemory) {
        LOG_WARNING("Requested memory limit exceeds available system memory");
        LOG_WARNING("Available: " + std::to_string(availableMemory / (1024 * 1024)) + "MB, Requested: " + std::to_string(maxMemoryMB) + "MB");
    }
    
    return true;
}

void* MemoryManager::allocate(size_t sizeBytes) {
    // Check if allocation would exceed our limit
    if (currentUsageBytes_ + sizeBytes > maxMemoryBytes_) {
        LOG_ERROR("Memory allocation of " + std::to_string(sizeBytes) + " bytes would exceed limit");
        return nullptr;
    }
    
    // Attempt to allocate memory
    void* ptr = nullptr;
    try {
        ptr = ::operator new(sizeBytes);
    }
    catch (const std::bad_alloc&) {
        LOG_ERROR("Failed to allocate " + std::to_string(sizeBytes) + " bytes");
        return nullptr;
    }
    
    // Update usage tracking
    currentUsageBytes_ += sizeBytes;
    
    LOG_DEBUG("Allocated " + std::to_string(sizeBytes) + " bytes, total usage: " + 
              std::to_string(currentUsageBytes_ / (1024 * 1024)) + "MB");
    
    return ptr;
}

void MemoryManager::free(void* ptr) {
    if (ptr) {
        // For now, we don't track the size of each allocation
        // In a more sophisticated implementation, we would keep a map of pointers to sizes
        
        ::operator delete(ptr);
        
        // For demonstration, let's assume we know how much memory was freed
        // This is not accurate and will be improved later
        currentUsageBytes_ = currentUsageBytes_ > 1024 ? currentUsageBytes_ - 1024 : 0;
        
        LOG_DEBUG("Memory freed, current usage: " + std::to_string(currentUsageBytes_ / (1024 * 1024)) + "MB");
    }
}

size_t MemoryManager::getCurrentUsage() const {
    return currentUsageBytes_;
}

size_t MemoryManager::getMaxAllowed() const {
    return maxMemoryBytes_;
}

size_t MemoryManager::getSystemAvailableMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return static_cast<size_t>(memInfo.ullAvailPhys);
#elif defined(__linux__)
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    return static_cast<size_t>(memInfo.freeram) * memInfo.mem_unit;
#else
    // Default fallback for unsupported platforms
    return 1024 * 1024 * 1024; // 1GB
#endif
}

size_t MemoryManager::getSystemTotalMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return static_cast<size_t>(memInfo.ullTotalPhys);
#elif defined(__linux__)
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    return static_cast<size_t>(memInfo.totalram) * memInfo.mem_unit;
#else
    // Default fallback for unsupported platforms
    return 2 * 1024 * 1024 * 1024; // 2GB
#endif
}

std::string MemoryManager::getMemoryReport() const {
    std::stringstream ss;
    
    // Format memory sizes
    auto formatSize = [](size_t bytes) -> std::string {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024 && unitIndex < 4) {
            size /= 1024;
            unitIndex++;
        }
        
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return stream.str();
    };
    
    // Get system memory info
    size_t totalSystem = getSystemTotalMemory();
    size_t availableSystem = getSystemAvailableMemory();
    size_t usedSystem = totalSystem - availableSystem;
    
    // Build report
    ss << "Memory Report:\n";
    ss << "  System Total: " << formatSize(totalSystem) << "\n";
    ss << "  System Used:  " << formatSize(usedSystem) << " (" 
       << std::fixed << std::setprecision(1) << (static_cast<double>(usedSystem) / totalSystem * 100) << "%)\n";
    ss << "  System Avail: " << formatSize(availableSystem) << "\n";
    ss << "  App Limit:    " << formatSize(maxMemoryBytes_) << "\n";
    ss << "  App Usage:    " << formatSize(currentUsageBytes_) << " (" 
       << std::fixed << std::setprecision(1) << (static_cast<double>(currentUsageBytes_) / maxMemoryBytes_ * 100) << "%)";
    
    return ss.str();
}

}} // namespace codelve::utils