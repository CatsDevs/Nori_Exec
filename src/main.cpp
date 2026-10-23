#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

class NoriApi {
public:
    explicit NoriApi(fs::path workspace)
        : workspace_(fs::absolute(std::move(workspace)).lexically_normal()) {
        fs::create_directories(workspace_);
    }

    // Standalone getgenv-style shared key/value storage.
    std::unordered_map<std::string, std::string>& getgenv() {
        return globals_;
    }

    void writefile(const std::string& name, const std::string& data) {
        const fs::path path = resolve(name);
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("writefile: cannot open file");
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!out) throw std::runtime_error("writefile: write failed");
    }

    std::string readfile(const std::string& name) const {
        std::ifstream in(resolve(name), std::ios::binary);
        if (!in) throw std::runtime_error("readfile: file not found");
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }

    void appendfile(const std::string& name, const std::string& data) {
        const fs::path path = resolve(name);
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("appendfile: cannot open file");
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!out) throw std::runtime_error("appendfile: write failed");
    }

    bool isfile(const std::string& name) const {
        std::error_code ec;
        return fs::is_regular_file(resolve(name), ec) && !ec;
    }

    bool isfolder(const std::string& name) const {
        std::error_code ec;
        return fs::is_directory(resolve(name), ec) && !ec;
    }

    void makefolder(const std::string& name) {
        fs::create_directories(resolve(name));
    }

    std::vector<std::string> listfiles(const std::string& name = ".") const {
        const fs::path dir = resolve(name);
        if (!fs::is_directory(dir)) throw std::runtime_error("listfiles: directory not found");
        std::vector<std::string> result;
        for (const auto& entry : fs::directory_iterator(dir))
            result.push_back(entry.path().filename().string());
        std::sort(result.begin(), result.end());
        return result;
    }

    void delfile(const std::string& name) {
        const fs::path path = resolve(name);
        if (!fs::is_regular_file(path) || !fs::remove(path))
            throw std::runtime_error("delfile: file not found");
    }

    void delfolder(const std::string& name) {
        const fs::path path = resolve(name);
        if (path == workspace_) throw std::runtime_error("delfolder: cannot delete workspace root");
        if (!fs::is_directory(path)) throw std::runtime_error("delfolder: directory not found");
        fs::remove_all(path);
    }

    static std::string base64encode(const std::string& input) {
        static constexpr char alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, bits = -6;
        for (unsigned char c : input) {
            val = (val << 8) | c;
            bits += 8;
            while (bits >= 0) {
                out.push_back(alphabet[(val >> bits) & 0x3F]);
                bits -= 6;
            }
        }
        if (bits > -6) out.push_back(alphabet[((val << 8) >> (bits + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    static std::string base64decode(const std::string& input) {
        static constexpr unsigned char invalid = 0xFF;
        static const auto table = [] {
            std::vector<unsigned char> t(256, static_cast<unsigned char>(0xFF));
            const std::string chars =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            for (std::size_t i = 0; i < chars.size(); ++i)
                t[static_cast<unsigned char>(chars[i])] = static_cast<unsigned char>(i);
            return t;
        }();

        std::string out;
        int val = 0, bits = -8;
        for (unsigned char c : input) {
            if (c == '=') break;
            if (std::isspace(c)) continue;
            if (table[c] == invalid) throw std::runtime_error("base64decode: invalid input");
            val = (val << 6) | table[c];
            bits += 6;
            if (bits >= 0) {
                out.push_back(static_cast<char>((val >> bits) & 0xFF));
                bits -= 8;
            }
        }
        return out;
    }

    static void rconsoleprint(const std::string& text) {
        std::cout << text << std::flush;
    }

    static void rconsoleclear() {
        std::cout << "\x1B[2J\x1B[H" << std::flush;
    }

private:
    fs::path workspace_;
    std::unordered_map<std::string, std::string> globals_;

    fs::path resolve(const std::string& name) const {
        if (name.empty()) throw std::runtime_error("path cannot be empty");
        const fs::path input(name);
        if (input.is_absolute() || input.has_root_name() || input.has_root_directory())
            throw std::runtime_error("absolute paths are not allowed");

        const fs::path result = (workspace_ / input).lexically_normal();
        const fs::path relative = result.lexically_relative(workspace_);
        if (relative.empty() || *relative.begin() == "..")
            throw std::runtime_error("path traversal denied");

        // Reject symlink escapes by checking the canonical existing parent/path.
        std::error_code ec;
        const fs::path canonicalRoot = fs::weakly_canonical(workspace_, ec);
        if (ec) throw std::runtime_error("cannot resolve workspace");
        const fs::path canonicalTarget = fs::weakly_canonical(result, ec);
        if (!ec) {
            const fs::path rel = canonicalTarget.lexically_relative(canonicalRoot);
            if (rel.empty() && canonicalTarget != canonicalRoot)
                throw std::runtime_error("path traversal denied");
            if (!rel.empty() && *rel.begin() == "..")
                throw std::runtime_error("path traversal denied");
        }
        return result;
    }
};

static int runTests(const fs::path& root) {
    const fs::path temp = root / "nori_exec_test_workspace";
    std::error_code ec;
    fs::remove_all(temp, ec);
    NoriApi api(temp);

    api.writefile("sample.txt", "Nori");
    if (api.readfile("sample.txt") != "Nori") return 1;
    api.appendfile("sample.txt", " Exec");
    if (api.readfile("sample.txt") != "Nori Exec") return 2;
    if (!api.isfile("sample.txt")) return 3;

    api.makefolder("nested");
    if (!api.isfolder("nested")) return 4;
    api.writefile("nested/data.txt", "ok");
    if (api.listfiles("nested").size() != 1) return 5;

    api.getgenv()["answer"] = "42";
    if (api.getgenv()["answer"] != "42") return 6;

    const std::string sample = "Hello, Luau!";
    if (NoriApi::base64decode(NoriApi::base64encode(sample)) != sample) return 7;

    bool traversalBlocked = false;
    try { api.writefile("../escape.txt", "no"); }
    catch (const std::runtime_error&) { traversalBlocked = true; }
    if (!traversalBlocked) return 8;

    fs::remove_all(temp, ec);
    std::cout << "All Nori API self-tests passed.\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--test")
            return runTests(fs::current_path());

        NoriApi api(fs::current_path() / "workspace");
        api.getgenv()["executor_name"] = "Nori Exec";
        api.rconsoleprint("Nori Exec standalone API initialized.\n");
        api.rconsoleprint("Workspace: " + (fs::current_path() / "workspace").string() + "\n");
        api.rconsoleprint("Run with --test to execute self-tests.\n");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}
