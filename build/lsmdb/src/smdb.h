#pragma once
#include <string>
#include <unordered_map>

#include "zstd.h"

#if defined(WIN32) | defined(_GAMING_XBOX) 
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#define fileno _fileno
#define ftruncate _chsize_s
#define filelength _filelength
#else
#include <climits>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
long filelength(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}
#endif

using namespace std;
using sptr = shared_ptr<string>;

namespace smdb {
     const uint32_t MAX_VAL_SIZE    = 0xffffff; //值最大长度 16M-1
     const uint32_t MEM_GROW_SIZE   = 0x100000; //单次内存分配 1M

    struct dbheader {
        char fmt[4] = {};               //文件格式标志位
        uint32_t filesize = 0;          //当前文件大小
        uint32_t kvnum = 0;             //当前kv数量
    };

    class smdb {
    public:
        bool open(const char* path) {
            m_values.clear();
            m_indexs.clear();
            auto file = fopen(path, "rb");
            if (!file) return true;
            bool ok = read(file);
            fclose(file);
            return ok;
        }

        void close() {
            m_values.clear();
            m_indexs.clear();
        }

        bool flush(const char* path) {
            size_t offset = 0;
            size_t alloc_size = MEM_GROW_SIZE;
            char* buf = (char*)malloc(alloc_size);
            if (!buf) return false;
            for (auto& [key, val] : m_values) {
                uint8_t ksz = key.size();
                uint32_t vsz = val->size();
                uint32_t size = (ksz << 24) | (vsz & MAX_VAL_SIZE);
                size_t grow = ksz + vsz + sizeof(uint32_t);
                if (offset + grow > alloc_size) {
                    alloc_size += (grow < MEM_GROW_SIZE ? MEM_GROW_SIZE : grow);
                    buf = (char*)realloc(buf, alloc_size);
                    if (!buf) return false;
                }
                memcpy(buf + offset, &size, sizeof(uint32_t));
                memcpy(buf + offset + sizeof(uint32_t), key.c_str(), ksz);
                memcpy(buf + offset + sizeof(uint32_t) + ksz, val->c_str(), vsz);
                offset += grow;
            }
            bool ok = compress(path, buf, offset);
            free(buf);
            return ok;
        }

        void indexs(string& index, vector<string_view>& vec) {
            auto it = m_indexs.find(index);
            if (it != m_indexs.end()) {
                for (auto val : it->second) {
                    vec.push_back(*val.get());
                }
            }
        }

        string_view get(string& key) {
            auto it = m_values.find(key);
            if (it == m_values.end()) return "";
            return it->second->c_str();
        }

        bool put(string& key, string_view val) {
            if (key.size() > UCHAR_MAX || val.size() > MAX_VAL_SIZE) return false;
            auto it = m_values.find(key);
            if (it != m_values.end()) {
                string sval = string(val.data(), val.size());
                it->second->swap(sval);
            } else {
                auto sval = make_shared<string>(val);
                m_values.emplace(key, sval);
                size_t pos = key.find_first_of(":");
                if (pos != string::npos && pos > 0) {
                    string sheet = key.substr(0, pos);
                    auto iti = m_indexs.find(sheet);
                    if (iti == m_indexs.end()) {
                        m_indexs.emplace(sheet, set<sptr>{ sval });
                    } else {
                        iti->second.insert(sval);
                    }
                }
            }
            return true;
        }

        void del(string& key) {
            auto oh = m_values.extract(key);
            if (!oh.empty()) {
                size_t pos = key.find_first_of(":");
                if (pos != string::npos && pos > 0) {
                    string sheet = key.substr(0, pos);
                    auto it = m_indexs.find(sheet);
                    if (it != m_indexs.end()) {
                        it->second.erase(oh.mapped());
                    }
                }
            }
        }

        bool first(string& key, string& val) {
            m_iter = m_values.begin();
            if (m_iter == m_values.end()) return false;
            key = m_iter->first;
            val = *m_iter->second.get();
            return true;
        }

        bool next(string& key, string& val) {
            m_iter++;
            if (m_iter != m_values.end()) {
                key = m_iter->first;
                val = *m_iter->second.get();
                return true;
            }
            return false;
        }

    protected:
        bool save(const char* path, char* buf, size_t size) {
            auto file = fopen(path, "wb+");
            if (!file) return false;
            //更改文件大小
            size_t fsize = sizeof(dbheader) + size;
            ftruncate(fileno(file), fsize);
            //写入文件
            fseek(file, 0, SEEK_SET);
            dbheader header{ { 'S', 'M', 'D', 'B' } };
            header.filesize = size;
            header.kvnum = m_values.size();
            memcpy(buf, &header, sizeof(dbheader));
            fwrite(buf, 1, fsize, file);
            fflush(file);
            fclose(file);
            commit();
            return true;
        }

        bool read(FILE* file) {
            dbheader header;
            int fd = fileno(file);
            size_t fsize = filelength(fd);
            if (fread(&header, 1, sizeof(dbheader), file) < 0) return false;
            size_t zsize = fsize - sizeof(dbheader);
            if (header.filesize != zsize) return false;
            char* zbuf = (char*)malloc(zsize);
            if (!zbuf) return false;
            fseek(file, sizeof(dbheader), SEEK_SET);
            if (fread(zbuf, 1, zsize, file) < 0) return false;
            bool ok = decompress(zbuf, zsize, header.kvnum);
            free(zbuf);
            return ok;
        }
        
        bool compress(const char* path, char* buf, size_t size) {
            size_t zsize = ZSTD_compressBound(size);
            if (ZSTD_isError(zsize)) return false;
            size_t header_len = sizeof(dbheader);
            char* zbuf = (char*)malloc(zsize + header_len);
            size_t comp_ize = ZSTD_compress(zbuf + header_len, zsize, buf, size, ZSTD_defaultCLevel());
            if (ZSTD_isError(comp_ize)) {
                free(zbuf);
                return false;
            }
            bool ok = save(path, zbuf, comp_ize);
            free(zbuf);
            return ok;
        }

        bool decompress(char* zbuf, size_t zsize, size_t num) {
            size_t size = ZSTD_getFrameContentSize(zbuf, zsize);
            if (ZSTD_isError(size)) return false;
            char* buf = (char*)malloc(size);
            size_t dec_size = ZSTD_decompress(buf, size, zbuf, zsize);
            if (ZSTD_isError(dec_size)) {
                free(buf);
                return false;
            }
            int ok = parse(buf, size, num);
            free(buf);
            return ok;
        }

        bool parse(char* zbuf, size_t zsize, size_t num) {
            size_t offset = 0;
            for (size_t i = 0; i < num; ++i) {
                if (offset + sizeof(uint32_t) > zsize)  return false;
                uint32_t size = *(uint32_t*)(zbuf + offset);
                uint8_t ksz = size >> 24;
                uint32_t vsz = size & MAX_VAL_SIZE;
                if (offset + ksz + vsz + sizeof(uint32_t) > zsize)  return false;
                string key = string(zbuf + offset + sizeof(uint32_t), ksz);
                put(key, string_view(zbuf + offset + sizeof(uint32_t) + ksz, vsz));
                offset += (ksz + vsz + sizeof(uint32_t));
            }
            return true;
        }

        void commit() {

        }

    protected:
        unordered_map<string, sptr> m_values;           //kv列表
        unordered_map<string, set<sptr>> m_indexs;      //索引列表
        unordered_map<string, sptr>::iterator m_iter;   //KEY索引迭代器
    };
}
