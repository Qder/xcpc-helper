<script setup lang="ts">
/** 收录区：粘贴题目链接 → 后端解析平台与题号 → 加入错题本。
 *
 * 不做前端正则校验（题目链接形态多：/contest/1/problem/A、/problem/P4779、
 * 带 query / 尾斜杠、官方短链），误报比不校验更烦人；错误统一由后端 detail 回显。
 * 成功后 emit added，由页面把条目并入列表（不整页刷新，保留筛选与滚动位置）。 */

import { ref } from 'vue'
import { Plus } from 'lucide-vue-next'
import { NButton, NInput, useMessage } from 'naive-ui'
import { addReviewManual } from '@/features/activity/api'
import { useActivity } from '@/features/activity/store'
import type { ReviewItem } from '@/features/activity/types'

const emit = defineEmits<{
  /** 后端收录成功（created=false 表示该题已在清单里） */
  added: [item: ReviewItem, created: boolean]
}>()

const { init } = useActivity()
const message = useMessage()

const url = ref('')
const submitting = ref(false)

init()

async function submit(): Promise<void> {
  const value = url.value.trim()
  if (!value || submitting.value) return
  submitting.value = true
  try {
    const res = await addReviewManual({ url: value })
    emit('added', res.item, res.created)
    url.value = ''
    message.success(res.created ? '已加入错题本' : '该题已在错题本中')
  } catch (e) {
    // 不猜原因：无法识别链接、平台不存在等都由后端 detail 给出
    message.error(e instanceof Error ? e.message : '收录失败，请稍后重试')
  } finally {
    submitting.value = false
  }
}
</script>

<template>
  <div class="manual-add">
    <span class="manual-label">收录</span>
    <NInput
      v-model:value="url"
      class="manual-input"
      size="small"
      placeholder="粘贴题目链接，如 https://codeforces.com/contest/2245/problem/F"
      :disabled="submitting"
      clearable
      @keyup.enter="submit"
    />
    <NButton
      size="small"
      type="primary"
      :loading="submitting"
      :disabled="!url.trim()"
      @click="submit"
    >
      <template #icon><Plus :size="14" /></template>
      加入
    </NButton>
  </div>
</template>

<style scoped>
.manual-add {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: var(--surface);
}

.manual-label {
  flex: none;
  font-size: 12px;
  font-weight: 600;
  color: var(--muted);
}

.manual-input {
  flex: 1;
  min-width: 0;
}
</style>
