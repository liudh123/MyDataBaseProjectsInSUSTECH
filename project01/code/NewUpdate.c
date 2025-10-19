#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 4096  // 支持长行
#define NUM_COPIES 5

// 解析CSV行，提取3个字段的原始内容（含引号），并确定字段边界
// 返回值：1=解析成功；通过指针传出3个字段（含原始引号）和字段结束位置
int parse_csv_fields(const char *line, char *part_num, char *name, char *part_cat_id, int *end_pos) {
    int in_quotes = 0;       // 是否在引号内
    int field_idx = 0;       // 当前字段索引（0:part_num, 1:name, 2:part_cat_id）
    int start = 0;           // 当前字段的起始位置
    const char *p = line;

    // 初始化输出
    *part_num = *name = *part_cat_id = '\0';
    *end_pos = 0;

    while (*p != '\0' && field_idx < 3) {
        if (*p == '"') {
            in_quotes = !in_quotes;  // 切换引号状态
        } else if (*p == ',' && !in_quotes) {
            // 遇到字段分隔符（不在引号内），提取当前字段
            int len = p - line - start;
            if (len > 0) {
                switch (field_idx) {
                    case 0:
                        strncpy(part_num, line + start, len);
                        part_num[len] = '\0';
                        break;
                    case 1:
                        strncpy(name, line + start, len);
                        name[len] = '\0';
                        break;
                }
            }
            field_idx++;
            start = p - line + 1;  // 下一个字段的起始位置
        }
        p++;
    }

    // 提取最后一个字段（part_cat_id）
    if (field_idx == 2) {
        int len = p - line - start;
        if (len > 0) {
            strncpy(part_cat_id, line + start, len);
            part_cat_id[len] = '\0';
        }
    }

    *end_pos = p - line;  // 记录行结束位置
    // 验证3个字段是否有效（非空）
    return (strlen(part_num) > 0 && strlen(name) > 0 && strlen(part_cat_id) > 0) ? 1 : 0;
}

// 移除字段的外层引号（仅用于判断part_cat_id是否为1，不改变原始输出）
void remove_quotes(const char *src, char *dest) {
    int len = strlen(src);
    if (len >= 2 && src[0] == '"' && src[len-1] == '"') {
        strncpy(dest, src + 1, len - 2);  // 去掉首尾引号
        dest[len - 2] = '\0';
    } else {
        strcpy(dest, src);  // 无引号则直接复制
    }
}

int main() {
    FILE *input_file, *output_file;
    char line[MAX_LINE_LENGTH];
    char part_num[MAX_LINE_LENGTH], name[MAX_LINE_LENGTH], part_cat_id[MAX_LINE_LENGTH];
    char part_cat_id_clean[MAX_LINE_LENGTH];  // 用于判断的去引号版本
    clock_t start, end;
    double duration, total_time = 0.0;
    int end_pos;  // 记录行解析的结束位置

    const char *input_file_path = "D:\\SQLlab\\lego\\data\\parts.csv";

    for (int i = 1; i <= NUM_COPIES; ++i) {
        char output_file_path[MAX_LINE_LENGTH];
        sprintf(output_file_path, "D:\\SQLlab\\lego\\data\\parts_copy%d.csv", i);

        input_file = fopen(input_file_path, "r");
        if (!input_file) {
            perror("无法打开输入文件");
            return 1;
        }
        output_file = fopen(output_file_path, "w");
        if (!output_file) {
            perror("无法打开输出文件");
            fclose(input_file);
            return 1;
        }

        start = clock();
        int line_count = 0, modified_count = 0;

        while (fgets(line, MAX_LINE_LENGTH, input_file)) {
            line_count++;
            // 移除行尾换行符（保留其他字符）
            line[strcspn(line, "\r\n")] = '\0';

            // 解析3个字段（保留原始引号）
            if (!parse_csv_fields(line, part_num, name, part_cat_id, &end_pos)) {
                // 解析失败时，直接写入原始行（完全不修改）
                fprintf(output_file, "%s\n", line);
                continue;
            }

            // 清理part_cat_id的引号（仅用于判断是否为1）
            remove_quotes(part_cat_id, part_cat_id_clean);

            if (strcmp(part_cat_id_clean, "1") == 0) {
                // 处理需要修改的行：
                // 1. part_num添加前缀"new_"（保留原始引号）
                // 2. name保持原始格式（含引号）
                // 3. part_cat_id改为"100"（若原字段有引号，保留引号格式）
                char new_part_num[MAX_LINE_LENGTH];
                if (part_num[0] == '"') {
                    // 原始part_num带引号，在引号内添加前缀（如"123" → "new_123"）
                    sprintf(new_part_num, "\"new_%s", part_num + 1);
                } else {
                    // 原始part_num无引号，直接添加前缀（如123 → new_123）
                    sprintf(new_part_num, "new_%s", part_num);
                }

                char new_cat_id[MAX_LINE_LENGTH];
                if (part_cat_id[0] == '"') {
                    // 原始part_cat_id带引号，新值也带引号（如"1" → "100"）
                    strcpy(new_cat_id, "\"100\"");
                } else {
                    // 原始part_cat_id无引号，新值也不带引号（如1 → 100）
                    strcpy(new_cat_id, "100");
                }

                // 写入修改后的行（保留所有引号格式）
                fprintf(output_file, "%s,%s,%s\n", new_part_num, name, new_cat_id);
                modified_count++;
            } else {
                // 不需要修改的行，完全保留原始格式（包括所有引号和逗号）
                fprintf(output_file, "%s\n", line);
            }
        }

        end = clock();
        duration = (double)(end - start) / CLOCKS_PER_SEC;
        total_time += duration;

        printf("生成文件 %s 完成 | 总行数：%d | 修改行数：%d | 耗时：%.4f 秒\n",
               output_file_path, line_count, modified_count, duration);

        fclose(input_file);
        fclose(output_file);
    }

    printf("\n所有文件生成完成 | 总耗时：%.4f 秒 | 平均耗时：%.4f 秒\n",
           total_time, total_time / NUM_COPIES);
    return 0;
}