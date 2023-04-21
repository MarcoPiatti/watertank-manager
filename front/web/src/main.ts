import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'
import WaterTank from './components/WaterTank.vue'

import './assets/main.css'

const app = createApp(WaterTank)

app.use(createPinia())
app.use(router)

app.mount('#app')
