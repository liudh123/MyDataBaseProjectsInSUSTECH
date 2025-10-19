#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>  // 用于错误信息
#include <time.h>

#define MAX_LINE_LEN 1024  // 每行最大长度

// 定义各表的数据结构（保持不变）
typedef struct {
    char set_num[50];
    char name[100];
    int year;
    int theme_id;
} Set;

typedef struct {
    int id;
    char name[100];
    int parent_id;
} Theme;

typedef struct {
    int id;
    int version;
    char set_num[50];
} Inventory;

typedef struct {
    int inventory_id;
    char part_num[50];
    int color_id;
    int quantity;
    char is_spare[5];
} InventoryPart;

typedef struct {
    int id;
    char name[50];
    char rgb[20];
    char is_trans[10];
} Color;

// 结果集结构（保持不变）
typedef struct {
    char set_num[50];
    char set_name[100];
    int publish_year;
    char theme_name[100];
    char part_id[50];
    int inventory_quantity;
} Result;

// 动态读取CSV到Set数组（返回实际记录数，数组地址通过指针传出）
int readSets(Set **sets, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s，错误原因：%s\n", filename, strerror(errno));
        *sets = NULL;
        return 0;
    }

    char line[MAX_LINE_LEN];
    int count = 0;          // 当前记录数
    int capacity = 100;     // 初始容量（可按需调整）
    *sets = (Set*)malloc(capacity * sizeof(Set));  // 初始分配
    if (!*sets) {
        printf("内存分配失败\n");
        fclose(file);
        return 0;
    }

    // 跳过表头
    if (fgets(line, MAX_LINE_LEN, file) == NULL) {
        fclose(file);
        return 0;
    }

    // 读取数据行（动态扩展容量）
    while (fgets(line, MAX_LINE_LEN, file)) {
        // 移除行尾换行符
        line[strcspn(line, "\r\n")] = '\0';

        // 若容量不足，扩展为原来的2倍（避免频繁分配）
        if (count >= capacity) {
            capacity *= 2;
            Set *temp = (Set*)realloc(*sets, capacity * sizeof(Set));
            if (!temp) {
                printf("内存扩展失败，已读取 %d 条记录\n", count);
                break;
            }
            *sets = temp;  // 指向新地址
        }

        // 解析字段（与之前逻辑一致）
        char *token = strtok(line, ",");
        if (token) strcpy((*sets)[count].set_num, token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*sets)[count].name, token);
        
        token = strtok(NULL, ",");
        if (token) (*sets)[count].year = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) (*sets)[count].theme_id = atoi(token);
        
        count++;
    }

    fclose(file);
    return count;
}

// 动态读取CSV到Theme数组（逻辑同上）
int readThemes(Theme **themes, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s，错误原因：%s\n", filename, strerror(errno));
        *themes = NULL;
        return 0;
    }

    char line[MAX_LINE_LEN];
    int count = 0;
    int capacity = 100;
    *themes = (Theme*)malloc(capacity * sizeof(Theme));
    if (!*themes) {
        printf("内存分配失败\n");
        fclose(file);
        return 0;
    }

    // 跳过表头
    if (fgets(line, MAX_LINE_LEN, file) == NULL) {
        fclose(file);
        return 0;
    }

    while (fgets(line, MAX_LINE_LEN, file)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (count >= capacity) {
            capacity *= 2;
            Theme *temp = (Theme*)realloc(*themes, capacity * sizeof(Theme));
            if (!temp) {
                printf("内存扩展失败，已读取 %d 条记录\n", count);
                break;
            }
            *themes = temp;
        }

        char *token = strtok(line, ",");
        if (token) (*themes)[count].id = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*themes)[count].name, token);
        
        token = strtok(NULL, ",");
        if (token) (*themes)[count].parent_id = atoi(token);
        
        count++;
    }

    fclose(file);
    return count;
}

// 动态读取CSV到Inventory数组
int readInventories(Inventory **inventories, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s，错误原因：%s\n", filename, strerror(errno));
        *inventories = NULL;
        return 0;
    }

    char line[MAX_LINE_LEN];
    int count = 0;
    int capacity = 100;
    *inventories = (Inventory*)malloc(capacity * sizeof(Inventory));
    if (!*inventories) {
        printf("内存分配失败\n");
        fclose(file);
        return 0;
    }

    // 跳过表头
    if (fgets(line, MAX_LINE_LEN, file) == NULL) {
        fclose(file);
        return 0;
    }

    while (fgets(line, MAX_LINE_LEN, file)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (count >= capacity) {
            capacity *= 2;
            Inventory *temp = (Inventory*)realloc(*inventories, capacity * sizeof(Inventory));
            if (!temp) {
                printf("内存扩展失败，已读取 %d 条记录\n", count);
                break;
            }
            *inventories = temp;
        }

        char *token = strtok(line, ",");
        if (token) (*inventories)[count].id = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) (*inventories)[count].version = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*inventories)[count].set_num, token);
        
        count++;
    }

    fclose(file);
    return count;
}

