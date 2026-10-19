import { describe, expect, it } from 'vitest'
import { ROOT_VERSION_TOKEN, type TemplateSummary, type TemplateVariant } from '@/features/template/types'
import {
  buildThemeMessage,
  filterVisualSummaries,
  parseVersionQuery,
  splitTemplateId,
  visualApiPath,
  visualIframeSrc,
  visualVariants,
  visualVersionToken,
  visualViewerPath,
} from '@/features/template/model/visual'

function summary(partial: Partial<TemplateSummary> & Pick<TemplateSummary, 'id' | 'name' | 'cat'>): TemplateSummary {
  return {
    lang: 'cpp',
    file: 'a.cpp',
    tags: [],
    src: null,
    page: null,
    updated: null,
    priority: 2,
    variant_count: 1,
    has_visual: true,
    ...partial,
  }
}

function variant(partial: Partial<TemplateVariant> & Pick<TemplateVariant, 'id' | 'name'>): TemplateVariant {
  return {
    lang: 'cpp',
    file: 'a.cpp',
    code: '',
    body: '',
    tags: [],
    src: null,
    page: null,
    updated: null,
    priority: 2,
    has_visual: true,
    ...partial,
  }
}

describe('splitTemplateId', () => {
  it('拆中文路径', () => {
    expect(splitTemplateId('字符串/哈希')).toEqual({ category: '字符串', name: '哈希' })
  })
})

describe('visualVersionToken', () => {
  it('顶层单版本为 ~', () => {
    expect(visualVersionToken('math/sieve', variant({ id: 'math/sieve', name: 'sieve' }))).toBe(
      ROOT_VERSION_TOKEN,
    )
  })
  it('副标签取 id 后缀', () => {
    expect(
      visualVersionToken(
        'ds/dsu',
        variant({ id: 'ds/dsu/path-compression', name: 'path-compression' }),
      ),
    ).toBe('path-compression')
  })
})

describe('visualApiPath', () => {
  it('中文与 ~ 都编码', () => {
    const path = `/templates/${encodeURIComponent('图论')}/${encodeURIComponent('最短路')}/versions/${encodeURIComponent('~')}/visual`
    expect(visualApiPath('图论', '最短路', '~')).toBe(path)
    expect(visualIframeSrc('图论', '最短路', '~')).toBe(`/api${path}`)
  })
})

describe('visualViewerPath', () => {
  it('缺省版本不带 query', () => {
    expect(visualViewerPath('math', 'sieve')).toBe('/template/visual/math/sieve')
  })
  it('指定版本写入 query 并编码', () => {
    expect(visualViewerPath('ds', 'dsu', '带权')).toBe(
      `/template/visual/ds/dsu?version=${encodeURIComponent('带权')}`,
    )
  })
})

describe('visualVariants / filterVisualSummaries', () => {
  it('只保留有可视化的版本', () => {
    const list = [
      variant({ id: 'a', name: 'a', has_visual: true }),
      variant({ id: 'b', name: 'b', has_visual: false }),
    ]
    expect(visualVariants(list).map((v) => v.id)).toEqual(['a'])
  })
  it('按分类与关键词筛模板', () => {
    const items = [
      summary({ id: 'math/sieve', name: 'sieve', cat: 'math', tags: ['素数'], has_visual: true }),
      summary({ id: 'ds/dsu', name: 'dsu', cat: 'ds', tags: [], has_visual: true }),
      summary({ id: 'math/fft', name: 'fft', cat: 'math', tags: [], has_visual: false }),
    ]
    expect(filterVisualSummaries(items, 'all', '').map((t) => t.id)).toEqual(['math/sieve', 'ds/dsu'])
    expect(filterVisualSummaries(items, 'math', '素').map((t) => t.id)).toEqual(['math/sieve'])
  })
})

describe('parseVersionQuery / buildThemeMessage', () => {
  it('空串视为缺省', () => {
    expect(parseVersionQuery('')).toBeNull()
    expect(parseVersionQuery('dijkstra')).toBe('dijkstra')
  })
  it('主题消息只含约定字段', () => {
    expect(buildThemeMessage(true, 160)).toEqual({ type: 'xcpc-theme', dark: true, hue: 160 })
  })
})
