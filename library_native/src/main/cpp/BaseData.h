//
// Created by dell on 2025/10/22/周三.
//

#ifndef FSP2PNATIVE_BASEDATA_H
#define FSP2PNATIVE_BASEDATA_H

#include <jni.h>
#include <string>
#include <map>
#include <memory>
#include <android/log.h>
#include <nlohmann/json.hpp>
using namespace nlohmann;

struct BaseData {
    int iPutType;
    std::string iid;
    std::string operation;
    std::map<std::string, ordered_json> maps;;  // 使用 shared_ptr 可以根据实际需要改动
};



#endif //FSP2PNATIVE_BASEDATA_H
