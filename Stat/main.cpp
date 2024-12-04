#include <cmath>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <grp.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// 文件信息结构体，封装单个文件的元数据信息
struct FileInfo {
  std::string path;        // 文件的完整路径
  std::string name;        // 文件名
  mode_t mode;             // 文件模式（类型和权限）
  int links;               // 硬链接数
  std::string owner;       // 所有者名称
  std::string group;       // 所属组名称
  off_t size;              // 文件大小
  time_t modificationTime; // 最后修改时间
  std::string linkTarget; // 符号链接目标路径（如果是符号链接）

  // 构造函数初始化路径和文件名
  FileInfo(const std::string &path, const std::string &name)
      : path(path), name(name), mode(0), links(0), size(0),
        modificationTime(0) {}
};

// 文件信息格式化工具类，提供静态方法处理权限、大小和时间
class FileFormatter {
public:
  // 格式化文件权限
  static std::string formatPermissions(mode_t mode) {
    std::string perms;
    perms += (mode & S_IRUSR) ? 'r' : '-'; // 用户读权限
    perms += (mode & S_IWUSR) ? 'w' : '-'; // 用户写权限
    perms += (mode & S_IXUSR) ? 'x' : '-'; // 用户执行权限
    perms += (mode & S_IRGRP) ? 'r' : '-'; // 组读权限
    perms += (mode & S_IWGRP) ? 'w' : '-'; // 组写权限
    perms += (mode & S_IXGRP) ? 'x' : '-'; // 组执行权限
    perms += (mode & S_IROTH) ? 'r' : '-'; // 其他读权限
    perms += (mode & S_IWOTH) ? 'w' : '-'; // 其他写权限
    perms += (mode & S_IXOTH) ? 'x' : '-'; // 其他执行权限
    return perms;
  }

  // 将文件大小格式化为人类可读格式（如 KB、MB 等）
  static std::string formatSize(off_t size) {
    static const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unitIndex = 0;
    double readableSize = static_cast<double>(size);

    // 递归除以 1024，直到找到合适的单位
    while (readableSize >= 1024 && unitIndex < 5) {
      readableSize /= 1024;
      ++unitIndex;
    }

    // 格式化为两位小数，返回字符串
    char formattedSize[20];
    snprintf(formattedSize, sizeof(formattedSize), "%.2f %s", readableSize,
             units[unitIndex]);
    return std::string(formattedSize);
  }

  // 格式化文件修改时间
  static std::string formatTime(time_t rawTime) {
    char timeBuffer[20];
    struct tm *timeInfo = localtime(&rawTime);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M",
             timeInfo); // 格式：YYYY-MM-DD HH:MM
    return std::string(timeBuffer);
  }
};

// 文件类型颜色策略类，根据文件类型返回对应的颜色代码
class FileColorizer {
private:
  // 文件类型与颜色代码的映射
  std::map<mode_t, std::string> typeColors = {
      {S_IFDIR, "34"}, // 目录: 蓝色
      {S_IFLNK, "36"}, // 符号链接: 青色
      {S_IFCHR, "32"}, // 字符设备: 绿色
      {S_IFBLK, "33"}, // 块设备: 黄色
      {S_IFIFO, "35"}, // 管道: 紫色
      {S_IFSOCK, "31"} // 套接字: 红色
  };

public:
  // 根据文件类型为文本着色
  std::string colorize(const std::string &text, mode_t mode) {
    std::string colorCode = "0"; // 默认颜色
    for (const auto &[fileType, color] : typeColors) {
      if ((mode & S_IFMT) == fileType) { // 比较文件类型
        colorCode = color;
        break;
      }
    }
    return "\033[" + colorCode + "m" + text + "\033[0m"; // 添加颜色转义序列
  }
};

// 目录打印器类，负责递归遍历目录并打印文件信息
class DirectoryPrinter {
private:
  FileColorizer colorizer; // 用于文件名着色

