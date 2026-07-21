import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    host: '127.0.0.1',
    port: 5173,
    strictPort: true,
    hmr: {
      host: '127.0.0.1',
      clientPort: 5173,
      protocol: 'ws'
    },
    proxy: {
      '/tiles': {
  target: 'http://127.0.0.1:3000',
        changeOrigin: true,
        rewrite: path => path.replace(/^\/tiles/, '')
      }
    }
  }
})
