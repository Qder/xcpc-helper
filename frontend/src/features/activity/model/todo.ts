/** 待办展示纯函数（无副作用，便于单测）。
 * 口径见 docs/design/activity/todo.md：标签花片、本地移除、可加入判定。 */

import type { ReviewItem, TodoBatchItem, TodoItem, TodoTag } from '@/features/activity/types'

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

/* ---------- 批量导入（错题本多选 → 待办） ---------- */

/**
 * 该行能否被勾选进批量导入：与单条「待办」按钮同一口径。
 * 不能加入的行（挣扎题、已在待办）也不该出现在批量里，否则用户勾了一堆
 * 最后全是「跳过」。
 */
export function canSelectForImport(item: ReviewItem): boolean {
  return canAddToTodo(item)
}

/** 勾选/取消一行（返回新 Set，便于 Vue 响应式替换） */
export function toggleSelection(
  selected: ReadonlySet<string>,
  key: string,
  checked: boolean,
): Set<string> {
  const next = new Set(selected)
  if (checked) next.add(key)
  else next.delete(key)
  return next
}

/**
 * 「全选」作用范围：当前**可导入**的行（不能导入的行永远不该被选上）。
 * 已选集合严格收窄到这些行——切筛选后旧的不可选行自动失效。
 */
export function selectableKeys(items: ReviewItem[]): string[] {
  return items.filter(canSelectForImport).map(todoKey)
}

/**
 * 全选/取消全选结果：勾选时并入当前可选行，取消时只移除这些行。
 * 只作用于传入集合（页面传的是「当前页」或「全部」），不清掉集合外的已选行。
 */
export function toggleSelectAllKeys(
  selected: ReadonlySet<string>,
  keys: string[],
  checked: boolean,
): Set<string> {
  const next = new Set(selected)
  for (const key of keys) {
    if (checked) next.add(key)
    else next.delete(key)
  }
  return next
}

/** 已选行对应的请求体（按列表顺序，保证落盘顺序与用户看到的顺序一致）。
 * 一并带上列表里已有的画像快照：批量时后端不必逐题再查一次平台。 */
export function selectionPayload(
  items: ReviewItem[],
  selected: ReadonlySet<string>,
): TodoBatchItem[] {
  return items
    .filter((i) => selected.has(todoKey(i)) && canSelectForImport(i))
    .map((i) => ({
      platform: i.platform,
      problemKey: i.problemKey,
      problemName: i.problemName,
      problemUrl: i.problemUrl,
      difficulty: i.difficulty,
    }))
}

/** 批量导入成功的行就地标成 inTodo（不整页刷新，保留筛选与滚动位置） */
export function markManyInTodo(
  items: ReviewItem[],
  added: { platform: string; problemKey: string }[],
): ReviewItem[] {
  const keys = new Set(added.map((i) => `${i.platform}/${i.problemKey}`))
  if (keys.size === 0) return items
  return items.map((i) => (keys.has(todoKey(i)) ? { ...i, inTodo: true } : i))
}

/**
 * 批量导入的结果文案：新增与跳过分开说清，跳过还要区分原因。
 * 全部跳过时不报「已加入 0 道」这种假成功文案。
 */
export function batchResultMessage(
  addedCount: number,
  skipped: { reason: string }[],
): string {
  const acCount = skipped.filter((s) => s.reason === 'ac').length
  const existsCount = skipped.filter((s) => s.reason === 'exists').length
  const reasons: string[] = []
  if (acCount > 0) reasons.push(`${acCount} 道已 AC`)
  if (existsCount > 0) reasons.push(`${existsCount} 道已在待办`)

  if (addedCount === 0) {
    return reasons.length > 0 ? `没有新增：${reasons.join('、')}` : '没有可加入的题目'
  }
  const base = `已加入 ${addedCount} 道待办`
  return reasons.length > 0 ? `${base}，跳过 ${reasons.join('、')}` : base
}