  // 从文件系统获取文件信息，填充 FileInfo 结构体
  void fetchFileInfo(const std::string &path, const std::string &filename,
                     FileInfo &fileInfo) {
    std::string fullPath = path + "/" + filename;
    struct stat fileStat;

    // 使用 lstat 获取文件信息（符号链接保留自身信息）
    if (lstat(fullPath.c_str(), &fileStat) == -1) {
      perror("lstat");
      return;
    }

    fileInfo.mode = fileStat.st_mode;
    fileInfo.links = fileStat.st_nlink;

    // 获取所有者名称
    struct passwd *pwd = getpwuid(fileStat.st_uid);
    fileInfo.owner = pwd ? pwd->pw_name : std::to_string(fileStat.st_uid);

    // 获取所属组名称
    struct group *grp = getgrgid(fileStat.st_gid);
    fileInfo.group = grp ? grp->gr_name : std::to_string(fileStat.st_gid);

    fileInfo.size = fileStat.st_size;              // 文件大小
    fileInfo.modificationTime = fileStat.st_mtime; // 修改时间

    // 如果是符号链接，获取其目标路径
    if (S_ISLNK(fileStat.st_mode)) {
      char linkTarget[1024];
      ssize_t len =
          readlink(fullPath.c_str(), linkTarget, sizeof(linkTarget) - 1);
      if (len != -1) {
        linkTarget[len] = '\0';
        fileInfo.linkTarget = linkTarget;
      }
    }
  }

  // 打印单个文件的信息
  void printFileInfo(const FileInfo &fileInfo) {
    std::string fileType(
        1, FileFormatter::formatPermissions(fileInfo.mode)[0]); // 文件类型
    std::string permissions =
        FileFormatter::formatPermissions(fileInfo.mode); // 权限字符串
    std::string size = FileFormatter::formatSize(fileInfo.size); // 格式化大小
    std::string modificationTime =
        FileFormatter::formatTime(fileInfo.modificationTime); // 格式化时间

    std::string name = fileInfo.name; // 文件名
    if (!fileInfo.linkTarget.empty()) {
      name += " -> " + fileInfo.linkTarget; // 符号链接目标
    }

    // 文件名着色
    name = colorizer.colorize(name, fileInfo.mode);

    // 输出文件信息
    std::cout << fileType << permissions << " " << std::setw(3)
              << fileInfo.links << " " << std::setw(8) << fileInfo.owner << " "
              << std::setw(8) << fileInfo.group << " " << std::setw(10) << size
              << " " << modificationTime << " " << name << std::endl;
  }

public:
  // 遍历目录并递归处理子目录
  void listDirectory(const std::string &path) {
    DIR *dir = opendir(path.c_str());
    if (!dir) {
      perror("opendir");
      return;
    }

    struct dirent *entry;
    std::vector<std::string> subdirs; // 存储子目录以便递归

    std::cout << "\n" << path << ":" << std::endl;

    // 遍历当前目录下的每个文件和子目录
    while ((entry = readdir(dir)) != nullptr) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue; // 跳过 . 和 ..
      }

      FileInfo fileInfo(path, entry->d_name); // 初始化文件信息结构
      fetchFileInfo(path, entry->d_name, fileInfo);
      printFileInfo(fileInfo);

      // 如果是子目录，加入待处理列表
      if (S_ISDIR(fileInfo.mode) && entry->d_type == DT_DIR) {
        subdirs.emplace_back(fileInfo.path + "/" + fileInfo.name);
      }
    }

    closedir(dir);

    // 递归进入子目录
    for (const auto &subdir : subdirs) {
      listDirectory(subdir);
    }
  }
};

// 主函数：从指定路径开始打印目录结构
int main(int argc, char *argv[]) {
  const std::string path = (argc > 1) ? argv[1] : ".";
  DirectoryPrinter printer;
  printer.listDirectory(path);
  return 0;
}
