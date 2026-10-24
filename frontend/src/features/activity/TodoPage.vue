<script setup lang="ts">
/** 待办页：从错题本挑出的短清单（精选，不自动灌入）。
 * 平台页签 + 标签花片筛选（纯本地、即时）；已 AC 的条目由后端隐藏，
 * 但磁盘保留，同步冲掉提交后还会回来。筛选不写进网址（与错题本一致）。 */

import { computed, onMounted, ref, watch } from 'vue'
import { ExternalLink, ListChecks } from 'lucide-vue-next'
import { NButton, NSkeleton, NTag, useMessage } from 'naive-ui'
import PlatformTabs from '@/features/activity/components/PlatformTabs.vue'
import { fetchTodo, removeTodo } from '@/features/activity/api'
import { difficultyLabel, relativeDay, statusLabel, statusTagType } from '@/features/activity/model/review'
import {
  isActiveTag,
  removeTodoItem,
  tagChipLabel,
  toggleTag,
  type TodoTagFilter,
} from '@/features/activity/model/todo'
import { useUserGroups } from '@/features/activity/profile'
import { useActivity, type PlatformScope } from '@/features/activity/store'
import type { TodoData, TodoItem } from '@/features/activity/types'

const { init, activePlatform, setPlatform, platformName } = useActivity()
const { currentKey } = useUserGroups()
const message = useMessage()

const data = ref<TodoData | null>(null)
const loading = ref(false)
const removing = ref<string | null>(null)
/** 当前标签筛选（null = 全部） */
const tag = ref<TodoTagFilter>(null)

/** 空态两分：还没加过 / 当前标签下没有（后者说明筛选导致） */
const emptyHint = computed(() => {
  if (tag.value !== null) return '当前标签下没有待办'
  return '还没加过待办；去错题本把没过的题加进来'
})

const showEmptyAction = computed(() => tag.value === null)

let requestSeq = 0

async function load(): Promise<void> {
  const seq = ++requestSeq
  loading.value = true
  try {
    const res = await fetchTodo({ platform: activePlatform.value, tag: tag.value })
    if (seq !== requestSeq) return // 丢弃过期响应（快速切筛选时的竞态）
    data.value = res
  } catch (e) {
    if (seq !== requestSeq) return
    message.error(e instanceof Error ? e.message : '待办加载失败')
  } finally {
    if (seq === requestSeq) loading.value = false
  }
}

onMounted(() => {
  init()
  void load()
})

watch([activePlatform, tag, currentKey], () => {
  void load()
})

/** 切平台时清掉标签筛选：花片是按平台算的，留着旧标签会变成看不见的隐形筛选 */
function scope(value: PlatformScope): void {
  if (value === activePlatform.value) return
  tag.value = null
  setPlatform(value)
}

/** 点花片：再点一次或点「全部」取消筛选 */
function pickTag(name: TodoTagFilter): void {
  tag.value = toggleTag(tag.value, name)
}

function rowKey(item: TodoItem): string {
  return `${item.platform}/${item.problemKey}`
}

/** 移出待办：先落盘再改本地列表（失败时列表保持原样，避免假成功） */
async function onRemove(item: TodoItem): Promise<void> {
  const key = rowKey(item)
  if (removing.value) return
  removing.value = key
  try {
    await removeTodo(item.platform, item.problemKey)
    if (data.value) {
      data.value = {
        ...data.value,
        items: removeTodoItem(data.value.items, item.platform, item.problemKey),
        // 标签花片随条目减少重算（后端下次请求也会给同样的结果）
        tags: recountTags(
          removeTodoItem(data.value.items, item.platform, item.problemKey),
        ),
      }
    }
    message.success('已移出待办')
  } catch (e) {
    message.error(e instanceof Error ? e.message : '移出失败，请稍后重试')
  } finally {
    removing.value = null
  }
}

/** 本地重算花片：与后端口径一致（按条计数，同题重复标签只算一次） */
function recountTags(items: TodoItem[]): TodoData['tags'] {
  const counter = new Map<string, number>()
  for (const it of items) {
    for (const name of new Set(it.tags)) {
      counter.set(name, (counter.get(name) ?? 0) + 1)
    }
  }
  return [...counter.entries()]
    .map(([name, count]) => ({ name, count }))
    .sort((a, b) => (b.count - a.count) || a.name.localeCompare(b.name))
}

function goReview(): void {
  window.location.assign('/activity/review')
}
</script>

