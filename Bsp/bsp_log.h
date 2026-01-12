//
// Created by 薛斌 on 24-8-16.
//

#ifndef BSP_LOG_H
#define BSP_LOG_H

#include <stdio.h>

// 定义日志级别
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

// 当前日志级别
#define LOG_LEVEL LOG_LEVEL_DEBUG

// 定义LOG宏，支持可变参数
#define LOG(level, fmt, ...) \
do { \
if (level <= LOG_LEVEL) { \
fprintf(stderr, "[%s:%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} \
} while (0)

// 定义常用的日志级别宏
#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, "ERROR: " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG(LOG_LEVEL_WARN,  "WARN: "  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG(LOG_LEVEL_INFO,  "INFO: "  fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, "DEBUG: " fmt, ##__VA_ARGS__)

#endif //BSP_LOG_H