// 动态读取CSV到InventoryPart数组
int readInventoryParts(InventoryPart **parts, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s，错误原因：%s\n", filename, strerror(errno));
        *parts = NULL;
        return 0;
    }

    char line[MAX_LINE_LEN];
    int count = 0;
    int capacity = 100;
    *parts = (InventoryPart*)malloc(capacity * sizeof(InventoryPart));
    if (!*parts) {
        printf("内存分配失败\n");
        fclose(file);
        return 0;
    }

    // 跳过表头
    if (fgets(line, MAX_LINE_LEN, file) == NULL) {
        fclose(file);
        return 0;
    }

    while (fgets(line, MAX_LINE_LEN, file)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (count >= capacity) {
            capacity *= 2;
            InventoryPart *temp = (InventoryPart*)realloc(*parts, capacity * sizeof(InventoryPart));
            if (!temp) {
                printf("内存扩展失败，已读取 %d 条记录\n", count);
                break;
            }
            *parts = temp;
        }

        char *token = strtok(line, ",");
        if (token) (*parts)[count].inventory_id = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*parts)[count].part_num, token);
        
        token = strtok(NULL, ",");
        if (token) (*parts)[count].color_id = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) (*parts)[count].quantity = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*parts)[count].is_spare, token);
        
        count++;
    }

    fclose(file);
    return count;
}

// 动态读取CSV到Color数组
int readColors(Color **colors, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s，错误原因：%s\n", filename, strerror(errno));
        *colors = NULL;
        return 0;
    }

    char line[MAX_LINE_LEN];
    int count = 0;
    int capacity = 100;
    *colors = (Color*)malloc(capacity * sizeof(Color));
    if (!*colors) {
        printf("内存分配失败\n");
        fclose(file);
        return 0;
    }

    // 跳过表头
    if (fgets(line, MAX_LINE_LEN, file) == NULL) {
        fclose(file);
        return 0;
    }

    while (fgets(line, MAX_LINE_LEN, file)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (count >= capacity) {
            capacity *= 2;
            Color *temp = (Color*)realloc(*colors, capacity * sizeof(Color));
            if (!temp) {
                printf("内存扩展失败，已读取 %d 条记录\n", count);
                break;
            }
            *colors = temp;
        }

        char *token = strtok(line, ",");
        if (token) (*colors)[count].id = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*colors)[count].name, token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*colors)[count].rgb, token);
        
        token = strtok(NULL, ",");
        if (token) strcpy((*colors)[count].is_trans, token);
        
        count++;
    }

    fclose(file);
    return count;
}

