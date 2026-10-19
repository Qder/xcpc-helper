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

export function visualViewerPath(
  category: string,
  name: string,
  versionQuery?: string | null,
): string {
  const base = `/template/visual/${encodeURIComponent(category)}/${encodeURIComponent(name)}`
  if (!versionQuery) return base
  return `${base}?version=${encodeURIComponent(versionQuery)}`
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

export function buildThemeMessage(dark: boolean, hue: number): VisualThemeMessage {
  return { type: 'xcpc-theme', dark, hue }
}
