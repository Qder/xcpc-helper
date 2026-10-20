/** 例题胶囊显示名：有 label 用 label，否则取 URL 最后一段路径或主机名。 */
export function exampleLabel(label: string, url: string): string {
  const text = label.trim()
  if (text) return text
  try {
    const parsed = new URL(url)
    const last = parsed.pathname.split('/').filter(Boolean).pop()
    if (last) return decodeURIComponent(last)
    return parsed.hostname
  } catch {
    return url
  }
}
