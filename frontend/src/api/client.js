const BASE = '/api';

export async function fetchMaps() {
  const res = await fetch(`${BASE}/maps`);
  return res.json();
}

export async function fetchDates() {
  const res = await fetch(`${BASE}/dates`);
  return res.json();
}

export async function fetchMatches(mapId, date) {
  const params = new URLSearchParams();
  if (mapId) params.set('map', mapId);
  if (date) params.set('date', date);
  const res = await fetch(`${BASE}/matches?${params}`);
  return res.json();
}

export async function fetchMatchDetail(matchId) {
  const res = await fetch(`${BASE}/match/${matchId}`);
  if (!res.ok) throw new Error('Match not found');
  return res.json();
}

export async function fetchHeatmap(mapId, eventType, gridSize) {
  const params = new URLSearchParams({ map: mapId, type: eventType, grid: String(gridSize) });
  const res = await fetch(`${BASE}/heatmap?${params}`);
  return res.json();
}

export async function fetchStats() {
  const res = await fetch(`${BASE}/stats`);
  return res.json();
}
