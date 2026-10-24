/** 错题本展示纯函数（无副作用，便于单测）。
 * 口径见 docs/design/activity/review.md：状态标签、失败次数文案、相对时间、难度格式化。 */

import type { ReviewFilter, ReviewItem, ReviewStatus } from '@/features/activity/types'

/** 状态标签文案 */
export function statusLabel(status: ReviewStatus): string {
  return status === 'unsolved' ? '未通过' : '挣扎题'
}

/** 失败次数文案（未通过 = 从未 AC，展示为「失败 N 次」；挣扎题为首次 AC 前失败数） */
export function failLabel(item: ReviewItem): string {
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
