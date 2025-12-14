# -*- coding: utf-8 -*-
"""
================================================================================
IMDB 电影数据爬虫 - 简洁入口
================================================================================
功能：
    根据 tconst (IMDB ID) 爬取电影详细信息，包括：
    - 电影信息：标题、国家、年份、时长、评分
    - 制作团队：导演(D)、编剧(W)、演员(A)

使用方法：
    # 任务1：爬取 all_matched_task1.csv 中的电影
    python run_crawler.py --task 1
    
    # 任务2：爬取 support_file_2.csv 中的电影  
    python run_crawler.py --task 2
    
    # 重置进度，从头开始（会覆盖输出文件）
    python run_crawler.py --task 1 --reset

输入文件：
    任务1: output/all_matched_task1.csv  (需先运行 prepare_task1.py)
    任务2: data/support_file_2.csv

输出文件（保存到新文件，不覆盖旧数据）：
    任务1: output/movie_spider_task1_v2.csv, output/mapping_IMDB_task1_v2.csv
    任务2: output/movie_spider_task2_v2.csv, output/mapping_IMDB_task2_v2.csv
================================================================================
"""

import os
import sys
import csv
import argparse
import logging
from typing import Any

# 添加项目路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from config import SUPPORT_FILE_2, LOG_DIR, OUTPUT_DIR
from utils.logger import setup_logger
from spider import IMDBSpider

# ============ 新的输出文件路径（避免覆盖旧数据）============
NEW_MOVIE_OUTPUT_TASK1 = os.path.join(OUTPUT_DIR, 'movie_spider_task1_v2.csv')
NEW_MAPPING_OUTPUT_TASK1 = os.path.join(OUTPUT_DIR, 'mapping_IMDB_task1_v2.csv')
NEW_MOVIE_OUTPUT_TASK2 = os.path.join(OUTPUT_DIR, 'movie_spider_task2_v2.csv')
NEW_MAPPING_OUTPUT_TASK2 = os.path.join(OUTPUT_DIR, 'mapping_IMDB_task2_v2.csv')

# 任务1输入文件
ALL_MATCHED_FILE = os.path.join(OUTPUT_DIR, 'all_matched_task1.csv')

# 新的输入文件（movies_tconst.csv）
MOVIES_TCONST_FILE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'movies_tconst.csv')