<template>
  <div class="todo-page">
    <div class="todo-toolbar">
      <PlatformTabs :model-value="activePlatform" @update:model-value="scope" />
      <span class="todo-count">{{ data ? `${data.items.length} 道` : '' }}</span>
    </div>

    <div v-if="data && data.tags.length > 0" class="tag-chips" role="group" aria-label="标签筛选">
      <button
        type="button"
        class="tag-chip"
        :class="{ active: tag === null }"
        :aria-pressed="tag === null"
        @click="pickTag(null)"
      >
        全部
      </button>
      <button
        v-for="t in data.tags"
        :key="t.name"
        type="button"
        class="tag-chip"
        :class="{ active: isActiveTag(tag, t.name) }"
        :aria-pressed="isActiveTag(tag, t.name)"
        @click="pickTag(t.name)"
      >
        {{ tagChipLabel(t) }}
      </button>
    </div>

    <section class="todo-panel">
      <div v-if="loading && !data" class="todo-skeleton">
        <NSkeleton v-for="i in 5" :key="i" height="38px" :sharp="false" />
      </div>

      <div v-else-if="(data?.items.length ?? 0) === 0" class="todo-empty">
        <div class="empty-icon"><ListChecks :size="26" /></div>
        <h2 class="empty-title">{{ tag === null ? '待办是空的' : '没有匹配的待办' }}</h2>
        <p class="empty-hint">{{ emptyHint }}</p>
        <NButton v-if="showEmptyAction" type="primary" @click="goReview">去错题本挑题</NButton>
        <p v-else class="empty-hint">点「全部」可以看完整清单。</p>
      </div>

      <ul v-else class="todo-list">
        <li v-for="item in data?.items" :key="rowKey(item)" class="todo-row">
          <span class="row-key">{{ item.problemKey }}</span>
          <a
            class="row-name"
            :href="item.problemUrl"
            target="_blank"
            rel="noopener noreferrer"
          >
            {{ item.problemName }}
            <ExternalLink :size="12" />
          </a>
          <span class="row-platform">{{ platformName(item.platform) }}</span>
          <span class="row-difficulty">{{ difficultyLabel(item.difficulty) }}</span>
          <NTag size="small" :bordered="false" :type="statusTagType(item.status)">
            {{ statusLabel(item.status) }}
          </NTag>
          <span class="row-tags">
            <template v-if="item.tags.length > 0">
              {{ item.tags.join(' · ') }}
            </template>
            <template v-else>—</template>
          </span>
          <span class="row-time">{{ relativeDay(item.addedAt) }}</span>
          <NButton
            class="row-remove"
            size="tiny"
            quaternary
            :loading="removing === rowKey(item)"
            @click="onRemove(item)"
          >
            移出
          </NButton>
        </li>
      </ul>
    </section>
  </div>
</template>

<style scoped>
.todo-page {
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 14px 20px 16px;
}

.todo-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: none;
  /* 与错题本一致：窄窗口下换行，不把整页撑出横向滚动条 */
  flex-wrap: wrap;
}

.todo-count {
  margin-left: auto;
  font-size: 12px;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.tag-chips {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px;
  flex: none;
}

.tag-chip {
  padding: 4px 11px;
  border: 1px solid var(--border);
  border-radius: 99px;
  background: var(--surface);
  color: var(--muted);
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition: background 0.16s ease, color 0.16s ease, border-color 0.16s ease;
}

.tag-chip:hover {
  background: var(--surface-2);
  color: var(--text);
}

.tag-chip.active {
  border-color: transparent;
  background: var(--accent-soft);
  color: var(--accent-strong);
}

.todo-panel {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.todo-skeleton {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.todo-list {
  margin: 0;
  padding: 0;
  list-style: none;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.todo-row {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 9px 12px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: var(--surface);
  font-size: 12.5px;
  transition: border-color 0.16s ease;
}

.todo-row:hover {
  border-color: var(--accent-soft);
}

.row-key {
  flex: none;
  min-width: 62px;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-weight: 600;
  color: var(--text);
}

.row-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: var(--text);
  text-decoration: none;
}

.row-name:hover {
  color: var(--accent-strong);
}

.row-platform,
.row-difficulty,
.row-time {
  flex: none;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.row-difficulty {
  min-width: 34px;
  text-align: right;
}

.row-tags {
  /* 标签列可压缩：窄窗口下优先挤压它，而不是把整行（进而整页）撑出横向滚动 */
  flex: 0 1 auto;
  min-width: 0;
  max-width: 220px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--muted);
}

.row-time {
  min-width: 52px;
  text-align: right;
}

.row-remove {
  flex: none;
  color: var(--muted);
}

.row-remove:hover {
  color: #c63b57;
}

.todo-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  margin: auto;
  padding: 48px 16px;
  text-align: center;
}

.todo-empty .empty-icon {
  color: var(--accent-strong);
  opacity: 0.8;
}

.empty-title {
  margin: 0;
  font-size: 15px;
  font-weight: 600;
  color: var(--text);
}

.empty-hint {
  margin: 0;
  font-size: 12.5px;
  color: var(--muted);
}
</style>
