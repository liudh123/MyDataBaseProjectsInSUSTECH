# -*- coding: utf-8 -*-
"""
================================================================================
IMDB爬虫项目 - HTML解析模块
================================================================================
功能：
    从 IMDB 电影页面的 __NEXT_DATA__ JSON 中提取结构化数据

提取的数据：
    电影信息 (movie):
        - imdb_id      : IMDB ID (如 tt0111161)
        - title        : 电影标题
        - country_name : 制片国家（可能有多个，逗号分隔）
        - year_released: 上映年份
        - runtime      : 时长（分钟）
        - rate         : IMDB 评分 (0-10)
    
    制作团队 (credits):
        - imdb_id      : 电影的 IMDB ID
        - nconst       : 人员的 IMDB ID (如 nm0000209)
        - credited_as  : 角色类型 (D=导演, W=编剧, A=演员)

技术说明：
    IMDB 使用 Next.js 框架，页面数据存储在 <script id="__NEXT_DATA__"> 中
    本模块通过正则提取该 JSON 并解析所需字段

注意事项：
    - country_name 使用 countriesOfOrigin（制片国家），而非 releaseDate（首映地点）
    - 这两个字段容易混淆，releaseDate.country 表示电影在哪个国家首映，
      countriesOfOrigin 表示电影是哪个国家制作的
================================================================================
"""

import json
import re
import logging
from typing import Any

logger = logging.getLogger('imdb_spider')


