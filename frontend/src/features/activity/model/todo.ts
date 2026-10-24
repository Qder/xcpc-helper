/** 待办展示纯函数（无副作用，便于单测）。
 * 口径见 docs/design/activity/todo.md：标签花片、本地移除、可加入判定。 */

import type { ReviewItem, TodoItem, TodoTag } from '@/features/activity/types'

/** 待办页的标签筛选值：null 表示不筛选（「全部」花片） */
export type TodoTagFilter = string | null

function todoKey(item: { platform: string; problemKey: string }): string {
  return `${item.platform}/${item.problemKey}`
}

/**
 * 错题本行是否给「待办」按钮：
 * - 只有未通过 / 收录能加入（挣扎题已经 AC，加进去立刻会被隐藏）；
 * - 已在清单里的行不给按钮（父级显示禁用的「已待办」）。
 */
export function canAddToTodo(item: ReviewItem): boolean {
  if (item.status === 'struggled') return false
  return !item.inTodo
}

/** 该行是否需要显示待办相关按钮（未通过 / 收录才显示） */
export function showsTodoAction(item: ReviewItem): boolean {
  return item.status !== 'struggled'
}

/** 标签筛选是否命中（null 表示不筛选；精确匹配，不做子串） */
export function matchesTag(item: TodoItem, tag: TodoTagFilter): boolean {
  return tag === null || item.tags.includes(tag)
}

/** 当前筛选下应展示的花片数量（点过的标签在花片上的高亮判断用） */
export function tagChipLabel(tag: TodoTag): string {
  return `${tag.name} ${tag.count}`
}

/** 判断某花片是否为当前选中项 */
export function isActiveTag(current: TodoTagFilter, name: string): boolean {
  return current === name
}

/**
 * 点击花片：点「全部」或再点一次已选标签 → 取消筛选。
 * 花片是单选（同一时刻只看一个算法标签），与错题本的状态筛选一致。
 */
export function toggleTag(current: TodoTagFilter, name: TodoTagFilter): TodoTagFilter {
  if (name === null || current === name) return null
  return name
}

/** 移出一条待办（按平台 + 题号精确匹配，不误删同号的其它平台题） */
export function removeTodoItem(
  items: TodoItem[],
  platform: string,
  problemKey: string,
): TodoItem[] {
  const key = `${platform}/${problemKey}`
  return items.filter((i) => todoKey(i) !== key)
}

/**
 * 错题本条目加入待办成功后，把该行标记为已待办（就地更新，不整页刷新）。
 * 找不到该行时原样返回（例如列表刚被筛选掉）。
 */
export function markInTodo(items: ReviewItem[], platform: string, problemKey: string): ReviewItem[] {
  const key = `${platform}/${problemKey}`
  return items.map((i) => (todoKey(i) === key ? { ...i, inTodo: true } : i))
}
