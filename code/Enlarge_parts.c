#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LEN 1024  // 每行最大长度
#define COPY_TIMES 25      // 复制次数（25次）
#define PREFIX_LEN 3       // 前缀长度（如 "01-" 含2位数字+1个短横线）

// 读取原始CSV并生成扩充数据
int expand_csv(const char *input_path, const char *output_path) {
    // 打开原始文件
    FILE *infile = fopen(input_path, "r");
    if (!infile) {
        fprintf(stderr, "无法打开原始文件 %s：%s\n", input_path, strerror(errno));
        return -1;
    }

    // 打开输出文件
    FILE *outfile = fopen(output_path, "w");
    if (!outfile) {
        fprintf(stderr, "无法创建输出文件 %s：%s\n", output_path, strerror(errno));
        fclose(infile);
        return -1;
    }

    char line[MAX_LINE_LEN];
    // 读取并写入表头（仅写一次）
    if (fgets(line, MAX_LINE_LEN, infile) == NULL) {
        fprintf(stderr, "原始文件为空或读取表头失败\n");
        fclose(infile);
        fclose(outfile);
        return -1;
    }
    fputs(line, outfile);  // 写入表头

    // 循环复制30次数据，每次添加不同前缀
    for (int i = 1; i <= COPY_TIMES; i++) {
        // 生成前缀（01- 到 30-）
        char prefix[PREFIX_LEN + 1];  // 包含结束符 '\0'
        snprintf(prefix, PREFIX_LEN + 1, "%02d-", i);  // %02d 确保两位数字（01, 02...30）

        // 重置文件指针到数据行开头（跳过表头）
        rewind(infile);
        fgets(line, MAX_LINE_LEN, infile);  // 跳过表头（已写入输出文件）

        // 读取原始数据行并添加前缀
        while (fgets(line, MAX_LINE_LEN, infile)) {
            // 处理空行（跳过）
            if (line[0] == '\n' || line[0] == '\r') continue;

            // 查找第一列分隔符（假设用逗号分隔）
            char *first_comma = strchr(line, ',');
            if (!first_comma) {
                // 无分隔符，视为无效行，直接写入（不添加前缀）
                fputs(line, outfile);
                continue;
            }

            // 计算第一列长度和剩余内容长度
            int first_col_len = first_comma - line;
            int rest_len = strlen(first_comma);  // 包含逗号和后续内容

            // 拼接前缀 + 第一列 + 剩余内容
            char new_line[MAX_LINE_LEN + PREFIX_LEN];  // 预留前缀空间
            // 复制前缀
            strncpy(new_line, prefix, PREFIX_LEN);
            // 复制第一列
            strncpy(new_line + PREFIX_LEN, line, first_col_len);
            // 复制剩余内容（逗号及之后）
            strncpy(new_line + PREFIX_LEN + first_col_len, first_comma, rest_len + 1);  // +1 包含结束符

            // 写入新行
            fputs(new_line, outfile);
        }
    }

    // 清理资源
    fclose(infile);
    fclose(outfile);
    printf("数据扩充完成！输出文件：%s\n", output_path);
    return 0;
}

int main() {
    // 原始CSV路径和输出路径（根据实际情况修改）
    const char *input_csv = "D:\\SQLlab\\lego\\data\\parts.csv";       // 原始文件
    const char *output_csv = "D:\\SQLlab\\lego\\data\\expanded_parts.csv";  // 扩充后文件

    // 执行扩充
    if (expand_csv(input_csv, output_csv) != 0) {
        printf("数据扩充失败\n");
        return 1;
    }

    return 0;
}