class IMDBParser:
    """IMDB页面解析器"""
    
    @staticmethod
    def parse_movie_page(html: str, url: str = '') -> dict[str, Any]:
        """
        解析电影详情页
        返回: dict 包含电影信息和credits
        """
        result: dict[str, Any] = {
            'movie': None,
            'credits': [],
            'error': None
        }
        
        try:
            # 提取 __NEXT_DATA__ JSON
            json_data = IMDBParser._extract_next_data(html)
            if not json_data:
                result['error'] = 'No __NEXT_DATA__ found'
                return result
            
            # 提取电影信息
            result['movie'] = IMDBParser._parse_movie_info(json_data)
            
            # 提取credits (导演/编剧/演员)
            result['credits'] = IMDBParser._parse_credits(json_data)
            
        except Exception as e:
            logger.error(f"解析失败 [{url}]: {e}")
            result['error'] = str(e)
        
        return result
    
    @staticmethod
    def _extract_next_data(html: str) -> dict[str, Any] | None:
        """提取 __NEXT_DATA__ JSON"""
        pattern = r'<script id="__NEXT_DATA__" type="application/json">(.+?)</script>'
        match = re.search(pattern, html, re.DOTALL)
        if match:
            try:
                return json.loads(match.group(1))
            except json.JSONDecodeError as e:
                logger.error(f"JSON解析失败: {e}")
        return None
    
    @staticmethod
    def _parse_movie_info(data: dict[str, Any]) -> dict[str, Any]:
        """解析电影基本信息"""
        movie: dict[str, Any] = {
            'imdb_id': None,
            'title': None,
            'country_name': None,
            'year_released': None,
            'runtime': None,
            'rate': None,
        }
        
        try:
            props: dict[str, Any] = data.get('props', {}).get('pageProps', {})
            above_fold: dict[str, Any] = props.get('aboveTheFoldData', {})
            
            # tconst
            movie['imdb_id'] = props.get('tconst')
            
            # 标题
            title_text: dict[str, Any] = above_fold.get('titleText', {})
            movie['title'] = title_text.get('text') if title_text else None
            
            # 原始标题（备选）
            if not movie['title']:
                original_title: dict[str, Any] = above_fold.get('originalTitleText', {})
                movie['title'] = original_title.get('text') if original_title else None
            
            # 年份 - 优先使用releaseDate.year（与IMDb官网显示的发布年份一致）
            # 回退到releaseYear.year（制作年份）
            release_date: dict[str, Any] = above_fold.get('releaseDate', {})
            movie['year_released'] = release_date.get('year') if release_date else None
            
            # 如果releaseDate.year不存在，回退到releaseYear.year
            if movie['year_released'] is None:
                release_year: dict[str, Any] = above_fold.get('releaseYear', {})
                movie['year_released'] = release_year.get('year') if release_year else None
            
            # 时长（秒转分钟）
            runtime: dict[str, Any] = above_fold.get('runtime', {})
            if runtime:
                seconds = runtime.get('seconds')
                if seconds is not None:
                    movie['runtime'] = int(seconds / 60)
            
            # 评分
            ratings: dict[str, Any] = above_fold.get('ratingsSummary', {})
            movie['rate'] = ratings.get('aggregateRating') if ratings else None
            
            # 国家（优先使用 aboveTheFoldData.countriesOfOrigin；只有在其为空或不完整时才使用 mainColumnData.countriesDetails）
            movie['country_name'] = IMDBParser._parse_country(props)
            
        except Exception as e:
            logger.error(f"解析电影信息失败: {e}")
        
        return movie
    
    @staticmethod
    def _parse_country(props: dict[str, Any]) -> str | None:
        """解析国家信息 - 优先检查 mainColumnData.countriesDetails，再检查 aboveTheFoldData.countriesOfOrigin。

        回退顺序：
          1. props.mainColumnData.countriesDetails
          2. props.aboveTheFoldData.countriesOfOrigin
          3. props.aboveTheFoldData.releaseDate.country (仅作最后回退)
        """
        try:
            above_fold: dict[str, Any] = props.get('aboveTheFoldData', {}) or {}
            main_column: dict[str, Any] = props.get('mainColumnData', {}) or {}

            # 国家代码映射
            country_codes = {
                'IN': 'India',
                'US': 'United States',
                'GB': 'United Kingdom', 
                'DE': 'Germany',
                'FR': 'France',
                'IT': 'Italy',
                'JP': 'Japan',
                'CN': 'China',
                'HK': 'Hong Kong',
                'KR': 'South Korea',
                'RU': 'Russia',
                'EG': 'Egypt',
                'ES': 'Spain',
                'BR': 'Brazil',
                'MX': 'Mexico',
                'CA': 'Canada',
                'AU': 'Australia',
                'SE': 'Sweden',
                'DK': 'Denmark',
                'NO': 'Norway',
                'PL': 'Poland',
                'NL': 'Netherlands',
                'BE': 'Belgium',
                'AT': 'Austria',
                'CH': 'Switzerland',
                'IE': 'Ireland',
                'NZ': 'New Zealand',
                'AR': 'Argentina',
                'TH': 'Thailand',
                'PH': 'Philippines',
                'ID': 'Indonesia',
                'MY': 'Malaysia',
                'SG': 'Singapore',
                'TW': 'Taiwan',
                'TR': 'Turkey',
                'GR': 'Greece',
                'PT': 'Portugal',
                'CZ': 'Czech Republic',
                'HU': 'Hungary',
                'RO': 'Romania',
                'IL': 'Israel',
                'ZA': 'South Africa',
                'XWG': 'West Germany',
                'SUHH': 'Soviet Union',
                'CSHH': 'Czechoslovakia',
                'YUCS': 'Yugoslavia',
                # 添加缺失的欧洲国家代码
                'SI': 'Slovenia',
                'HR': 'Croatia',
                'RS': 'Serbia',
                'ME': 'Montenegro',
                'MK': 'North Macedonia',
                'BG': 'Bulgaria',
                'LT': 'Lithuania',
                'LV': 'Latvia',
                'EE': 'Estonia',
                'SK': 'Slovakia',
                'BA': 'Bosnia and Herzegovina',
                'AL': 'Albania',
                'IS': 'Iceland',
            }

            # 首先使用 aboveTheFoldData.countriesOfOrigin（首选）
            countries: list[str] = []
            countries_of_origin: dict[str, Any] = above_fold.get('countriesOfOrigin', {})
            country_list: list[Any] = countries_of_origin.get('countries', []) if countries_of_origin else []

            # 如果 above_fold 没有数据，或只包含一项而 main_column 提供了更多条目，则使用 mainColumnData
            if not country_list or (len(country_list) == 1 and isinstance(main_column.get('countriesDetails', {}), dict) and len(main_column.get('countriesDetails', {}).get('countries', []) or []) > 1):
                countries_details: dict[str, Any] = main_column.get('countriesDetails', {})
                country_list = countries_details.get('countries', []) if countries_details else country_list

            if country_list:
                for c in country_list:
                    cid = c.get('id') if isinstance(c, dict) else None
                    if cid:
                        countries.append(country_codes.get(cid, cid))

                if countries:
                    return ', '.join(countries)

            # 最后回退到 releaseDate.country.text（表示首映国家）
            release_date: dict[str, Any] = above_fold.get('releaseDate', {})
            if release_date:
                country_obj: dict[str, Any] = release_date.get('country', {})
                if country_obj and country_obj.get('text'):
                    return country_obj.get('text')

        except Exception as e:
            logger.debug(f"解析国家失败: {e}")

        return None
    
    @staticmethod
    def _parse_credits(data: dict[str, Any]) -> list[dict[str, str]]:
        """解析导演/编剧/演员"""
        credits: list[dict[str, str]] = []
        
        try:
            props: dict[str, Any] = data.get('props', {}).get('pageProps', {})
            above_fold: dict[str, Any] = props.get('aboveTheFoldData', {})
            tconst: str | None = props.get('tconst')
            
            if not tconst:
                return credits
            
            # 从 principalCreditsV2 提取
            principal_credits: list[Any] = above_fold.get('principalCreditsV2', [])
            
            for credit_group in principal_credits:
                if not isinstance(credit_group, dict):
                    continue
                
                # 获取分组名称
                grouping: dict[str, Any] = credit_group.get('grouping', {})  # type: ignore[union-attr]
                grouping_text: str = ''
                if grouping:
                    text_val = grouping.get('text', '')  # type: ignore[union-attr]
                    if isinstance(text_val, str):
                        grouping_text = text_val.strip()
                
                # 确定角色类型
                if grouping_text in ['Director', 'Directors']:
                    role = 'D'
                elif grouping_text in ['Writer', 'Writers']:
                    role = 'W'
                elif grouping_text in ['Star', 'Stars']:
                    role = 'A'
                else:
                    continue
                
                # 提取人员ID
                credit_list: list[Any] = credit_group.get('credits', [])  # type: ignore[union-attr]
                for credit in credit_list:  # type: ignore[assignment]
                    if not isinstance(credit, dict):
                        continue
                    
                    name_obj: dict[str, Any] = credit.get('name', {})  # type: ignore[union-attr]
                    if name_obj:
                        nconst_val = name_obj.get('id')  # type: ignore[union-attr]
                        if nconst_val and isinstance(nconst_val, str) and nconst_val.strip():
                            credits.append({
                                'imdb_id': tconst,
                                'nconst': nconst_val,
                                'credited_as': role
                            })
            
        except Exception as e:
            logger.error(f"解析credits失败: {e}")
        
        return credits


# 测试用
if __name__ == '__main__':
    # 测试解析
    test_html = '''
    <script id="__NEXT_DATA__" type="application/json">
    {"props":{"pageProps":{"tconst":"tt0111161"}}}
    </script>
    '''
    result = IMDBParser.parse_movie_page(test_html)
    print(result)
