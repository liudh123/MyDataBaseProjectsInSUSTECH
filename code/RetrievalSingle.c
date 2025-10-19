#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

typedef struct {
    int inventory_id;
    char part_num[50];
    int color_id;
    int quantity;
    char is_spare;
} InventoryPart;

void trim(char* str) {
    if (str == NULL || *str == '\0') return;
    int len = strlen(str);
    int start = 0, end = len - 1;

    while (start <= end && (isspace((unsigned char)str[start]) || str[start] == '"')) {
        start++;
    }
    while (end >= start && (isspace((unsigned char)str[end]) || str[end] == '"')) {
        end--;
    }

    if (start > end) {
        str[0] = '\0';
    } else {
        memmove(str, str + start, end - start + 1);
        str[end - start + 1] = '\0';
    }
}

int read_inventory_from_csv(const char* filename, InventoryPart**parts, int* capacity, double* read_time) {
    clock_t start = clock();

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("无法打开inventory_parts.csv文件");
        *read_time = 0;
        return -1;
    }

    *capacity = 100;
    *parts = (InventoryPart*)malloc(*capacity * sizeof(InventoryPart));
    if (!*parts) {
        perror("内存分配失败");
        fclose(file);
        *read_time = 0;
        return -1;
    }

    char line[1024];
    int count = 0;
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "CSV文件为空\n");
        fclose(file);
        *read_time = 0;
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (count >= *capacity) {
            *capacity *= 2;
            InventoryPart* temp = (InventoryPart*)realloc(*parts, *capacity * sizeof(InventoryPart));
            if (!temp) {
                perror("内存扩容失败");
                fclose(file);
                *read_time = 0;
                return -1;
            }
            *parts = temp;
        }

        InventoryPart* part = &(*parts)[count];
        char* token;
        int field_idx = 0;

        token = strtok(line, ",");
        while (token != NULL && field_idx < 5) {
            trim(token);
            switch (field_idx) {
                case 0:
                    part->inventory_id = atoi(token);
                    break;
                case 1:
                    strncpy(part->part_num, token, sizeof(part->part_num) - 1);
                    part->part_num[sizeof(part->part_num) - 1] = '\0';
                    break;
                case 2:
                    part->color_id = atoi(token);
                    break;
                case 3:
                    part->quantity = atoi(token);
                    break;
                case 4:
                    part->is_spare = (strcmp(token, "t") == 0) ? 't' : 'f';
                    break;
            }
            field_idx++;
            token = strtok(NULL, ",");
        }
        count++;
    }

    fclose(file);
    *read_time = (double)(clock() - start) / CLOCKS_PER_SEC * 1000;
    return count;
}

void export_spare_parts(InventoryPart* parts, int count, const char* txt_filename, double* export_time) {
    clock_t start = clock();

    FILE* file = fopen(txt_filename, "w");
    if (!file) {
        perror("无法创建输出TXT文件");
        *export_time = 0;
        return;
    }

    fprintf(file, "inventory_id,part_num,color_id,quantity\n");

    int spare_count = 0;
    for (int i = 0; i < count; i++) {
        if (parts[i].is_spare == 't') {
            fprintf(file, "%d,%s,%d,%d\n",
                    parts[i].inventory_id,
                    parts[i].part_num,
                    parts[i].color_id,
                    parts[i].quantity);
            spare_count++;
        }
    }



    fclose(file);
    *export_time = (double)(clock() - start) / CLOCKS_PER_SEC * 1000;
}

int main() {
    const int TEST_COUNT = 10;  // 测试次数
    double total_times[TEST_COUNT];  // 存储每次总耗时（读取+导出）
    double avg_time = 0.0;

    // 循环执行10次测试（每次都重新读取CSV）
    for (int i = 0; i < TEST_COUNT; i++) {
        char filename[50];
        InventoryPart* inventory_parts = NULL;  // 每次测试重新分配内存
        int part_count, capacity;
        double read_time, export_time;

        // 生成文件名：spare_parts10.txt 到 spare_parts19.txt
        sprintf(filename, "D:\\SQLlab\\lego\\outputs\\spare_parts%d.txt", 10 + i);

        printf("===== 测试 %d/%d 开始 =====", i + 1, TEST_COUNT);
        
        // 1. 重新读取CSV并记录时间（每次测试都执行）
        printf("开始读取inventory_parts.csv...\n");
        part_count = read_inventory_from_csv("D:\\SQLlab\\lego\\data\\inventory_parts.csv", &inventory_parts, &capacity, &read_time);
        if (part_count <= 0) {
            if (inventory_parts) free(inventory_parts);
            return 1;
        }
        printf("CSV读取完成：共读取 %d 条记录，耗时 %.2f 毫秒\n", part_count, read_time);

        // 2. 导出TXT并记录时间
        printf("开始导出空闲零件...\n");
        export_spare_parts(inventory_parts, part_count, filename, &export_time);
        printf("导出处理耗时：%.2f 毫秒\n", export_time);

        // 3. 计算本次总耗时（读取+导出）
        total_times[i] = read_time + export_time;
        printf("测试 %d 总耗时：%.2f 毫秒\n\n", i + 1, total_times[i]);

        // 释放本次测试的内存（避免累计占用）
        free(inventory_parts);
        
        // 累加用于计算平均值
        avg_time += total_times[i];
    }

    // 计算并输出平均耗时
    avg_time /= TEST_COUNT;
    printf("===== 测试完成 =====");
    printf("10次测试总耗时分别为：\n");
    for (int i = 0; i < TEST_COUNT; i++) {
        printf("测试 %d：%.2f 毫秒\n", i + 1, total_times[i]);
    }
    printf("平均耗时：%.2f 毫秒\n", avg_time);

    return 0;
}