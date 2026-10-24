import { describe, expect, it } from 'vitest'
import {
  canAddToTodo,
  isActiveTag,
  markInTodo,
  matchesTag,
  removeTodoItem,
  showsTodoAction,
  tagChipLabel,
  toggleTag,
} from '@/features/activity/model/todo'
import type { ReviewItem, TodoItem } from '@/features/activity/types'

function reviewItem(overrides: Partial<ReviewItem> = {}): ReviewItem {
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

function todoItem(overrides: Partial<TodoItem> = {}): TodoItem {
  return {
    platform: 'codeforces',
    problemKey: '2245F',
    problemName: 'Xor Minimization',
    problemUrl: 'https://codeforces.com/contest/2245/problem/F',
    difficulty: 2100,
    tags: ['dp', 'bitmasks'],
    addedAt: '2026-10-20T10:00:00+08:00',
    status: 'unsolved',
    ...overrides,
  }
}

describe('todo 按钮可见性', () => {
  it('未通过 / 收录可加入，挣扎题不出现', () => {
    expect(canAddToTodo(reviewItem({ status: 'unsolved' }))).toBe(true)
    expect(canAddToTodo(reviewItem({ status: 'collected' }))).toBe(true)
    expect(canAddToTodo(reviewItem({ status: 'struggled' }))).toBe(false)
    expect(showsTodoAction(reviewItem({ status: 'struggled' }))).toBe(false)
    expect(showsTodoAction(reviewItem({ status: 'unsolved' }))).toBe(true)
  })

  it('已在待办里的行不再可加入（前端置为已待办）', () => {
    expect(canAddToTodo(reviewItem({ inTodo: true }))).toBe(false)
  })
})

describe('matchesTag', () => {
  it('null 不筛选；标签是精确匹配不是子串', () => {
    const item = todoItem({ tags: ['线性dp'] })
    expect(matchesTag(item, null)).toBe(true)
    expect(matchesTag(item, '线性dp')).toBe(true)
    expect(matchesTag(item, 'dp')).toBe(false)
  })
})

describe('toggleTag', () => {
  it('点新标签选中，再点同一标签取消（含「全部」）', () => {
    expect(toggleTag(null, 'dp')).toBe('dp')
    expect(toggleTag('dp', 'math')).toBe('math')
    expect(toggleTag('dp', 'dp')).toBe(null)
    expect(toggleTag('dp', null)).toBe(null)
  })

  it('isActiveTag 只在名称完全一致时为真', () => {
    expect(isActiveTag('dp', 'dp')).toBe(true)
    expect(isActiveTag(null, 'dp')).toBe(false)
  })
})

describe('tagChipLabel', () => {
  it('花片文案带计数', () => {
    expect(tagChipLabel({ name: '线性dp', count: 2 })).toBe('线性dp 2')
  })
})

describe('removeTodoItem', () => {
  it('按平台 + 题号删除，不误删同号其它平台', () => {
    const items = [
      todoItem({ platform: 'codeforces', problemKey: '2245F' }),
      todoItem({ platform: 'luogu', problemKey: '2245F' }),
    ]
    const next = removeTodoItem(items, 'codeforces', '2245F')
    expect(next).toHaveLength(1)
    expect(next[0].platform).toBe('luogu')
  })
})

describe('markInTodo', () => {
  it('只标记命中的行，其余原样返回', () => {
    const items = [
      reviewItem({ problemKey: '2245B' }),
      reviewItem({ problemKey: '2245C' }),
    ]
    const next = markInTodo(items, 'codeforces', '2245B')
    expect(next[0].inTodo).toBe(true)
    expect(next[1].inTodo).toBeUndefined()
    expect(items[0].inTodo).toBeUndefined() // 不改原数组元素
  })
})
