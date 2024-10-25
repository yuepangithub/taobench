#include "memcache.h"
#include <iostream>

namespace benchmark {

MemcachedClient::MemcachedClient(const std::vector<std::pair<std::string, in_port_t>> &servers) {
    // 创建一个 Memcached 实例
    memc = memcached_create(nullptr);

    // 初始化服务器列表
    memcached_server_st *server_list = nullptr;

    // 循环遍历每个服务器并将其添加到服务器列表中
    for (const auto &server : servers) {
        const std::string &server_address = server.first;
        in_port_t port = server.second;
        server_list = memcached_server_list_append(server_list, server_address.c_str(), port, nullptr);
    }

    // 将服务器列表推送到 Memcached 实例
    memcached_server_push(memc, server_list);

    // 释放服务器列表
    memcached_server_free(server_list);
}

MemcachedClient::~MemcachedClient() {
    memcached_free(memc);
}

bool MemcachedClient::get(const DB::DB_Operation &operation, std::vector<DB::TimestampValue> &buffer) {
    assert(operation.operation == Operation::READ);
    std::string key = fields2Str(operation.key);
    std::string rsl = readValue(key);
    if (rsl == "") {
        return false;
    }
    DB::TimestampValue value = str2Timeval(rsl);
    buffer.push_back(value);
    return true;
}

bool MemcachedClient::put(const DB::DB_Operation &operation, std::vector<DB::TimestampValue> &buffer) {
    assert(operation.operation == Operation::READ);
    std::string key = fields2Str(operation.key);
    std::string value = timeval2Str(buffer[buffer.size()-1]);
    return storeValue(key, value);
}

bool MemcachedClient::invalidate(const DB::DB_Operation &operation) {
    std::string key = fields2Str(operation.key);
    return deleteValue(key);
}

std::string MemcachedClient::fields2Str(std::vector<DB::Field> const & k) {
    std::ostringstream oss;
    for (size_t i = 0; i < k.size(); i++) {
        oss << k[i].value;
        if (i != k.size()-1) {
            oss << ",";
        }
    }
    return oss.str();
}

std::string MemcachedClient::timeval2Str(DB::TimestampValue const & tv) {
    return std::to_string(tv.timestamp) + "," + tv.value;
}

DB::TimestampValue MemcachedClient::str2Timeval(const std::string &str) {
    size_t pos = str.find(',');
    return {std::stoll(str.substr(0, pos)), str.substr(pos+1)};
}

bool MemcachedClient::storeValue(const std::string &key, const std::string &value, time_t expiration) {
    memcached_return_t rc = memcached_set(memc, key.c_str(), key.length(), value.c_str(), value.length(), expiration, 0);
    return rc == MEMCACHED_SUCCESS;
}

std::string MemcachedClient::readValue(const std::string &key) {
    size_t value_length;
    uint32_t flags;
    memcached_return_t rc;
    
    char *value = memcached_get(memc, key.c_str(), key.length(), &value_length, &flags, &rc);
    
    if (rc == MEMCACHED_SUCCESS) {
        std::string result(value, value_length);
        free(value); // Free the retrieved value
        return result;
    } else {
        return ""; // Return empty string on failure
    }
}

bool MemcachedClient::deleteValue(const std::string &key) {
    memcached_return_t rc = memcached_delete(memc, key.c_str(), key.length(), 0);
    return rc == MEMCACHED_SUCCESS;
}

} // benchmark
