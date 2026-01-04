#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <io.h>
#include <fcntl.h>

// 在 Windows 中设置控制台编码
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// 配置结构体
struct Config {
    std::string keyword;
    std::string newName;
    std::string command;
    bool executeCommand = false;
    bool verbose = false;
};

// 设置控制台编码
void setConsoleEncoding() {
#ifdef _WIN32
    // Windows 下设置控制台编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // 或者使用本地编码（GBK）
    // SetConsoleOutputCP(936);
    // SetConsoleCP(936);
#endif
    
    // 对于 Linux/macOS，通常默认就是 UTF-8
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
}

// 显示帮助信息
void showHelp() {
    std::cout << "文件夹重命名工具 v1.0\n";
    std::cout << "用法: ./rename_folders [选项]\n";
    std::cout << "选项:\n";
    std::cout << "  -k, --keyword <关键词>    要搜索的关键词 (必需)\n";
    std::cout << "  -n, --newname <新名称>    重命名的目标名称 (必需)\n";
    std::cout << "  -c, --command <命令>      重命名后执行的命令 (可选)\n";
    std::cout << "  -v, --verbose             显示详细信息\n";
    std::cout << "  -h, --help                显示此帮助信息\n";
    std::cout << "\n示例:\n";
    std::cout << "  ./rename_folders -k \"old\" -n \"new\"\n";
    std::cout << "  ./rename_folders -k \"temp\" -n \"final\" -c \"echo 完成\"\n";
}

// 解析命令行参数
bool parseArguments(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp();
            return false;
        }
        else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "-k" || arg == "--keyword") {
            if (i + 1 < argc) {
                config.keyword = argv[++i];
            } else {
                std::cerr << "错误: 缺少关键词参数\n";
                return false;
            }
        }
        else if (arg == "-n" || arg == "--newname") {
            if (i + 1 < argc) {
                config.newName = argv[++i];
            } else {
                std::cerr << "错误: 缺少新名称参数\n";
                return false;
            }
        }
        else if (arg == "-c" || arg == "--command") {
            if (i + 1 < argc) {
                config.command = argv[++i];
                config.executeCommand = true;
            } else {
                std::cerr << "错误: 缺少命令参数\n";
                return false;
            }
        }
        else {
            std::cerr << "未知选项: " << arg << "\n";
            return false;
        }
    }
    
    if (config.keyword.empty() || config.newName.empty()) {
        std::cerr << "错误: 关键词和新名称必须提供\n";
        showHelp();
        return false;
    }
    
    return true;
}

// 检查文件夹名是否包含关键词
bool containsKeyword(const std::string& folderName, const std::string& keyword) {
    std::string lowerFolder = folderName;
    std::string lowerKeyword = keyword;
    
    // 转换为小写进行不区分大小写的匹配
    std::transform(lowerFolder.begin(), lowerFolder.end(), lowerFolder.begin(), ::tolower);
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
    
    return lowerFolder.find(lowerKeyword) != std::string::npos;
}

// 执行附加命令
bool executeExternalCommand(const std::string& command, bool verbose) {
    if (verbose) {
        std::cout << "执行命令: " << command << "\n";
    }
    
    int result = std::system(command.c_str());
    
    if (result != 0) {
        std::cerr << "命令执行失败 (退出码: " << result << ")\n";
        return false;
    }
    
    if (verbose) {
        std::cout << "命令执行成功\n";
    }
    
    return true;
}

// 主函数
int main(int argc, char* argv[]) {
    // 设置控制台编码
    setConsoleEncoding();
    
    Config config;
    
    // 解析命令行参数
    if (!parseArguments(argc, argv, config)) {
        return 1;
    }
    
    if (config.verbose) {
        std::cout << "开始处理...\n";
        std::cout << "关键词: " << config.keyword << "\n";
        std::cout << "新名称: " << config.newName << "\n";
        if (config.executeCommand) {
            std::cout << "附加命令: " << config.command << "\n";
        }
        std::cout << "------------------------\n";
    }
    
    try {
        std::vector<fs::path> matchedFolders;
        
        // 遍历当前目录
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_directory()) {
                std::string folderName = entry.path().filename().string();
                
                if (containsKeyword(folderName, config.keyword)) {
                    matchedFolders.push_back(entry.path());
                    
                    if (config.verbose) {
                        std::cout << "找到匹配文件夹: " << folderName << "\n";
                    }
                }
            }
        }
        
        if (matchedFolders.empty()) {
            std::cout << "未找到包含关键词 \"" << config.keyword << "\" 的文件夹\n";
            return 0;
        }
        
        if (matchedFolders.size() > 1) {
            std::cout << "警告: 找到 " << matchedFolders.size() 
                      << " 个匹配的文件夹，但一次运行只能处理一个关键词\n";
            std::cout << "请指定更具体的关键词\n";
            return 1;
        }
        
        // 重命名文件夹
        fs::path oldPath = matchedFolders[0];
        fs::path newPath = config.newName;
        
        // 检查新名称是否已存在
        if (fs::exists(newPath)) {
            std::cerr << "错误: 目标名称 \"" << config.newName << "\" 已存在\n";
            return 1;
        }
        
        // 执行重命名
        fs::rename(oldPath, newPath);
        std::cout << "重命名成功: " << oldPath.filename() << " -> " << config.newName << "\n";
        
        // 执行附加命令
        if (config.executeCommand) {
            std::cout << "\n执行附加命令...\n";
            if (!executeExternalCommand(config.command, config.verbose)) {
                return 1;
            }
        }
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "文件系统错误: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
    
    if (config.verbose) {
        std::cout << "\n处理完成!\n";
    }
    
    return 0;
}
