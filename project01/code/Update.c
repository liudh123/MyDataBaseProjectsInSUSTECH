#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024
#define DELIMITER ','
#define NUM_COPIES 5

int main() {
    FILE *input_file, *output_file;
    char line[MAX_LINE_LENGTH];
    char *token;
    char part_num[MAX_LINE_LENGTH];
    char name[MAX_LINE_LENGTH];
    char part_cat_id[MAX_LINE_LENGTH];
    clock_t start, end;
    double duration;

    // 输入文件路径
    const char *input_file_path = "D:\\SQLlab\\lego\\data\\parts.csv";

    for (int i = 1; i <= NUM_COPIES; ++i) {
        // 构建输出文件路径
        char output_file_path[MAX_LINE_LENGTH];
        sprintf(output_file_path, "D:\\SQLlab\\lego\\data\\parts_copy%d.csv", i);

        // 打开输入文件
        input_file = fopen(input_file_path, "r");
        if (input_file == NULL) {
            perror("无法打开输入文件");
            return 1;
        }

        // 打开输出文件
        output_file = fopen(output_file_path, "w");
        if (output_file == NULL) {
            perror("无法打开输出文件");
            fclose(input_file);
            return 1;
        }

        start = clock();

        // 逐行读取输入文件
        while (fgets(line, MAX_LINE_LENGTH, input_file)!= NULL) {
            // 移除换行符
            line[strcspn(line, "\r\n")] = '\0';

            // 解析字段
            token = strtok(line, ",");
            if (token!= NULL) {
                strcpy(part_num, token);
                token = strtok(NULL, ",");
                if (token!= NULL) {
                    strcpy(name, token);
                    token = strtok(NULL, ",");
                    if (token!= NULL) {
                        strcpy(part_cat_id, token);

                        // 检查最后一列是否为1
                        if (strcmp(part_cat_id, "1") == 0) {
                            // 在第一项加前缀“new_”
                            char new_part_num[MAX_LINE_LENGTH];
                            sprintf(new_part_num, "new_%s", part_num);

                            // 将最后一列修改为100
                            fprintf(output_file, "%s,%s,100\n", new_part_num, name);
                        } else {
                            // 不需要修改的行直接写入
                            fprintf(output_file, "%s,%s,%s\n", part_num, name, part_cat_id);
                        }
                    }
                }
            }
        }

        end = clock();
        duration = ((double)(end - start)) / CLOCKS_PER_SEC;

        // 关闭文件
        fclose(input_file);
        fclose(output_file);

        printf("生成文件 %s 完成，耗时: %.4f 秒\n", output_file_path, duration);
    }
}