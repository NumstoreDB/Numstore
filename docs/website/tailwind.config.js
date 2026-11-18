/** @type {import('tailwindcss').Config} */
export default {
  darkMode: 'class',
  content: ['./index.html', './src/**/*.{vue,ts,tsx}'],
  theme: {
    extend: {
      fontFamily: {
        display: ['"Space Grotesk"', 'system-ui', 'sans-serif'],
        sans: ['Inter', 'system-ui', 'sans-serif'],
        mono: ['"JetBrains Mono"', 'ui-monospace', 'monospace'],
      },
      colors: {
        bg: '#0a0a0a',
        surface: '#0a0a0a',
        elevated: '#141414',
        border: '#333333',
        fg: '#f2f2f2',
        muted: '#8a8a8a',
      },
      maxWidth: {
        container: '1200px',
      },
    },
  },
  plugins: [],
}