def load_task1_data() -> list[dict[str, Any]]:
    """
    加载任务1数据（从 all_matched_task1.csv）
    
    Returns:
        list[dict]: 包含 movie_id 和 tconst 的字典列表
    """
    if not os.path.exists(ALL_MATCHED_FILE):
        raise FileNotFoundError(
            f"未找到 {ALL_MATCHED_FILE}\n"
            f"请先运行: python prepare_task1.py"
        )
    
    data: list[dict[str, Any]] = []
    with open(ALL_MATCHED_FILE, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append({
                'movie_id': row.get('movie_id'),
                'tconst': row.get('tconst'),
            })
    return data


def load_movies_tconst_data() -> list[dict[str, Any]]:
    """
    加载movies_tconst.csv数据
    
    Returns:
        list[dict]: 包含tconst的字典列表（movie_id设为None）
    """
    if not os.path.exists(MOVIES_TCONST_FILE):
        raise FileNotFoundError(f"未找到 {MOVIES_TCONST_FILE}")
    
    data: list[dict[str, Any]] = []
    with open(MOVIES_TCONST_FILE, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            tconst = row.get('tconst')
            if tconst:
                data.append({
                    'movie_id': None,  # movies_tconst.csv没有movie_id
                    'tconst': tconst,
                })
    return data


def load_task2_data() -> list[str]:
    """
    加载任务2数据（从 support_file_2.csv）
    
    Returns:
        list[str]: tconst 列表
    """
    tconst_list: list[str] = []
    with open(SUPPORT_FILE_2, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            tconst = row.get('tconst')
            if tconst:
                tconst_list.append(tconst)
    return tconst_list


class CrawlerRunner:
    """
    爬虫运行器
    封装爬虫逻辑，支持任务1、任务2和任务3
    """
    
    def __init__(self, task: int):
        """
        初始化
        
        Args:
            task: 任务编号 (1、2 或 3)
        """
        self.task = task
        self.spider = IMDBSpider(task=task)
        self.logger = logging.getLogger('imdb_spider')
    
    def _get_output_files(self) -> tuple[str, str]:
        """获取输出文件路径"""
        if self.task == 1:
            return NEW_MOVIE_OUTPUT_TASK1, NEW_MAPPING_OUTPUT_TASK1
        elif self.task == 2:
            return NEW_MOVIE_OUTPUT_TASK2, NEW_MAPPING_OUTPUT_TASK2
        else:
            # 任务3：使用movies_tconst.csv，根据输入文件名生成输出文件名
            base_name = 'movies_tconst'
            movie_output = os.path.join(OUTPUT_DIR, f'movie_spider_{base_name}.csv')
            mapping_output = os.path.join(OUTPUT_DIR, f'mapping_IMDB_{base_name}.csv')
            return movie_output, mapping_output
    
    def init_files(self):
        """初始化输出文件（写入表头）"""
        movie_file, mapping_file = self._get_output_files()
        
        # 写入电影文件表头
        with open(movie_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(
                f,
                fieldnames=['movie_id', 'imdb_id', 'title', 'country_name', 
                           'year_released', 'runtime', 'rate'],
                quoting=csv.QUOTE_ALL
            )
            writer.writeheader()
        
        # 写入映射文件表头
        with open(mapping_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(
                f,
                fieldnames=['imdb_id', 'nconst', 'credited_as'],
                quoting=csv.QUOTE_ALL
            )
            writer.writeheader()
        
        self.logger.info(f"✅ 初始化输出文件:")
        self.logger.info(f"   {movie_file}")
        self.logger.info(f"   {mapping_file}")
    
    def run_task1(self):
        """运行任务1"""
        self.logger.info("=" * 60)
        self.logger.info("🎬 任务1: 爬取 all_matched_task1.csv")
        self.logger.info("=" * 60)
        
        # 加载数据
        data = load_task1_data()
        self.logger.info(f"📂 加载了 {len(data)} 条数据")
        
        # 覆盖spider的输出文件路径
        self._override_output_paths()
        
        # 开始爬取
        self.spider.crawl_task1(data)
    
    def run_task3(self):
        """运行任务3: 爬取 movies_tconst.csv"""
        self.logger.info("=" * 60)
        self.logger.info("🎬 任务3: 爬取 movies_tconst.csv")
        self.logger.info("=" * 60)
        
        # 加载数据
        data = load_movies_tconst_data()
        self.logger.info(f"📂 加载了 {len(data)} 条数据，准备开始爬取")
        
        # 初始化输出文件（写入表头）
        self.init_files()
        
        # 覆盖spider的输出文件路径
        self._override_output_paths()
        
        # 开始爬取
        self.spider.crawl_task1(data)
    
    def run_task2(self):
        """运行任务2"""
        self.logger.info("=" * 60)
        self.logger.info("🎬 任务2: 爬取 support_file_2.csv")
        self.logger.info("=" * 60)
        
        # 加载数据
        tconst_list = load_task2_data()
        self.logger.info(f"📂 加载了 {len(tconst_list)} 个 tconst")
        
        # 覆盖spider的输出文件路径
        self._override_output_paths()
        
        # 开始爬取
        self.spider.crawl_task2(tconst_list)
    
    def _override_output_paths(self):
        """覆盖spider的输出文件路径"""
        import config
        movie_file, mapping_file = self._get_output_files()
        
        if self.task == 1:
            config.MOVIE_OUTPUT_TASK1 = movie_file
            config.MAPPING_OUTPUT_TASK1 = mapping_file
        elif self.task == 2:
            config.MOVIE_OUTPUT_TASK2 = movie_file
            config.MAPPING_OUTPUT_TASK2 = mapping_file
        else:
            # 任务3：覆盖默认输出路径
            config.MOVIE_OUTPUT = movie_file
            config.MAPPING_OUTPUT = mapping_file


def main():
    """主函数"""
    # 解析命令行参数
    parser = argparse.ArgumentParser(
        description='IMDB电影数据爬虫',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python run_crawler.py --task 1         爬取任务1（all_matched_task1.csv）
  python run_crawler.py --task 2         爬取任务2（support_file_2.csv）
  python run_crawler.py --task 3         爬取任务3（movies_tconst.csv）
  python run_crawler.py --task 1 --reset 重置任务1进度
        """
    )
    parser.add_argument('--task', type=int, required=True, choices=[1, 2, 3],
                       help='任务编号: 1、2 或 3')
    parser.add_argument('--reset', action='store_true',
                       help='重置进度，从头开始')
    
    args = parser.parse_args()
    
    # 设置日志
    logger = setup_logger('imdb_spider', LOG_DIR)
    
    # 显示启动信息
    print()
    print("=" * 60)
    print("🚀 IMDB 电影数据爬虫")
    print("=" * 60)
    print(f"  任务: {args.task}")
    print(f"  重置: {'是' if args.reset else '否'}")
    print()
    
    # 创建运行器
    runner = CrawlerRunner(task=args.task)
    
    # 重置进度
    if args.reset:
        runner.init_files()
        runner.spider.checkpoint.reset()
        logger.info("🔄 已重置进度")
    
    # 运行任务
    if args.task == 1:
        runner.run_task1()
    elif args.task == 2:
        runner.run_task2()
    else:
        runner.run_task3()
    
    # 打印统计
    from utils.anti_spider import anti_spider
    stats = anti_spider.get_stats()
    
    print()
    print("=" * 60)
    print("📊 爬取统计")
    print("=" * 60)
    print(f"  总请求: {stats['total_requests']}")
    print(f"  成功:   {stats['success']}")
    print(f"  失败:   {stats['failed']}")
    print(f"  成功率: {stats['success_rate']}")
    print()
    print("✅ 爬虫完成！")
    print("=" * 60)


if __name__ == '__main__':
    main()
