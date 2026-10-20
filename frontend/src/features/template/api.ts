/** 模板库相关 API。 */

import { request } from '@/shared/api/client'
import type {
  DiagnosticItem,
  SortMode,
  TemplateCreateInput,
  TemplateDetail,
  TemplateSummary,
  VersionUpsertPayload,
} from '@/features/template/types'

export interface ApiCategory {
  id: string
  name: string
  count: number
}

export interface TemplateQuery {
  category?: string
  keyword?: string
  sort?: SortMode
  /** true 时只返回至少一个版本有可视化的模板 */
  hasVisual?: boolean
}

export function fetchTemplates(query: TemplateQuery = {}): Promise<TemplateSummary[]> {
  const params = new URLSearchParams()
  if (query.category && query.category !== 'all') params.set('category', query.category)
  if (query.keyword?.trim()) params.set('keyword', query.keyword.trim())
  if (query.sort) params.set('sort', query.sort)
  if (query.hasVisual === true) params.set('has_visual', 'true')
  const qs = params.toString()
  return request<TemplateSummary[]>(`/templates${qs ? `?${qs}` : ''}`)
}

export function fetchTemplateDetail(id: string): Promise<TemplateDetail> {
  const encoded = id.split('/').map(encodeURIComponent).join('/')
  return request<TemplateDetail>(`/templates/${encoded}`)
}

export function fetchCategories(): Promise<ApiCategory[]> {
  return request<ApiCategory[]>('/categories')
}

export function fetchDiagnostics(): Promise<{ items: DiagnosticItem[] }> {
  return request<{ items: DiagnosticItem[] }>('/diagnostics')
}

export function reloadTemplates(): Promise<{ templates: number; diagnostics: number }> {
  return request<{ templates: number; diagnostics: number }>('/templates/reload', {
    method: 'POST',
  })
}

/** 将 "分类/模板名" 形式的 id 编码为 URL 路径段 */
function encodeId(id: string): string {
  return id.split('/').map(encodeURIComponent).join('/')
}

export function createTemplate(input: TemplateCreateInput): Promise<TemplateDetail> {
  return request<TemplateDetail>('/templates', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(input),
  })
}

export function deleteTemplate(id: string): Promise<void> {
  return request<void>(`/templates/${encodeId(id)}`, { method: 'DELETE' })
}

/** 批量删除主标签（含全部版本），ids 为「分类/模板名」 */
export function batchDeleteTemplates(ids: string[]): Promise<void> {
  return request<void>('/templates/batch-delete', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ids }),
  })
}

export function createVersion(
  templateId: string,
  payload: VersionUpsertPayload,
): Promise<TemplateDetail> {
  return request<TemplateDetail>(`/templates/${encodeId(templateId)}/versions`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  })
}

/** token 为副标签名；顶层单版本传 ROOT_VERSION_TOKEN（"~"） */
export function updateVersion(
  templateId: string,
  token: string,
  payload: VersionUpsertPayload,
): Promise<TemplateDetail> {
  return request<TemplateDetail>(
    `/templates/${encodeId(templateId)}/versions/${encodeURIComponent(token)}`,
    {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    },
  )
}

export function deleteVersion(templateId: string, token: string): Promise<void> {
  return request<void>(
    `/templates/${encodeId(templateId)}/versions/${encodeURIComponent(token)}`,
    { method: 'DELETE' },
  )
}

function visualBase(category: string, name: string, versionToken: string): string {
  return (
    `/templates/${encodeURIComponent(category)}/${encodeURIComponent(name)}` +
    `/versions/${encodeURIComponent(versionToken)}/visual`
  )
}

/** 写入某版本 visual.html；顶层单版本 token 为 ~ */
export function putVisual(
  category: string,
  name: string,
  versionToken: string,
  html: string,
): Promise<void> {
  return request<void>(visualBase(category, name, versionToken), {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ html }),
  })
}

export interface VisualHistoryItem {
  id: string
  bytes: number
  saved_at: string
}

export function fetchVisualHistory(
  category: string,
  name: string,
  versionToken: string,
): Promise<{ items: VisualHistoryItem[] }> {
  return request<{ items: VisualHistoryItem[] }>(
    `${visualBase(category, name, versionToken)}/history`,
  )
}

export function restoreVisualHistory(
  category: string,
  name: string,
  versionToken: string,
  snapshotId: string,
): Promise<void> {
  return request<void>(
    `${visualBase(category, name, versionToken)}/history/${encodeURIComponent(snapshotId)}/restore`,
    { method: 'POST' },
  )
}

export function deleteVisualHistoryItem(
  category: string,
  name: string,
  versionToken: string,
  snapshotId: string,
): Promise<void> {
  return request<void>(
    `${visualBase(category, name, versionToken)}/history/${encodeURIComponent(snapshotId)}`,
    { method: 'DELETE' },
  )
}

export function clearVisualHistory(
  category: string,
  name: string,
  versionToken: string,
): Promise<void> {
  return request<void>(`${visualBase(category, name, versionToken)}/history`, {
    method: 'DELETE',
  })
}