// 多表关联查询（结果集也用动态分配）
Result* multiTableJoin(
    Set *sets, int setCount,
    Theme *themes, int themeCount,
    Inventory *inventories, int inventoryCount,
    InventoryPart *parts, int partCount,
    Color *colors, int colorCount,
    int *resultCount  // 用于传出结果数量
) {
    *resultCount = 0;
    int capacity = 100;
    Result *results = (Result*)malloc(capacity * sizeof(Result));
    if (!results) {
        printf("结果集内存分配失败\n");
        return NULL;
    }

    // 多重循环关联（逻辑不变，增加动态扩展）
    for (int s = 0; s < setCount; s++) {
        if (sets[s].year < 2000 || sets[s].year > 2020) continue;

        for (int t = 0; t < themeCount; t++) {
            if (sets[s].theme_id == themes[t].id && strcmp(themes[t].name, "Castle") == 0) {

                for (int i = 0; i < inventoryCount; i++) {
                    if (strcmp(sets[s].set_num, inventories[i].set_num) == 0) {

                        for (int p = 0; p < partCount; p++) {
                            if (inventories[i].id == parts[p].inventory_id && parts[p].quantity >= 5) {

                                for (int c = 0; c < colorCount; c++) {
                                    if (parts[p].color_id == colors[c].id && strcmp(colors[c].name, "Black") == 0) {

                                        // 扩展结果集容量
                                        if (*resultCount >= capacity) {
                                            capacity *= 2;
                                            Result *temp = (Result*)realloc(results, capacity * sizeof(Result));
                                            if (!temp) {
                                                printf("结果集扩展失败，已保存 %d 条记录\n", *resultCount);
                                                return results;
                                            }
                                            results = temp;
                                        }

                                        // 保存结果
                                        strcpy(results[*resultCount].set_num, sets[s].set_num);
                                        strcpy(results[*resultCount].set_name, sets[s].name);
                                        results[*resultCount].publish_year = sets[s].year;
                                        strcpy(results[*resultCount].theme_name, themes[t].name);
                                        strcpy(results[*resultCount].part_id, parts[p].part_num);
                                        results[*resultCount].inventory_quantity = parts[p].quantity;
                                        (*resultCount)++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 按数量降序排序
    for (int i = 0; i < *resultCount - 1; i++) {
        for (int j = 0; j < *resultCount - i - 1; j++) {
            if (results[j].inventory_quantity < results[j+1].inventory_quantity) {
                Result temp = results[j];
                results[j] = results[j+1];
                results[j+1] = temp;
            }
        }
    }

    return results;
}

// 打印结果
void printResults(Result *results, int count) {
    if (count == 0) {
        printf("未找到符合条件的记录\n");
        return;
    }
    printf("套装编号,套装名称,发布年份,主题名称,零件编号,零件数量\n");
    for (int i = 0; i < count; i++) {
        printf("%s,%s,%d,%s,%s,%d\n",
            results[i].set_num,
            results[i].set_name,
            results[i].publish_year,
            results[i].theme_name,
            results[i].part_id,
            results[i].inventory_quantity
        );
    }
}

// 封装一次完整查询（读取文件+执行查询+释放内存），返回总耗时（秒）
double runOnce() {
    // 记录开始时间（包含读取文件的时间）
    clock_t start_time = clock();

    // 动态数组指针
    Set *sets = NULL;
    Theme *themes = NULL;
    Inventory *inventories = NULL;
    InventoryPart *inventoryParts = NULL;
    Color *colors = NULL;

    // 读取文件（使用绝对路径）
    int setCount = readSets(&sets, "D:\\SQLlab\\lego\\data\\sets.csv");
    int themeCount = readThemes(&themes, "D:\\SQLlab\\lego\\data\\themes.csv");
    int inventoryCount = readInventories(&inventories, "D:\\SQLlab\\lego\\data\\inventories.csv");
    int partCount = readInventoryParts(&inventoryParts, "D:\\SQLlab\\lego\\data\\inventory_parts.csv");
    int colorCount = readColors(&colors, "D:\\SQLlab\\lego\\data\\colors.csv");

    // 检查文件读取是否成功
    if (!sets || !themes || !inventories || !inventoryParts || !colors) {
        printf("文件读取失败，本次查询终止\n");
        // 释放已分配的内存
        free(sets);
        free(themes);
        free(inventories);
        free(inventoryParts);
        free(colors);
        return -1.0;  // 标记失败
    }

    // 执行查询
    int resultCount = 0;
    Result *results = multiTableJoin(
        sets, setCount,
        themes, themeCount,
        inventories, inventoryCount,
        inventoryParts, partCount,
        colors, colorCount,
        &resultCount
    );

    // 打印本次查询结果数量（可选，避免重复输出详细结果）
    printf("第 X 次查询结果：%d 条记录\n", resultCount);  // 后续会替换 X 为具体次数

    // 释放所有内存
    free(sets);
    free(themes);
    free(inventories);
    free(inventoryParts);
    free(colors);
    free(results);

    // 计算总耗时（包含读取文件和查询）
    clock_t end_time = clock();
    return (double)(end_time - start_time) / CLOCKS_PER_SEC;
}

int main() {
    const int total_runs = 5;  // 连续查询5次
    double times[total_runs];  // 存储每次耗时
    double sum = 0.0;
    int success_runs = 0;      // 记录成功的次数

    // 连续执行5次查询
    for (int i = 0; i < total_runs; i++) {
        printf("\n===== 第 %d 次查询开始 =====\n", i + 1);
        double elapsed = runOnce();

        if (elapsed < 0) {
            // 本次查询失败
            times[i] = -1.0;
            printf("第 %d 次查询失败\n", i + 1);
        } else {
            // 本次查询成功，记录时间
            times[i] = elapsed;
            sum += elapsed;
            success_runs++;
            printf("第 %d 次查询耗时：%.6f 秒（%.2f 毫秒）\n",
                   i + 1, elapsed, elapsed * 1000);
        }
    }

    // 输出统计结果
    printf("\n===== 查询统计结果 =====\n");
    printf("总查询次数：%d 次\n", total_runs);
    printf("成功次数：%d 次\n", success_runs);
    if (success_runs > 0) {
        double avg = sum / success_runs;
        printf("每次耗时：");
        for (int i = 0; i < total_runs; i++) {
            if (times[i] >= 0) {
                printf("%.6f 秒", times[i]);
            } else {
                printf("失败");
            }
            if (i < total_runs - 1) printf("，");
        }
        printf("\n平均耗时：%.6f 秒（%.2f 毫秒）\n", avg, avg * 1000);
    } else {
        printf("所有查询均失败，无法计算平均值\n");
    }

    return 0;
}