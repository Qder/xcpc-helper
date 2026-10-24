/** 错题本展示纯函数（无副作用，便于单测）。
 * 口径见 docs/design/activity/review.md：状态标签、失败次数文案、相对时间、难度格式化。 */

import type { ReviewFilter, ReviewItem, ReviewStatus } from '@/features/activity/types'

/** 状态标签文案 */
export function statusLabel(status: ReviewStatus): string {
  if (status === 'unsolved') return '未通过'
  if (status === 'struggled') return '挣扎题'
  return '收录'
}

/** 状态标签配色（naive-ui NTag 的 type） */
export function statusTagType(status: ReviewStatus): 'error' | 'warning' | 'info' {
  if (status === 'unsolved') return 'error'
  if (status === 'struggled') return 'warning'
  return 'info'
}

/** 失败次数文案（收录项没有提交数据，不显示次数，返回空串） */
export function failLabel(item: ReviewItem): string {
  if (item.status === 'collected') return ''
  return `失败 ${item.failCount} 次`
}

/**
 * 难度展示：CF 为分值（1850），洛谷为 0-7 档（4），LeetCode 为档位文本。
 * 各平台口径不同，只做展示格式化，不做跨平台归一。
 */
export function difficultyLabel(difficulty: number | string | null): string {
  if (difficulty === null || difficulty === '') return '—'
  if (typeof difficulty === 'number') return String(difficulty)
  return difficulty
}

function startOfDay(d: Date): number {
  return new Date(d.getFullYear(), d.getMonth(), d.getDate()).getTime()
}

const DAY_MS = 86_400_000

/**
 * 相对时间（按本地日历天数，不按 24 小时整除）：
 * 今天 / 昨天 / N 天前，超过 30 天回落到日期 MM-DD（跨年 YYYY-MM-DD）。
 */
export function relativeDay(iso: string, now: Date = new Date()): string {
  const d = new Date(iso)
  if (Number.isNaN(d.getTime())) return '—'
  const diffDays = Math.round((startOfDay(now) - startOfDay(d)) / DAY_MS)
  if (diffDays <= 0) return '今天'
  if (diffDays === 1) return '昨天'
  if (diffDays < 30) return `${diffDays} 天前`
  const y = d.getFullYear()
  const m = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  return y === now.getFullYear() ? `${m}-${day}` : `${y}-${m}-${day}`
}

/** 平台筛选是否命中（platform 为 null 表示不筛选） */
export function matchesPlatform(
  item: ReviewItem,
  platform: ReviewItem['platform'] | null,
): boolean {
  return platform === null || item.platform === platform
}

/** 状态筛选是否命中（filter 为 all 表示不筛选） */
export function matchesStatus(item: ReviewItem, filter: ReviewFilter): boolean {
  return filter === 'all' || item.status === filter
}

/** 平台能力是否支持按算法查找（与后端 review_tags 的能力分支一致） */
export function supportsTagSearch(capabilities: readonly string[]): boolean {
  return capabilities.includes('problem_lookup') || capabilities.includes('problemset')
}

function reviewKey(item: { platform: string; problemKey: string }): string {
  return `${item.platform}/${item.problemKey}`
}

/**
 * 收录成功后把条目并入当前列表（不整页刷新，保留筛选与滚动位置）。
 * - 已存在同 (platform, problemKey) 时原位替换（后端可能补到题名/难度）；
 * - 否则插入并按最近尝试时间倒序（与后端列表排序一致）。
 * 注意：若该题已 AC，后端不会把它算进 review 列表，这里也不该凭空插入——
 * 由调用方（页面）在收录前判断 solved，或依赖刷新后的列表结果。
 */
export function upsertReviewItem(items: ReviewItem[], item: ReviewItem): ReviewItem[] {
  const key = reviewKey(item)
  const next = items.some((i) => reviewKey(i) === key)
    ? items.map((i) => (reviewKey(i) === key ? item : i))
    : [...items, item]
  return next.sort((a, b) => {
    const diff = Date.parse(b.lastAttemptAt) - Date.parse(a.lastAttemptAt)
    if (diff !== 0) return diff
    return reviewKey(a).localeCompare(reviewKey(b))
  })
}

/** 删除一条手动收录（按平台 + 题号精确匹配，不误删同号的其它平台题） */
export function removeReviewItem(
  items: ReviewItem[],
  platform: string,
  problemKey: string,
): ReviewItem[] {
  const key = `${platform}/${problemKey}`
  return items.filter((i) => reviewKey(i) !== key)
}
