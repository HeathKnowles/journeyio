export const MAP_CONFIGS = {
  AmbroseValley: { scale: 900, origin_x: -370, origin_z: -473, name: 'Ambrose Valley', minimap: '/minimaps/AmbroseValley_Minimap.png' },
  GrandRift: { scale: 581, origin_x: -290, origin_z: -290, name: 'Grand Rift', minimap: '/minimaps/GrandRift_Minimap.png' },
  Lockdown: { scale: 1000, origin_x: -500, origin_z: -500, name: 'Lockdown', minimap: '/minimaps/Lockdown_Minimap.jpg' },
};

export const EVENT_COLORS = {
  Position: '#58a6ff',
  BotPosition: '#f0883e',
  Kill: '#f85149',
  Killed: '#da3633',
  BotKill: '#2ea043',
  BotKilled: '#f85149',
  KilledByStorm: '#bc8cff',
  Loot: '#e3b341',
};

export const HEATMAP_TYPES = [
  { value: 'traffic', label: 'Traffic (Movement)' },
  { value: 'kill', label: 'Kill Hotspots' },
  { value: 'death', label: 'Death Zones' },
  { value: 'loot', label: 'Loot Density' },
  { value: 'storm', label: 'Storm Fatalities' },
];

export function gameToPixel(x, z, mapId) {
  const cfg = MAP_CONFIGS[mapId];
  if (!cfg) return { px: 0, py: 0 };
  const u = (x - cfg.origin_x) / cfg.scale;
  const v = (z - cfg.origin_z) / cfg.scale;
  return { px: u * 1024, py: (1 - v) * 1024 };
}

export function gameToLatLng(x, z, mapId) {
  const cfg = MAP_CONFIGS[mapId];
  if (!cfg) return [0, 0];
  const u = (x - cfg.origin_x) / cfg.scale;
  const v = (z - cfg.origin_z) / cfg.scale;
  // In Leaflet CRS.Simple:
  // Lat increases UPWARDS (0 at south, 1024 at north).
  // Lng increases RIGHTWARDS (0 at west, 1024 at east).
  return [v * 1024, u * 1024];
}

export function latLngToGame(lat, lng, mapId) {
  const cfg = MAP_CONFIGS[mapId];
  if (!cfg) return { x: 0, z: 0 };
  const u = lng / 1024;
  const v = lat / 1024;
  const x = u * cfg.scale + cfg.origin_x;
  const z = v * cfg.scale + cfg.origin_z;
  return { x, z };
}

export function formatTimestamp(ts) {
  const d = new Date(ts);
  return d.toLocaleString();
}

export function formatTime(ts) {
  const d = new Date(ts);
  const h = d.getHours().toString().padStart(2, '0');
  const m = d.getMinutes().toString().padStart(2, '0');
  const s = d.getSeconds().toString().padStart(2, '0');
  return `${h}:${m}:${s}`;
}

export function formatDuration(ms) {
  const sec = Math.floor(ms / 1000);
  const min = Math.floor(sec / 60);
  return `${min}m ${sec % 60}s`;
}

export function shortId(id) {
  if (!id) return '';
  if (id.length <= 12) return id;
  return id.substring(0, 8) + '...';
}

export function matchLabel(index, match) {
  const time = formatTime(match.start_ts);
  return `#${index + 1} - ${time}`;
}
