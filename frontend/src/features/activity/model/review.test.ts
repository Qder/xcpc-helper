import { describe, expect, it } from 'vitest'
import {
  difficultyLabel,
  failLabel,
  matchesPlatform,
  matchesStatus,
  relativeDay,
  removeReviewItem,
  statusLabel,
  statusTagType,
  supportsTagSearch,
  upsertReviewItem,
} from '@/features/activity/model/review'
import type { ReviewItem } from '@/features/activity/types'

function item(overrides: Partial<ReviewItem> = {}): ReviewItem {
  return {
    platform: 'codeforces',
    problemKey: '2245F',
    problemName: 'Xor Minimization',
    problemUrl: 'https://codeforces.com/contest/2245/problem/F',
    difficulty: 2100,
    status: 'unsolved',
    failCount: 4,
    lastAttemptAt: '2026-10-20T10:00:00+08:00',
    canRecommend: true,
    ...overrides,
  }
}

describe('statusLabel', () => {
  it('区分未通过、挣扎题与收录', () => {
    expect(statusLabel('unsolved')).toBe('未通过')
    expect(statusLabel('struggled')).toBe('挣扎题')
    expect(statusLabel('collected')).toBe('收录')
  })
})

describe('statusTagType', () => {
  it('三种状态配色各不相同', () => {
    expect(statusTagType('unsolved')).toBe('error')
    expect(statusTagType('struggled')).toBe('warning')
    expect(statusTagType('collected')).toBe('info')
  })
})

describe('failLabel', () => {
  it('展示失败次数', () => {
    expect(failLabel(item({ failCount: 4 }))).toBe('失败 4 次')
    expect(failLabel(item({ failCount: 1 }))).toBe('失败 1 次')
  })

  it('收录项没有提交数据，不展示次数', () => {
    expect(failLabel(item({ status: 'collected', failCount: 0 }))).toBe('')
  })
})

describe('supportsTagSearch', () => {
  it('problemset 或 problem_lookup 任一即可（与后端能力分支一致）', () => {
    expect(supportsTagSearch(['problemset'])).toBe(true)
    expect(supportsTagSearch(['problem_lookup'])).toBe(true)
    expect(supportsTagSearch(['sync', 'refine_verdict'])).toBe(false)
    expect(supportsTagSearch([])).toBe(false)
  })
})

describe('upsertReviewItem', () => {
  it('新条目按最近时间倒序插入', () => {
    const old = item({ problemKey: '1A', lastAttemptAt: '2026-10-01T10:00:00+08:00' })
    const added = item({
      problemKey: '2B',
      status: 'collected',
      lastAttemptAt: '2026-10-20T10:00:00+08:00',
    })
    expect(upsertReviewItem([old], added).map((i) => i.problemKey)).toEqual(['2B', '1A'])
  })

  it('同 (platform, key) 原位替换而不是重复插入', () => {
    const before = item({ problemName: '旧题名' })
    const after = item({ problemName: '新题名' })
    const merged = upsertReviewItem([before], after)
    expect(merged).toHaveLength(1)
    expect(merged[0].problemName).toBe('新题名')
  })

  it('不同平台的同号题互不影响', () => {
    const cf = item({ platform: 'codeforces', problemKey: 'P1001' })
    const lg = item({ platform: 'luogu', problemKey: 'P1001' })
    expect(upsertReviewItem([cf], lg)).toHaveLength(2)
  })
})

describe('removeReviewItem', () => {
  it('按平台 + 题号精确删除', () => {
    const cf = item({ platform: 'codeforces', problemKey: 'P1001' })
    const lg = item({ platform: 'luogu', problemKey: 'P1001' })
    expect(removeReviewItem([cf, lg], 'luogu', 'P1001')).toEqual([cf])
  })

  it('条目不存在时原样返回', () => {
    const list = [item()]
    expect(removeReviewItem(list, 'luogu', 'P4779')).toEqual(list)
  })
})

describe('difficultyLabel', () => {
  it('数值难度直接展示', () => {
    expect(difficultyLabel(2100)).toBe('2100')
    expect(difficultyLabel(0)).toBe('0')
  })

  it('文本难度原样展示', () => {
    expect(difficultyLabel('困难')).toBe('困难')
  })

  it('缺失难度显示占位符', () => {
    expect(difficultyLabel(null)).toBe('—')
    expect(difficultyLabel('')).toBe('—')
  })
})

describe('relativeDay', () => {
  const now = new Date(2026, 9, 24, 12, 0, 0) // 2026-10-24 12:00 本地

  it('今天与昨天按日历天判定', () => {
    expect(relativeDay(new Date(2026, 9, 24, 0, 5).toISOString(), now)).toBe('今天')
    expect(relativeDay(new Date(2026, 9, 23, 23, 55).toISOString(), now)).toBe('昨天')
  })

  it('30 天内显示 N 天前', () => {
    expect(relativeDay(new Date(2026, 9, 20, 10, 0).toISOString(), now)).toBe('4 天前')
    expect(relativeDay(new Date(2026, 8, 25, 10, 0).toISOString(), now)).toBe('29 天前')
  })

  it('超过 30 天回落到日期', () => {
    expect(relativeDay(new Date(2026, 7, 1, 10, 0).toISOString(), now)).toBe('08-01')
  })

  it('跨年显示完整日期', () => {
    expect(relativeDay(new Date(2025, 4, 3, 10, 0).toISOString(), now)).toBe('2025-05-03')
  })

  it('非法时间给占位符', () => {
    expect(relativeDay('not-a-date', now)).toBe('—')
  })
})

describe('筛选', () => {
  it('平台筛选：null 表示全部', () => {
    expect(matchesPlatform(item(), null)).toBe(true)
    expect(matchesPlatform(item(), 'codeforces')).toBe(true)
    expect(matchesPlatform(item(), 'atcoder')).toBe(false)
  })

  it('状态筛选：all 表示全部', () => {
    expect(matchesStatus(item({ status: 'struggled' }), 'all')).toBe(true)
    expect(matchesStatus(item({ status: 'struggled' }), 'struggled')).toBe(true)
    expect(matchesStatus(item({ status: 'struggled' }), 'unsolved')).toBe(false)
  })
})
