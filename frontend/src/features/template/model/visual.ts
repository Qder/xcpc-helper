/** 算法可视化：地址构建、版本 token 与列表筛选。 */

import { ROOT_VERSION_TOKEN, type TemplateSummary, type TemplateVariant } from '@/features/template/types'

export interface VisualThemeMessage {
  type: 'xcpc-theme'
  dark: boolean
  hue: number
}

export function splitTemplateId(id: string): { category: string; name: string } {
  const i = id.indexOf('/')
  if (i <= 0 || i === id.length - 1) {
    throw new Error(`模板 id 无效: ${id}`)
  }
  return { category: id.slice(0, i), name: id.slice(i + 1) }
}

/** 顶层单版本用 ~，副标签用目录名（id 去掉「分类/模板/」前缀） */
export function visualVersionToken(templateId: string, variant: TemplateVariant): string {
  if (variant.id === templateId) return ROOT_VERSION_TOKEN
  const prefix = `${templateId}/`
  if (variant.id.startsWith(prefix)) return variant.id.slice(prefix.length)
  return variant.name
}

export function visualApiPath(category: string, name: string, versionToken: string): string {
  return (
    `/templates/${encodeURIComponent(category)}/${encodeURIComponent(name)}` +
      `/versions/${encodeURIComponent(versionToken)}/visual`
  )
}

export function visualIframeSrc(category: string, name: string, versionToken: string): string {
  return `/api${visualApiPath(category, name, versionToken)}`
}

export function visualHistoryListPath(
  category: string,
  name: string,
  versionToken: string,
): string {
  return `${visualApiPath(category, name, versionToken)}/history`
}

export function visualHistoryApiPath(
  category: string,
  name: string,
  versionToken: string,
  snapshotId: string,
): string {
  return `${visualHistoryListPath(category, name, versionToken)}/${encodeURIComponent(snapshotId)}`
}

export function visualHistoryIframeSrc(
  category: string,
  name: string,
  versionToken: string,
  snapshotId: string,
): string {
  return `/api${visualHistoryApiPath(category, name, versionToken, snapshotId)}`
}

export function visualViewerPath(
  category: string,
  name: string,
  versionQuery?: string | null,
  historyId?: string | null,
): string {
  const base = `/template/visual/${encodeURIComponent(category)}/${encodeURIComponent(name)}`
  const params = new URLSearchParams()
  if (versionQuery) params.set('version', versionQuery)
  if (historyId) params.set('history', historyId)
  const qs = params.toString()
  return qs ? `${base}?${qs}` : base
}

export function visualVariants(variants: TemplateVariant[]): TemplateVariant[] {
  return variants.filter((v) => v.has_visual)
}

export function filterVisualSummaries(
  items: TemplateSummary[],
  category: string,
  keyword: string,
): TemplateSummary[] {
  const cat = category === 'all' ? '' : category
  const q = keyword.trim().toLowerCase()
  return items.filter((t) => {
    if (!t.has_visual) return false
    if (cat && t.cat !== cat) return false
    if (!q) return true
    const tags = t.tags.join(' ').toLowerCase()
    return t.name.toLowerCase().includes(q) || tags.includes(q) || t.cat.toLowerCase().includes(q)
  })
}

export function parseVersionQuery(raw: unknown): string | null {
  if (typeof raw !== 'string') return null
  const value = raw.trim()
  return value ? value : null
}

const HISTORY_ID_RE = /^\d{8}-\d{6}Z(?:-\d+)?$/

export function parseHistoryQuery(raw: unknown): string | null {
  if (typeof raw !== 'string') return null
  const value = raw.trim()
  return HISTORY_ID_RE.test(value) ? value : null
}

export function formatHistorySavedAt(savedAt: string): string {
  return savedAt.replace('T', ' ').replace(/Z$/, ' UTC')
}

export function buildThemeMessage(dark: boolean, hue: number): VisualThemeMessage {
  return { type: 'xcpc-theme', dark, hue }
}
