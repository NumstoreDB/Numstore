import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'
import { docs, posts } from 'virtual:content'
import LandingView from './landing/LandingView.vue'
import DocPage from './docs/DocPage.vue'
import BlogIndex from './blog/BlogIndex.vue'
import BlogPost from './blog/BlogPost.vue'
import NotFound from './NotFound.vue'
import { tabFor } from './content/nav'
import { site_data } from './data'

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    name: 'home',
    component: LandingView,
    meta: { title: `${site_data.name} - ${site_data.tagline}` },
  },
  ...docs.map((page) => ({
    path: page.path,
    component: DocPage,
    props: { page },
    meta: { title: `${page.title} - ${site_data.name}`, tab: tabFor(page).label },
  })),
  {
    path: '/blog',
    component: BlogIndex,
    meta: { title: `Blog - ${site_data.name}`, tab: 'Blog' },
  },
  ...posts.map((post) => ({
    path: post.path,
    component: BlogPost,
    props: { post },
    meta: { title: `${post.title} - ${site_data.name}`, tab: 'Blog' },
  })),
  {
    path: '/:pathMatch(.*)*',
    component: NotFound,
    meta: { title: `Not found - ${site_data.name}` },
  },
]

// Pages load on demand, so an #anchor may not exist yet when the route changes.
function waitForElement(id: string, timeoutMs = 5000): Promise<Element | null> {
  const start = performance.now()
  return new Promise((resolve) => {
    const check = () => {
      const el = document.getElementById(id)
      if (el || performance.now() - start > timeoutMs) resolve(el)
      else setTimeout(check, 50)
    }
    check()
  })
}

export const router = createRouter({
  history: createWebHistory(),
  routes,
  async scrollBehavior(to, _from, saved) {
    if (saved) return saved
    if (to.hash) {
      const el = await waitForElement(decodeURIComponent(to.hash.slice(1)))
      if (el) return { el, top: 80 }
    }
    return { top: 0 }
  },
})

router.afterEach((to) => {
  const title = (to.meta?.title as string | undefined) ?? 'Numstore'
  if (typeof document !== 'undefined') document.title = title
})
