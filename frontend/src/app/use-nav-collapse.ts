/** 应用左侧导航的收起状态，全页共用。 */

import { ref, watch } from 'vue'
import { load, save } from '@/shared/utils/storage'

const STORAGE_KEY = 'xc-nav-collapsed'
const navCollapsed = ref(load<boolean>(STORAGE_KEY, false))

watch(navCollapsed, (value) => save(STORAGE_KEY, value))

export function useNavCollapse() {
  function toggleNav(): void {
    navCollapsed.value = !navCollapsed.value
  }

  function setNavCollapsed(value: boolean): void {
    navCollapsed.value = value
  }

  return { navCollapsed, toggleNav, setNavCollapsed }
}
