// '2026-11-18' -> 'November 18, 2026'. UTC, so the day does not shift by time zone.
export function formatDate(date: string): string {
  return new Date(`${date}T00:00:00Z`).toLocaleDateString('en-US', {
    year: 'numeric',
    month: 'long',
    day: 'numeric',
    timeZone: 'UTC',
  })
}
