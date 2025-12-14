# -*- coding: utf-8 -*-
"""
IMDB爬虫项目 - 核心爬虫模块
"""

import csv
import logging
from typing import Any

from parser import IMDBParser
from utils.anti_spider import anti_spider
from utils.checkpoint import CheckpointManager
import config

import os

logger = logging.getLogger('imdb_spider')


class IMDBSpider:
    """IMDB爬虫"""
    
    BASE_URL = 'https://www.imdb.com/title/{tconst}/'
    
    def __init__(self, task: int = 0) -> None:
        self.parser = IMDBParser()
        self.current_task: int = task  # 当前任务编号
        self.checkpoint = self._get_checkpoint_manager()
        self.movie_data: list[dict[str, Any]] = []     # 存储电影数据
        self.mapping_data: list[dict[str, Any]] = []   # 存储credits数据
        
        # 确保输出目录存在
        os.makedirs(config.OUTPUT_DIR, exist_ok=True)
    
    def _get_checkpoint_manager(self) -> CheckpointManager:
        """根据任务获取对应的checkpoint管理器"""
        if self.current_task == 1:
            return CheckpointManager(config.CHECKPOINT_FILE_TASK1)
        elif self.current_task == 2:
            return CheckpointManager(config.CHECKPOINT_FILE_TASK2)
        elif self.current_task == 3:
            return CheckpointManager()  # 任务3使用默认checkpoint
        else:
            return CheckpointManager()
    
    def set_task(self, task: int) -> None:
        """切换任务并重新加载对应的checkpoint"""
        self.current_task = task
        self.checkpoint = self._get_checkpoint_manager()
    
    def _get_output_files(self) -> tuple[str, str]:
        """根据当前任务获取输出文件路径（动态读取config，支持运行时覆盖）"""
        if self.current_task == 1:
            return config.MOVIE_OUTPUT_TASK1, config.MAPPING_OUTPUT_TASK1
        elif self.current_task == 2:
            return config.MOVIE_OUTPUT_TASK2, config.MAPPING_OUTPUT_TASK2
        else:
            return config.MOVIE_OUTPUT, config.MAPPING_OUTPUT
    
    def crawl_by_tconst(self, tconst: str, movie_id: str = '') -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
        """
        根据tconst爬取单个电影
        
        Args:
            tconst: IMDB ID (如 tt0111161)
            movie_id: 原始数据的movie_id（任务1用）
        
        Returns:
            (movie_info, credits_list) 或 (None, [])
        """
        url = self.BASE_URL.format(tconst=tconst)
        logger.info(f"🎬 爬取: {tconst}")
        
        # 发送请求
        response, status = anti_spider.safe_request(url)
        
        if status != 'ok' or response is None:
            logger.warning(f"❌ 爬取失败: {tconst}, status={status}")
            return None, []
        
        # 解析页面
        result = self.parser.parse_movie_page(response.text, url)
        
        if result['error']:
            logger.warning(f"⚠️ 解析失败: {tconst}, error={result['error']}")
            return None, []
        
        movie = result['movie']
        credits = result['credits']
        
        # 添加movie_id
        if movie:
            movie['movie_id'] = movie_id
        
        logger.info(f"✅ 成功: {tconst} - {movie.get('title', 'N/A')} "
                   f"({movie.get('year_released', 'N/A')}) "
                   f"⭐{movie.get('rate', 'N/A')} "
                   f"👥{len(credits)}人")
        
        return movie, credits
    
    def crawl_task2(self, tconst_list: list[str]) -> None:
        """
        任务2: 根据tconst列表爬取
        
        Args:
            tconst_list: tconst列表
        """
        self.current_task = 2  # 设置当前任务
        total = len(tconst_list)
        self.checkpoint.set_total(2, total)
        
        # 获取未完成的
        remaining = self.checkpoint.get_remaining(2, tconst_list)
        logger.info(f"📋 任务2: 共{total}条，已完成{total-len(remaining)}条，剩余{len(remaining)}条")
        
        for i, tconst in enumerate(remaining):
            logger.info(f"[{i+1}/{len(remaining)}] 进度: {(i+1)/len(remaining)*100:.1f}%")
            
            movie, credits = self.crawl_by_tconst(tconst, movie_id='')
            
            if movie:
                self.movie_data.append(movie)
                self.mapping_data.extend(credits)
                self.checkpoint.mark_completed(2, tconst)
                
                # 每100条保存一次
                if len(self.movie_data) % 100 == 0:
                    self._save_incremental()
            else:
                self.checkpoint.mark_failed(2, tconst)
        
        # 最终保存
        self._save_incremental()
        self.checkpoint.save()
        self.checkpoint.print_status()
    
    def crawl_task1(self, matched_list: list[dict[str, Any]]) -> None:
        """
        根据匹配结果爬取（支持任务1和任务3）
        
        Args:
            matched_list: 包含 movie_id 和 tconst 的字典列表
        """
        # 使用当前已经设置的任务，不硬编码为1
        total = len(matched_list)
        
        # 根据当前任务使用不同的进度跟踪键
        if self.current_task == 3:
            # 任务3：使用tconst作为跟踪ID，因为没有movie_id
            self.checkpoint.set_total(self.current_task, total)
            
            # 获取未完成的tconst
            all_ids = [m['tconst'] for m in matched_list]
            remaining_ids = set(self.checkpoint.get_remaining(self.current_task, all_ids))
            remaining = [m for m in matched_list if m['tconst'] in remaining_ids]
            
            logger.info(f"📋 任务{self.current_task}: 共{total}条，已完成{total-len(remaining)}条，剩余{len(remaining)}条")
            
            for i, item in enumerate(remaining):
                movie_id = item['movie_id']
                tconst = item['tconst']
                
                logger.info(f"[{i+1}/{len(remaining)}] 进度: {(i+1)/len(remaining)*100:.1f}%")
                
                movie, credits = self.crawl_by_tconst(tconst, movie_id=str(movie_id) if movie_id else '')
                
                if movie:
                    self.movie_data.append(movie)
                    self.mapping_data.extend(credits)
                    self.checkpoint.mark_completed(self.current_task, tconst)
                    
                    if len(self.movie_data) % 100 == 0:
                        self._save_incremental()
                else:
                    self.checkpoint.mark_failed(self.current_task, tconst)
        else:
            # 任务1：使用movie_id作为跟踪ID
            self.checkpoint.set_total(self.current_task, total)
            
            # 获取未完成的movie_id
            all_ids = [m['movie_id'] for m in matched_list]
            remaining_ids = set(self.checkpoint.get_remaining(self.current_task, all_ids))
            remaining = [m for m in matched_list if m['movie_id'] in remaining_ids]
            
            logger.info(f"📋 任务{self.current_task}: 共{total}条，已完成{total-len(remaining)}条，剩余{len(remaining)}条")
            
            for i, item in enumerate(remaining):
                movie_id = item['movie_id']
                tconst = item['tconst']
                
                logger.info(f"[{i+1}/{len(remaining)}] 进度: {(i+1)/len(remaining)*100:.1f}%")
                
                movie, credits = self.crawl_by_tconst(tconst, movie_id=str(movie_id))
                
                if movie:
                    self.movie_data.append(movie)
                    self.mapping_data.extend(credits)
                    self.checkpoint.mark_completed(self.current_task, movie_id)
                    
                    if len(self.movie_data) % 100 == 0:
                        self._save_incremental()
                else:
                    self.checkpoint.mark_failed(self.current_task, movie_id)
        
        self._save_incremental()
        self.checkpoint.save()
        self.checkpoint.print_status()
    
    def _save_incremental(self):
        """增量保存数据"""
        movie_file, mapping_file = self._get_output_files()
        
        if self.movie_data:
            self._append_csv(movie_file, self.movie_data, [
                'movie_id', 'imdb_id', 'title', 'country_name', 
                'year_released', 'runtime', 'rate'
            ])
            self.movie_data = []
        
        if self.mapping_data:
            self._append_csv(mapping_file, self.mapping_data, [
                'imdb_id', 'nconst', 'credited_as'
            ])
            self.mapping_data = []
    
    def _append_csv(self, filepath: str, data: list[dict[str, Any]], fieldnames: list[str]) -> None:
        """追加写入CSV"""
        file_exists = os.path.exists(filepath)
        
        with open(filepath, 'a', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(
                f, 
                fieldnames=fieldnames,
                quoting=csv.QUOTE_ALL,
                extrasaction='ignore'
            )
            
            # 如果文件不存在，写入header
            if not file_exists:
                writer.writeheader()
            
            writer.writerows(data)
        
        logger.debug(f"已追加 {len(data)} 条到 {filepath}")
    
    def init_output_files(self, task: int = 0):
        """初始化输出文件（清空并写入header）"""
        self.current_task = task
        movie_file, mapping_file = self._get_output_files()
        
        # movie_spider.csv
        with open(movie_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(
                f,
                fieldnames=['movie_id', 'imdb_id', 'title', 'country_name', 
                           'year_released', 'runtime', 'rate'],
                quoting=csv.QUOTE_ALL
            )
            writer.writeheader()
        
        # mapping_IMDB.csv
        with open(mapping_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(
                f,
                fieldnames=['imdb_id', 'nconst', 'credited_as'],
                quoting=csv.QUOTE_ALL
            )
            writer.writeheader()
        
        logger.info(f"✅ 初始化输出文件: {movie_file}, {mapping_file}")
        
        logger.info(f"✅ 初始化输出文件: {config.MOVIE_OUTPUT}, {config.MAPPING_OUTPUT}")
