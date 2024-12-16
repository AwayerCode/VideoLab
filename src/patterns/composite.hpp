#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct FileInfo {
    FileInfo() = default;
    FileInfo(const std::string& name, const std::string& type) : name(name), type(type) {}
    
    std::string name;
    std::string type;
};

// 基类
class FileBase {
public:
    virtual ~FileBase() = default;
    virtual void display(int level = 0) const = 0;  // 添加纯虚函数
    virtual bool isFolder() const = 0;  // 添加类型判断
    virtual void add(std::shared_ptr<FileBase> file) {}  // 默认实现
    virtual void remove(std::shared_ptr<FileBase> file) {}  // 默认实现
    
    void setInfo(const FileInfo& info) {
        this->info = info;
    }

    FileInfo getInfo() const {
        return info;
    }

protected:
    FileInfo info;
};

// 文件类
class File : public FileBase {
public:
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "- " << info.name 
                 << " (" << info.type << ")" << std::endl;
    }
    
    bool isFolder() const override { return false; }
};

// 文件夹类
class Folder : public FileBase {
public:
    void add(std::shared_ptr<FileBase> file) override {
        files.push_back(file);
    }
    
    void remove(std::shared_ptr<FileBase> file) override {
        files.erase(
            std::remove(files.begin(), files.end(), file),
            files.end()
        );
    }
    
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "+ " << info.name 
                 << " (" << info.type << ")" << std::endl;
        
        for (const auto& file : files) {
            file->display(level + 1);
        }
    }
    
    bool isFolder() const override { return true; }

private:
    std::vector<std::shared_ptr<FileBase>> files;
};

// 测试函数
void testComposite() {
    auto root = std::make_shared<Folder>();
    root->setInfo(FileInfo("root", "folder"));

    auto folder1 = std::make_shared<Folder>();
    folder1->setInfo(FileInfo("folder1", "folder"));
    
    auto file1 = std::make_shared<File>();
    file1->setInfo(FileInfo("file1.txt", "text"));
    auto file2 = std::make_shared<File>();
    file2->setInfo(FileInfo("file2.mp3", "audio"));
    
    folder1->add(file1);
    folder1->add(file2);
    root->add(folder1);
    
    auto file3 = std::make_shared<File>();
    file3->setInfo(FileInfo("file3.jpg", "image"));
    root->add(file3);
    
    // 显示整个文件结构
    root->display();
    
    // 测试移除功能
    folder1->remove(file1);
    std::cout << "\nAfter removing file1:\n";
    root->display();
}