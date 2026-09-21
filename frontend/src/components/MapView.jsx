import { useEffect, useRef, useState, useMemo } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import {
  MAP_CONFIGS,
  gameToLatLng,
  latLngToGame,
  EVENT_COLORS,
  formatTimestamp,
  shortId,
} from '../config';

export default function MapView({
  mapId,
  heatmap,
  matchDetail,
  selectedPlayer,
  currentTimeMs = null,
  filters = {
    humans: true,
    bots: true,
    trails: true,
    kills: true,
    deaths: true,
    storm: true,
    loot: true,
  },
}) {
  const mapRef = useRef(null);
  const leafletRef = useRef(null);
  const layersRef = useRef({
    minimap: null,
    heatmap: null,
    positions: null,
    trail: null,
    heads: null,
  });
  const hasFitBounds = useRef(false);
  const [cursorCoords, setCursorCoords] = useState(null);

  // Initialize Leaflet map with CRS.Simple
  useEffect(() => {
    if (!leafletRef.current && mapRef.current) {
      const map = L.map(mapRef.current, {
        crs: L.CRS.Simple,
        minZoom: -2,
        maxZoom: 4,
        zoomSnap: 0.25,
        zoomDelta: 0.5,
        attributionControl: false,
        zoomControl: true,
      });
      map.setView([512, 512], 0);

      map.on('mousemove', (e) => {
        setCursorCoords(e.latlng);
      });
      map.on('mouseout', () => {
        setCursorCoords(null);
      });

      leafletRef.current = map;
    }
  }, []);

  // Update background minimap image overlay
  useEffect(() => {
    const map = leafletRef.current;
    if (!map) return;
    const cfg = MAP_CONFIGS[mapId];
    if (!cfg) return;

    if (layersRef.current.minimap) {
      map.removeLayer(layersRef.current.minimap);
    }
    layersRef.current.minimap = L.imageOverlay(
      cfg.minimap,
      [[0, 0], [1024, 1024]],
      { opacity: 1 }
    ).addTo(map);

    if (!hasFitBounds.current) {
      map.setView([512, 512], 0);
      hasFitBounds.current = true;
    }
  }, [mapId]);

  // Update Heatmap overlay
  useEffect(() => {
    const map = leafletRef.current;
    if (!map) return;

    if (layersRef.current.heatmap) {
      map.removeLayer(layersRef.current.heatmap);
      layersRef.current.heatmap = null;
    }

    if (!heatmap || !heatmap.cells || heatmap.cells.length === 0) return;

    const maxIntensity = Math.max(...heatmap.cells.map((c) => c.intensity), 1);
    const markers = [];
    const gridSize = heatmap.grid_size || 64;

    for (const cell of heatmap.cells) {
      if (cell.intensity <= 0) continue;
      const ratio = cell.intensity / maxIntensity;
      const color = getHeatColor(ratio);
      const cx = cell.px + gridSize / 2;
      // cell.py is in image pixel space (0 at top, 1024 at bottom).
      // In Leaflet CRS.Simple, lat is 0 at bottom, 1024 at top.
      const cy = 1024 - (cell.py + gridSize / 2);
      const radius = (gridSize / 2) * 0.95;

      markers.push(
        L.circleMarker([cy, cx], {
          radius,
          fillColor: color,
          color: 'transparent',
          weight: 0,
          fillOpacity: 0.65,
        })
      );
    }

    if (markers.length > 0) {
      layersRef.current.heatmap = L.layerGroup(markers).addTo(map);
    }
  }, [mapId, heatmap]);

  // Filter events up to currentTimeMs if playback is enabled
  const visibleEvents = useMemo(() => {
    if (!matchDetail || !matchDetail.events) return [];
    if (currentTimeMs === null || currentTimeMs === undefined) {
      return matchDetail.events;
    }
    const cutoffTs = (matchDetail.events[0]?.ts || 0) + currentTimeMs;
    return matchDetail.events.filter((e) => e.ts <= cutoffTs);
  }, [matchDetail, currentTimeMs]);

  // Update Trails and Event Markers
  useEffect(() => {
    const map = leafletRef.current;
    if (!map) return;

    if (layersRef.current.positions) map.removeLayer(layersRef.current.positions);
    if (layersRef.current.trail) map.removeLayer(layersRef.current.trail);
    if (layersRef.current.heads) map.removeLayer(layersRef.current.heads);

    if (!matchDetail || visibleEvents.length === 0) return;

    const currentMapId = matchDetail.map_id || mapId;
    const markers = [];
    const headMarkers = [];
    const trailLayers = [];

    // Group movement events by player
    const playerPaths = {};
    for (const evt of visibleEvents) {
      const isPos = evt.event === 'Position' || evt.event === 'BotPosition';
      if (!isPos) continue;

      const isHuman = evt.user_id.length > 6;
      if (isHuman && !filters.humans) continue;
      if (!isHuman && !filters.bots) continue;

      if (!playerPaths[evt.user_id]) {
        playerPaths[evt.user_id] = [];
      }
      playerPaths[evt.user_id].push(evt);
    }

    // Determine trails to draw
    const shouldDrawTrailFor = (uid) => {
      if (!filters.trails) return false;
      if (selectedPlayer) return uid === selectedPlayer;
      // If no player selected, draw trails for all visible players
      return true;
    };

    Object.entries(playerPaths).forEach(([uid, positions]) => {
      if (!shouldDrawTrailFor(uid)) return;

      const isHuman = uid.length > 6;
      const trailColor = isHuman ? EVENT_COLORS.Position : EVENT_COLORS.BotPosition;
      const isSelected = uid === selectedPlayer;

      const latlngs = positions.map((p) => gameToLatLng(p.x, p.z, currentMapId));
      if (latlngs.length > 1) {
        trailLayers.push(
          L.polyline(latlngs, {
            color: trailColor,
            weight: isSelected ? 3 : 1.5,
            opacity: isSelected ? 0.9 : 0.45,
            dashArray: isSelected ? '5,4' : '3,3',
            smoothFactor: 1,
          })
        );
      }

      // If playback is active or player is selected, mark current head position
      if (positions.length > 0) {
        const lastPos = positions[positions.length - 1];
        const headLatLng = gameToLatLng(lastPos.x, lastPos.z, currentMapId);

        headMarkers.push(
          L.circleMarker(headLatLng, {
            radius: isSelected ? 6 : 4,
            fillColor: trailColor,
            color: '#ffffff',
            weight: isSelected ? 2 : 1,
            fillOpacity: 1,
            opacity: 1,
          }).bindTooltip(
            `<div style="font-size:11px;font-family:monospace;padding:2px">
              <b>${isHuman ? '👤 Human' : '🤖 Bot'}:</b> ${shortId(uid)}<br/>
              <span>World: (${lastPos.x.toFixed(1)}, ${lastPos.z.toFixed(1)})</span><br/>
              <span style="color:#8b949e">${formatTimestamp(lastPos.ts)}</span>
            </div>`,
            { direction: 'top', offset: [0, -6] }
          )
        );
      }
    });

    // Discrete Combat & Loot events
    for (const evt of visibleEvents) {
      const isHuman = evt.user_id.length > 6;
      if (isHuman && !filters.humans) continue;
      if (!isHuman && !filters.bots) continue;

      if (selectedPlayer && evt.user_id !== selectedPlayer) {
        // In single-player focus mode, only show their relevant events
        continue;
      }

      const latlng = gameToLatLng(evt.x, evt.z, currentMapId);
      const isKill = evt.event === 'Kill' || evt.event === 'BotKill';
      const isDeath = evt.event === 'Killed' || evt.event === 'BotKilled';
      const isStorm = evt.event === 'KilledByStorm';
      const isLoot = evt.event === 'Loot';

      if (isKill && filters.kills) {
        markers.push(
          L.circleMarker(latlng, {
            radius: 6,
            fillColor: EVENT_COLORS.Kill,
            color: '#fff',
            weight: 1.5,
            fillOpacity: 0.95,
            opacity: 1,
          }).bindTooltip(
            `<div style="font-size:11px;font-family:monospace">
              <span style="color:${EVENT_COLORS.Kill}">⚔️ ${evt.event}</span><br/>
              <b>By:</b> ${shortId(evt.user_id)}<br/>
              x: ${evt.x.toFixed(1)}, z: ${evt.z.toFixed(1)}<br/>
              <span style="color:#8b949e">${formatTimestamp(evt.ts)}</span>
            </div>`,
            { direction: 'top', offset: [0, -6] }
          )
        );
      } else if (isStorm && filters.storm) {
        markers.push(
          L.circleMarker(latlng, {
            radius: 7,
            fillColor: EVENT_COLORS.KilledByStorm,
            color: '#ffffff',
            weight: 2,
            fillOpacity: 0.95,
            opacity: 1,
            className: 'storm-death-marker',
          }).bindTooltip(
            `<div style="font-size:11px;font-family:monospace">
              <span style="color:${EVENT_COLORS.KilledByStorm}">⚡ KilledByStorm</span><br/>
              <b>Player:</b> ${shortId(evt.user_id)}<br/>
              x: ${evt.x.toFixed(1)}, z: ${evt.z.toFixed(1)}<br/>
              <span style="color:#8b949e">${formatTimestamp(evt.ts)}</span>
            </div>`,
            { direction: 'top', offset: [0, -6] }
          )
        );
      } else if (isDeath && filters.deaths) {
        markers.push(
          L.circleMarker(latlng, {
            radius: 6,
            fillColor: EVENT_COLORS.Killed,
            color: '#ffffff',
            weight: 1.5,
            fillOpacity: 0.9,
            opacity: 1,
          }).bindTooltip(
            `<div style="font-size:11px;font-family:monospace">
              <span style="color:${EVENT_COLORS.Killed}">💀 ${evt.event}</span><br/>
              <b>Player:</b> ${shortId(evt.user_id)}<br/>
              x: ${evt.x.toFixed(1)}, z: ${evt.z.toFixed(1)}<br/>
              <span style="color:#8b949e">${formatTimestamp(evt.ts)}</span>
            </div>`,
            { direction: 'top', offset: [0, -6] }
          )
        );
      } else if (isLoot && filters.loot) {
        markers.push(
          L.circleMarker(latlng, {
            radius: 4,
            fillColor: EVENT_COLORS.Loot,
            color: '#e3b341',
            weight: 1,
            fillOpacity: 0.85,
            opacity: 0.9,
          }).bindTooltip(
            `<div style="font-size:11px;font-family:monospace">
              <span style="color:${EVENT_COLORS.Loot}">📦 Loot Pickup</span><br/>
              <b>By:</b> ${shortId(evt.user_id)}<br/>
              x: ${evt.x.toFixed(1)}, z: ${evt.z.toFixed(1)}<br/>
              <span style="color:#8b949e">${formatTimestamp(evt.ts)}</span>
            </div>`,
            { direction: 'top', offset: [0, -5] }
          )
        );
      }
    }

    if (trailLayers.length > 0) {
      layersRef.current.trail = L.layerGroup(trailLayers).addTo(map);
    }
    if (markers.length > 0) {
      layersRef.current.positions = L.layerGroup(markers).addTo(map);
    }
    if (headMarkers.length > 0) {
      layersRef.current.heads = L.layerGroup(headMarkers).addTo(map);
    }
  }, [matchDetail, visibleEvents, selectedPlayer, filters, mapId]);

  // Fit bounds when selecting a match
  useEffect(() => {
    const map = leafletRef.current;
    if (!map || !matchDetail) return;
    map.fitBounds([[0, 0], [1024, 1024]], { animate: false });
  }, [matchDetail]);

  const activeMapId = matchDetail?.map_id || mapId;
  const worldPos = cursorCoords ? latLngToGame(cursorCoords.lat, cursorCoords.lng, activeMapId) : null;

  return (
    <div className="map-container-rel">
      <div ref={mapRef} className="leaflet-container" />

      {/* Level Designer Coordinate Inspector HUD */}
      {worldPos && (
        <div className="coord-inspector-hud">
          <span className="coord-map">{activeMapId}</span>
          <span className="coord-val">X: <b>{worldPos.x.toFixed(1)}</b></span>
          <span className="coord-val">Z: <b>{worldPos.z.toFixed(1)}</b></span>
          <span className="coord-uv">
            (UV: {(cursorCoords.lng / 1024).toFixed(3)}, {(cursorCoords.lat / 1024).toFixed(3)})
          </span>
        </div>
      )}
    </div>
  );
}

function getHeatColor(ratio) {
  if (ratio < 0.2) return `rgba(0, 0, 255, ${0.25 + ratio * 2})`;
  if (ratio < 0.4) return `rgba(0, ${Math.floor(120 + (ratio - 0.2) * 500)}, 255, 0.65)`;
  if (ratio < 0.6) return `rgba(0, 255, ${Math.floor(255 - (ratio - 0.4) * 500)}, 0.75)`;
  if (ratio < 0.8) return `rgba(${Math.floor((ratio - 0.6) * 1275)}, 255, 0, 0.85)`;
  return `rgba(255, ${Math.floor(255 - (ratio - 0.8) * 1275)}, 0, 0.95)`;
}
