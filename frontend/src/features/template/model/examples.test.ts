import { describe, expect, it } from 'vitest'
import { exampleLabel } from '@/features/template/model/examples'

describe('exampleLabel', () => {
  it('prefers a non-empty label', () => {
    expect(exampleLabel('CF 26A', 'https://codeforces.com/problemset/problem/26/A')).toBe(
      'CF 26A',
    )
  })

  it('uses the last path segment when label is blank', () => {
    expect(exampleLabel('  ', 'https://www.luogu.com.cn/problem/P3383')).toBe('P3383')
  })

  it('falls back to hostname then to the raw url', () => {
    expect(exampleLabel('', 'https://example.com')).toBe('example.com')
    expect(exampleLabel('', 'not a url')).toBe('not a url')
  })
})
