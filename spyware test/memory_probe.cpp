#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>

namespace {

struct Options {
    std::size_t allocation_mb = 64;
    int interval_seconds = 2;
    int samples = 5;
};

void print_value(const std::string& label, std::uint64_t value, const std::string& unit) {
    std::cout << std::left << std::setw(28) << label << value << ' ' << unit << '\n';
}

std::uint64_t read_proc_kb(const std::string& key) {
    std::ifstream input("/proc/self/status");
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind(key, 0) == 0) {
            std::uint64_t value = 0;
            std::istringstream fields(line.substr(key.size()));
            fields >> value;
            return value;
        }
    }
    return 0;
}

std::uint64_t read_meminfo_kb(const std::string& key) {
    std::ifstream input("/proc/meminfo");
    std::string name;
    std::uint64_t value = 0;
    std::string unit;
    while (input >> name >> value >> unit) {
        if (name == key) {
            return value;
        }
    }
    return 0;
}

void print_snapshot(int sample) {
    struct rusage usage {};
    getrusage(RUSAGE_SELF, &usage);

    std::cout << "\nSample " << sample << "\n";
    print_value("Process resident memory", read_proc_kb("VmRSS:"), "KB");
    print_value("Process virtual memory", read_proc_kb("VmSize:"), "KB");
    print_value("System available memory", read_meminfo_kb("MemAvailable:"), "KB");
    print_value("Peak resident memory", static_cast<std::uint64_t>(usage.ru_maxrss), "KB");
    print_value("Voluntary context switches", static_cast<std::uint64_t>(usage.ru_nvcsw), "count");
    print_value("Involuntary context switches", static_cast<std::uint64_t>(usage.ru_nivcsw), "count");
    print_value("Open file descriptors", static_cast<std::uint64_t>(std::distance(
        std::filesystem::directory_iterator("/proc/self/fd"),
        std::filesystem::directory_iterator{})), "count");
}

void print_system_info() {
    struct utsname system {};
    if (uname(&system) == 0) {
        std::cout << "Kernel: " << system.sysname << ' ' << system.release
                  << " (" << system.machine << ")\n";
    }

    std::ifstream loadavg("/proc/loadavg");
    std::string load;
    std::getline(loadavg, load);
    std::cout << "Load average: " << load << '\n';

    struct statvfs disk {};
    if (statvfs(".", &disk) == 0) {
        const auto free_mb = static_cast<std::uint64_t>(disk.f_bavail) * disk.f_frsize / (1024 * 1024);
        const auto total_mb = static_cast<std::uint64_t>(disk.f_blocks) * disk.f_frsize / (1024 * 1024);
        print_value("Current filesystem free", free_mb, "MB");
        print_value("Current filesystem total", total_mb, "MB");
    }
}

bool parse_positive(const char* text, std::uint64_t& value) {
    try {
        const auto parsed = std::stoull(text);
        if (parsed == 0 || parsed > std::numeric_limits<std::uint64_t>::max()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_options(int argc, char* argv[], Options& options) {
    if (argc > 4) {
        return false;
    }

    std::uint64_t value = 0;
    if (argc >= 2 && (!parse_positive(argv[1], value) || value > 512)) {
        return false;
    }
    if (argc >= 2) {
        options.allocation_mb = static_cast<std::size_t>(value);
    }
    if (argc >= 3 && (!parse_positive(argv[2], value) || value > 60)) {
        return false;
    }
    if (argc >= 3) {
        options.interval_seconds = static_cast<int>(value);
    }
    if (argc >= 4 && (!parse_positive(argv[3], value) || value > 60)) {
        return false;
    }
    if (argc >= 4) {
        options.samples = static_cast<int>(value);
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    Options options;
    if (!parse_options(argc, argv, options)) {
        std::cerr << "Usage: " << argv[0] << " [allocation_mb 1-512] [interval_seconds 1-60] [samples 1-60]\n";
        return 2;
    }

    std::cout << "Cryptor isolated diagnostics\n"
              << "This test reads local metrics only and does not access network, credentials, or keyboard input.\n";
    print_system_info();
    print_snapshot(0);

    const auto bytes = options.allocation_mb * 1024ULL * 1024ULL;
    std::vector<std::uint8_t> memory(bytes);
    for (std::size_t offset = 0; offset < memory.size(); offset += 4096) {
        memory[offset] = static_cast<std::uint8_t>(offset / 4096);
    }

    std::cout << "\nTouched " << options.allocation_mb << " MB of test memory.\n";
    for (int sample = 1; sample <= options.samples; ++sample) {
        print_snapshot(sample);
        if (sample != options.samples) {
            std::this_thread::sleep_for(std::chrono::seconds(options.interval_seconds));
        }
    }

    memory.clear();
    memory.shrink_to_fit();
    std::cout << "\nReleased test memory. Final snapshot:\n";
    print_snapshot(options.samples + 1);
    return 0;
}
