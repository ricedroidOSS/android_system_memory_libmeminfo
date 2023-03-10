/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace android {
namespace dmabufinfo {

struct DmaBuffer {
  public:
    DmaBuffer(ino_t inode, uint64_t size, uint64_t count, const std::string& exporter,
              const std::string& name)
        : inode_(inode), size_(size), count_(count), exporter_(exporter), name_(name) {
        total_refs_ = 0;
    }
    DmaBuffer() = default;
    ~DmaBuffer() = default;

    // Adds one file descriptor reference for the given pid
    void AddFdRef(pid_t pid) {
        AddRefToPidMap(pid, &fdrefs_);
        total_refs_++;
    }

    // Adds one map reference for the given pid
    void AddMapRef(pid_t pid) {
        AddRefToPidMap(pid, &maprefs_);
        total_refs_++;
    }

    // Getters for each property
    uint64_t size() const { return size_; }
    const std::unordered_map<pid_t, int>& fdrefs() const { return fdrefs_; }
    const std::unordered_map<pid_t, int>& maprefs() const { return maprefs_; }
    ino_t inode() const { return inode_; }
    uint64_t total_refs() const { return total_refs_; }
    uint64_t count() const { return count_; };
    const std::set<pid_t>& pids() const { return pids_; }
    const std::string& name() const { return name_; }
    const std::string& exporter() const { return exporter_; }
    void SetName(const std::string& name) { name_ = name; }
    void SetExporter(const std::string& exporter) { exporter_ = exporter; }
    void SetCount(uint64_t count) { count_ = count; }
    uint64_t Pss() const { return size_ / pids_.size(); }

    bool operator==(const DmaBuffer& rhs) {
        return (inode_ == rhs.inode()) && (size_ == rhs.size()) && (name_ == rhs.name()) &&
               (exporter_ == rhs.exporter());
    }

  private:
    ino_t inode_;
    uint64_t size_;
    uint64_t count_;
    uint64_t total_refs_;
    std::set<pid_t> pids_;
    std::string exporter_;
    std::string name_;
    std::unordered_map<pid_t, int> fdrefs_;
    std::unordered_map<pid_t, int> maprefs_;
    void AddRefToPidMap(pid_t pid, std::unordered_map<pid_t, int>* map) {
        // The first time we find a ref, we set the ref count to 1
        // otherwise, increment the existing ref count
        auto [it, inserted] = map->insert(std::make_pair(pid, 1));
        if (!inserted) it->second++;
        pids_.insert(pid);
    }
};

// Read and return dmabuf objects for a given process without the help
// of DEBUGFS
// Returns false if something went wrong with the function, true otherwise.
bool ReadDmaBufInfo(pid_t pid, std::vector<DmaBuffer>* dmabufs, bool read_fdrefs = true,
                    const std::string& procfs_path = "/proc",
                    const std::string& dmabuf_sysfs_path = "/sys/kernel/dmabuf/buffers");

// Appends new fd-referenced dmabuf objects from a given process to an existing vector.
// If the vector contains an existing element with a matching inode, the reference
// counts are updated.
// On common kernels earlier than 5.4, reading fd-referenced dmabufs of other processes
// is only possible if the caller has root privileges. On 5.4+ common kernels the caller
// can read this information with the PTRACE_MODE_READ permission.
// Returns true on success, otherwise false.
bool ReadDmaBufFdRefs(int pid, std::vector<DmaBuffer>* dmabufs,
                      const std::string& procfs_path = "/proc");

// Appends new mapped dmabuf objects from a given process to an existing vector.
// If the vector contains an existing element with a matching inode, the reference
// counts are updated.
// Returns true on success, otherwise false.
bool ReadDmaBufMapRefs(pid_t pid, std::vector<DmaBuffer>* dmabufs,
                       const std::string& procfs_path = "/proc",
                       const std::string& dmabuf_sysfs_path = "/sys/kernel/dmabuf/buffers");



// Get the DMA buffers PSS contribution for the specified @pid
// Returns true on success, false otherwise
bool ReadDmaBufPss(int pid, uint64_t* pss, const std::string& procfs_path = "/proc",
                   const std::string& dmabuf_sysfs_path = "/sys/kernel/dmabuf/buffers");

// Writes DmaBuffer info into an existing vector (which will be cleared first.)
// Will include all DmaBuffers, whether thay are retained or mapped.
// Returns true on success, otherwise false.
bool ReadProcfsDmaBufs(std::vector<DmaBuffer>* bufs);

}  // namespace dmabufinfo
}  // namespace android
