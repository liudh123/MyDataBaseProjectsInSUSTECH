#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024  // 每行最大长度
#define FILE_COUNT 20         // 要比较的文件数量
#define HASH_TABLE_SIZE 100000 // 哈希表大小（根据数据量调整）

// 哈希表节点结构
typedef struct HashNode {
    char* record;             // 存储一条数据记录
    struct HashNode* next;    // 链表解决哈希冲突
} HashNode;

// 创建哈希表
HashNode**create_hash_table() {
    HashNode** table = (HashNode**)calloc(HASH_TABLE_SIZE, sizeof(HashNode*));
    if (!table) {
        perror("哈希表内存分配失败");
        exit(EXIT_FAILURE);
    }
    return table;
}

// 哈希函数
unsigned int hash(const char* str) {
    unsigned int hash_val = 0;
    while (*str) {
        hash_val = (hash_val << 5) - hash_val + *str++; // 简单哈希算法
    }
    return hash_val % HASH_TABLE_SIZE;
}

// 向哈希表插入记录（去重）
void insert_record(HashNode**table, const char* record) {
    if (!record || *record == '\0') return;

    unsigned int index = hash(record);
    HashNode* current = table[index];

    // 检查是否已存在
    while (current) {
        if (strcmp(current->record, record) == 0) {
            return; // 已存在则不重复插入
        }
        current = current->next;
    }

    // 插入新记录
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (!new_node) {
        perror("节点内存分配失败");
        exit(EXIT_FAILURE);
    }
    new_node->record = (char*)malloc(strlen(record) + 1);
    if (!new_node->record) {
        perror("记录内存分配失败");
        free(new_node);
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->record, record);
    new_node->next = table[index];
    table[index] = new_node;
}

// 从哈希表查找记录
int find_record(HashNode**table, const char* record) {
    if (!record || *record == '\0') return 0;

    unsigned int index = hash(record);
    HashNode* current = table[index];

    while (current) {
        if (strcmp(current->record, record) == 0) {
            return 1; // 找到记录
        }
        current = current->next;
    }
    return 0; // 未找到
}

// 计算哈希表中的记录数量
int get_record_count(HashNode**table) {
    int count = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* current = table[i];
        while (current) {
            count++;
            current = current->next;
        }
    }
    return count;
}

// 释放哈希表内存
void free_hash_table(HashNode**table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* current = table[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            free(temp->record);
            free(temp);
        }
    }
    free(table);
}

// 读取文件中的有效数据记录（忽略表头和统计行），存储到哈希表
int load_records(const char* filename, HashNode**table) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("无法打开文件");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int is_header = 1; // 标记表头行

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        // 处理换行符和首尾空白
        line[strcspn(line, "\r\n")] = '\0';
        int len = strlen(line);
        while (len > 0 && isspace((unsigned char)line[len - 1])) {
            len--;
        }
        line[len] = '\0';

        // 忽略空行
        if (len == 0) continue;

        // 忽略表头行（第一行）
        if (is_header) {
            is_header = 0;
            continue;
        }

        // 忽略统计行（含"总计："或分隔线）
        if (strstr(line, "总计：") != NULL || strstr(line, "---") != NULL) {
            continue;
        }

        // 插入有效数据记录
        insert_record(table, line);
    }

    fclose(file);
    return get_record_count(table);
}

// 比较所有文件的记录集合是否相同（忽略顺序）
int compare_files() {
    // 生成文件名（spare_parts10.txt 到 spare_parts19.txt）
    char* filenames[FILE_COUNT];
    for (int i = 0; i < FILE_COUNT; i++) {
        filenames[i] = (char*)malloc(30 * sizeof(char));
        sprintf(filenames[i], "D:\\SQLlab\\lego\\outputs\\spare_parts%d.txt", 10 + i);
    }

    // 读取第一个文件作为基准
    HashNode**base_table = create_hash_table();
    int base_count = load_records(filenames[0], base_table);
    if (base_count <= 0) {
        printf("基准文件 %s 中未读取到有效记录\n", filenames[0]);
        free_hash_table(base_table);
        for (int i = 0; i < FILE_COUNT; i++) free(filenames[i]);
        return -1;
    }
    printf("基准文件 %s 读取完成，有效记录数：%d\n", filenames[0], base_count);

    // 依次比较其他文件
    int all_same = 1;
    for (int i = 1; i < FILE_COUNT; i++) {
        HashNode**curr_table = create_hash_table();
        int curr_count = load_records(filenames[i], curr_table);

        if (curr_count <= 0) {
            printf("文件 %s 中未读取到有效记录\n", filenames[i]);
            all_same = 0;
            free_hash_table(curr_table);
            break;
        }

        // 先比较记录数量
        if (curr_count != base_count) {
            printf("文件 %s 与基准文件记录数量不同（%d vs %d）\n", 
                   filenames[i], curr_count, base_count);
            all_same = 0;
            free_hash_table(curr_table);
            break;
        }

        // 检查当前文件的所有记录是否都在基准文件中
        int match = 1;
        for (int j = 0; j < HASH_TABLE_SIZE; j++) {
            HashNode* current = curr_table[j];
            while (current) {
                if (!find_record(base_table, current->record)) {
                    printf("文件 %s 包含额外记录：%s\n", filenames[i], current->record);
                    match = 0;
                    break;
                }
                current = current->next;
            }
            if (!match) break;
        }

        if (!match) {
            all_same = 0;
            free_hash_table(curr_table);
            break;
        }

        printf("文件 %s 与基准文件记录集合一致（%d 条记录）\n", filenames[i], curr_count);
        free_hash_table(curr_table);
    }

    // 输出最终结果
    if (all_same) {
        printf("\n所有文件的有效数据记录集合完全相同（忽略顺序）\n");
    } else {
        printf("\n文件记录集合存在差异\n");
    }

    // 清理资源
    free_hash_table(base_table);
    for (int i = 0; i < FILE_COUNT; i++) {
        free(filenames[i]);
    }
    return all_same ? 0 : 1;
}

int main() {
    clock_t start = clock();
    int result = compare_files();
    double time = (double)(clock() - start) / CLOCKS_PER_SEC * 1000;

    printf("\n比较完成，耗时：%.2f 毫秒\n", time);
    return result;
}