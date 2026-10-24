import { describe, expect, it } from 'vitest'
import {
  batchResultMessage,
  canAddToTodo,
  canSelectForImport,
  isActiveTag,
  markInTodo,
  markManyInTodo,
  matchesTag,
  removeTodoItem,
  selectableKeys,
  selectionPayload,
  showsTodoAction,
  tagChipLabel,
  toggleSelectAllKeys,
  toggleSelection,
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

describe('批量导入：可选口径与勾选', () => {
  it('可导入口径与单条「待办」按钮一致', () => {
    expect(canSelectForImport(reviewItem({ status: 'unsolved' }))).toBe(true)
    expect(canSelectForImport(reviewItem({ status: 'collected' }))).toBe(true)
    expect(canSelectForImport(reviewItem({ status: 'struggled' }))).toBe(false)
    expect(canSelectForImport(reviewItem({ inTodo: true }))).toBe(false)
  })

  it('selectableKeys 只留下可导入行，并带上平台前缀', () => {
    const items = [
      reviewItem({ platform: 'codeforces', problemKey: '2245B' }),
      reviewItem({ problemKey: '2245C', status: 'struggled' }),
      reviewItem({ platform: 'luogu', problemKey: 'P1001' }),
    ]
    expect(selectableKeys(items)).toEqual(['codeforces/2245B', 'luogu/P1001'])
  })

  it('toggleSelection 返回新集合，不改原集合', () => {
    const start = new Set<string>()
    const added = toggleSelection(start, 'codeforces/2245B', true)
    expect(added.has('codeforces/2245B')).toBe(true)
    expect(start.size).toBe(0)
    expect(toggleSelection(added, 'codeforces/2245B', false).size).toBe(0)
  })

  it('全选只并入/移除传入的键，不清掉集合外已选行', () => {
    const selected = new Set(['codeforces/9999Z']) // 别的页勾的
    const pageKeys = ['codeforces/2245B', 'codeforces/2245C']

    const on = toggleSelectAllKeys(selected, pageKeys, true)
    expect([...on].sort()).toEqual(
      ['codeforces/2245B', 'codeforces/2245C', 'codeforces/9999Z'].sort(),
    )

    const off = toggleSelectAllKeys(on, pageKeys, false)
    expect([...off]).toEqual(['codeforces/9999Z'])
  })

  it('selectionPayload 按列表顺序输出，且过滤掉不可导入的行', () => {
    const items = [
      reviewItem({ platform: 'luogu', problemKey: 'P1001' }),
      reviewItem({ platform: 'codeforces', problemKey: '2245B' }),
      reviewItem({ problemKey: '2245C', status: 'struggled' }),
    ]
    const selected = new Set([
      'luogu/P1001',
      'codeforces/2245B',
      'codeforces/2245C',
    ])
    const payload = selectionPayload(items, selected)
    expect(payload.map((i) => [i.platform, i.problemKey])).toEqual([
      ['luogu', 'P1001'],
      ['codeforces', '2245B'],
    ])
  })

  it('payload 带上画像快照，后端不必逐题再查平台', () => {
    const items = [
      reviewItem({
        platform: 'codeforces',
        problemKey: '2245B',
        problemName: 'Xor',
        problemUrl: 'https://codeforces.com/contest/2245/problem/B',
        difficulty: 2100,
      }),
    ]
    expect(selectionPayload(items, new Set(['codeforces/2245B']))).toEqual([
      {
        platform: 'codeforces',
        problemKey: '2245B',
        problemName: 'Xor',
        problemUrl: 'https://codeforces.com/contest/2245/problem/B',
        difficulty: 2100,
      },
    ])
  })
})

describe('markManyInTodo', () => {
  it('批量标记命中的行，空结果原样返回', () => {
    const items = [
      reviewItem({ problemKey: '2245B' }),
      reviewItem({ problemKey: '2245C' }),
      reviewItem({ platform: 'luogu', problemKey: 'P1001' }),
    ]
    const next = markManyInTodo(items, [
      { platform: 'codeforces', problemKey: '2245B' },
      { platform: 'luogu', problemKey: 'P1001' },
    ])
    expect(next.map((i) => i.inTodo)).toEqual([true, undefined, true])
    expect(markManyInTodo(items, [])).toBe(items)
  })

  it('同号不同平台不会互相误标', () => {
    const items = [
      reviewItem({ platform: 'codeforces', problemKey: 'P1001' }),
      reviewItem({ platform: 'luogu', problemKey: 'P1001' }),
    ]
    const next = markManyInTodo(items, [{ platform: 'luogu', problemKey: 'P1001' }])
    expect(next[0].inTodo).toBeUndefined()
    expect(next[1].inTodo).toBe(true)
  })
})

describe('batchResultMessage', () => {
  it('只说新增', () => {
    expect(batchResultMessage(3, [])).toBe('已加入 3 道待办')
  })

  it('新增与跳过分开说清，并按原因计数', () => {
    expect(
      batchResultMessage(2, [{ reason: 'ac' }, { reason: 'exists' }, { reason: 'ac' }]),
    ).toBe('已加入 2 道待办，跳过 2 道已 AC、1 道已在待办')
  })

  it('全部跳过时不说成成功', () => {
    expect(batchResultMessage(0, [{ reason: 'exists' }])).toBe(
      '没有新增：1 道已在待办',
    )
    expect(batchResultMessage(0, [])).toBe('没有可加入的题目')
  })
})
