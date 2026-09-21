import { useState, useEffect, useCallback, useRef, useMemo } from 'react';
import './index.css';
import {
  fetchMaps,
  fetchDates,
  fetchMatches,
  fetchMatchDetail,
  fetchHeatmap,
  fetchStats,
} from './api/client';
import MapView from './components/MapView';
import Sidebar from './components/Sidebar';
import MinimapLegend from './components/MinimapLegend';
import HeatmapLegend from './components/HeatmapLegend';
import TimelineControl from './components/TimelineControl';

function App() {
  const [stats, setStats] = useState(null);
  const [maps, setMaps] = useState([]);
  const [dates, setDates] = useState([]);
  const [matches, setMatches] = useState([]);
  const [matchDetail, setMatchDetail] = useState(null);
  const [heatmap, setHeatmap] = useState(null);

  const [selectedMap, setSelectedMap] = useState('');
  const [selectedDate, setSelectedDate] = useState('');
  const [selectedMatch, setSelectedMatch] = useState('');
  const [selectedPlayer, setSelectedPlayer] = useState(null);
  const [showHeatmap, setShowHeatmap] = useState(true);
  const [heatmapType, setHeatmapType] = useState('traffic');
  const [loading, setLoading] = useState(false);
  const [matchLoading, setMatchLoading] = useState(false);
  const [panelOpen, setPanelOpen] = useState(true);

  // Playback & Timeline state
  const [currentTimeMs, setCurrentTimeMs] = useState(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [playbackSpeed, setPlaybackSpeed] = useState(1);

  // Level Designer layer filters
  const [filters, setFilters] = useState({
    humans: true,
    bots: true,
    trails: true,
    kills: true,
    deaths: true,
    storm: true,
    loot: true,
  });

  const matchListRef = useRef([]);
  const selectedMatchRef = useRef(selectedMatch);

  useEffect(() => {
    selectedMatchRef.current = selectedMatch;
  }, [selectedMatch]);

  useEffect(() => {
    fetchStats().then(setStats).catch(console.error);
    fetchMaps().then(setMaps).catch(console.error);
    fetchDates().then(setDates).catch(console.error);
  }, []);

  useEffect(() => {
    setLoading(true);
    fetchMatches(selectedMap, selectedDate)
      .then((data) => {
        setMatches(data);
        matchListRef.current = data;
      })
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [selectedMap, selectedDate]);

  useEffect(() => {
    if (selectedMatch) {
      setMatchLoading(true);
      setIsPlaying(false);
      fetchMatchDetail(selectedMatch)
        .then((detail) => {
          setMatchDetail(detail);
          setCurrentTimeMs(detail.duration_ms || 0);
        })
        .catch(console.error)
        .finally(() => setMatchLoading(false));
    } else {
      setMatchDetail(null);
      setCurrentTimeMs(null);
      setIsPlaying(false);
    }
    setSelectedPlayer(null);
  }, [selectedMatch]);

  // Heatmap fetcher
  useEffect(() => {
    const mapId = selectedMap || (matchDetail ? matchDetail.map_id : 'AmbroseValley');
    if (showHeatmap && mapId) {
      fetchHeatmap(mapId, heatmapType, 64)
        .then(setHeatmap)
        .catch(console.error);
    } else {
      setHeatmap(null);
    }
  }, [selectedMap, matchDetail, showHeatmap, heatmapType]);

  // Timeline playback animation loop
  useEffect(() => {
    let animationId;
    let lastTime = performance.now();

    const animate = (now) => {
      if (!isPlaying || !matchDetail?.duration_ms) return;
      const deltaMs = now - lastTime;
      lastTime = now;

      setCurrentTimeMs((prev) => {
        const current = prev ?? 0;
        const next = current + deltaMs * playbackSpeed;
        if (next >= matchDetail.duration_ms) {
          setIsPlaying(false);
          return matchDetail.duration_ms;
        }
        return next;
      });

      animationId = requestAnimationFrame(animate);
    };

    if (isPlaying) {
      lastTime = performance.now();
      animationId = requestAnimationFrame(animate);
    }

    return () => {
      if (animationId) cancelAnimationFrame(animationId);
    };
  }, [isPlaying, playbackSpeed, matchDetail]);

  const handleTogglePlay = useCallback(() => {
    if (!matchDetail?.duration_ms) return;
    setIsPlaying((prev) => {
      if (!prev) {
        // If at the end, restart from beginning
        setCurrentTimeMs((curr) =>
          curr >= matchDetail.duration_ms ? 0 : curr
        );
      }
      return !prev;
    });
  }, [matchDetail]);

  const handleSeek = useCallback((ms) => {
    setCurrentTimeMs(ms);
  }, []);

  const handleReset = useCallback(() => {
    setIsPlaying(false);
    setCurrentTimeMs(0);
  }, []);

  const handleMatchSelect = useCallback((matchId) => {
    setSelectedMatch((prev) => (prev === matchId ? '' : matchId));
  }, []);

  // Compute live match stats for current timeline position
  const aliveStats = useMemo(() => {
    if (!matchDetail?.events || currentTimeMs === null) return null;
    const startTs = matchDetail.events[0]?.ts || 0;
    const cutoffTs = startTs + currentTimeMs;

    const playerStatus = {};
    let lootCount = 0;

    for (const e of matchDetail.events) {
      if (e.ts > cutoffTs) continue;
      if (e.event === 'Loot') lootCount++;

      if (!playerStatus[e.user_id]) {
        playerStatus[e.user_id] = {
          isHuman: e.user_id.length > 6,
          isAlive: true,
        };
      }
      if (
        e.event === 'Killed' ||
        e.event === 'BotKilled' ||
        e.event === 'KilledByStorm'
      ) {
        playerStatus[e.user_id].isAlive = false;
      }
    }

    let humans = 0;
    let bots = 0;
    for (const st of Object.values(playerStatus)) {
      if (st.isAlive) {
        if (st.isHuman) humans++;
        else bots++;
      }
    }
    return { humans, bots, lootCount };
  }, [matchDetail, currentTimeMs]);

  // Keyboard navigation shortcuts
  useEffect(() => {
    const handleKeyDown = (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
      if (e.key === 'Escape') {
        setSelectedMatch('');
        setSelectedPlayer(null);
      }
      if (e.key === 'h' || e.key === 'H') setShowHeatmap((prev) => !prev);
      if (e.key === 'Tab') {
        e.preventDefault();
        setPanelOpen((prev) => !prev);
      }
      if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
        e.preventDefault();
        const list = matchListRef.current;
        if (list.length === 0) return;
        const idx = list.findIndex(
          (m) => m.match_id === selectedMatchRef.current
        );
        if (e.key === 'ArrowDown') {
          setSelectedMatch(list[(idx + 1) % list.length].match_id);
        } else {
          setSelectedMatch(
            list[(idx - 1 + list.length) % list.length].match_id
          );
        }
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  const activeMap =
    selectedMap || (matchDetail ? matchDetail.map_id : 'AmbroseValley');
  const showLegends = matchDetail || selectedPlayer;

  return (
    <div className="app">
      <div className="map-fullscreen">
        {matchLoading && (
          <div className="loading-overlay">
            <div style={{ textAlign: 'center' }}>
              <div className="spinner" />
              <div className="loading-text">Loading match telemetry...</div>
            </div>
          </div>
        )}
        <MapView
          mapId={activeMap}
          heatmap={showHeatmap ? heatmap : null}
          matchDetail={matchDetail}
          selectedPlayer={selectedPlayer}
          currentTimeMs={currentTimeMs}
          filters={filters}
        />
      </div>

      <button
        className={`open-btn ${panelOpen ? 'hidden' : ''}`}
        onClick={() => setPanelOpen(true)}
      >
        <span>&#9776;</span>
        <span className="label">Controls</span>
      </button>

      <Sidebar
        maps={maps}
        dates={dates}
        matches={matches}
        matchDetail={matchDetail}
        selectedMap={selectedMap}
        selectedDate={selectedDate}
        selectedMatch={selectedMatch}
        selectedPlayer={selectedPlayer}
        onMapChange={setSelectedMap}
        onDateChange={setSelectedDate}
        onMatchSelect={handleMatchSelect}
        onPlayerSelect={setSelectedPlayer}
        loading={loading}
        showHeatmap={showHeatmap}
        onToggleHeatmap={() => setShowHeatmap((p) => !p)}
        heatmapType={heatmapType}
        onHeatmapTypeChange={setHeatmapType}
        panelOpen={panelOpen}
        onClose={() => setPanelOpen(false)}
        stats={stats}
        filters={filters}
        onFilterChange={setFilters}
      />

      {/* Match Timeline / Playback Scrubber */}
      {matchDetail && matchDetail.duration_ms > 0 && (
        <TimelineControl
          durationMs={matchDetail.duration_ms}
          currentTimeMs={currentTimeMs ?? matchDetail.duration_ms}
          isPlaying={isPlaying}
          playbackSpeed={playbackSpeed}
          onTogglePlay={handleTogglePlay}
          onSeek={handleSeek}
          onReset={handleReset}
          onSpeedChange={setPlaybackSpeed}
          aliveStats={aliveStats}
        />
      )}

      {showLegends && <MinimapLegend />}
      <HeatmapLegend
        visible={
          showHeatmap &&
          heatmap &&
          heatmap.cells &&
          heatmap.cells.length > 0
        }
      />

      {!matchDetail && !matchLoading && !panelOpen && (
        <div className="empty-state">
          <h3>Press Tab or click Controls to start</h3>
          <p>
            <kbd>Tab</kbd> toggle panel &middot; <kbd>Space</kbd> play/pause &middot;{' '}
            <kbd>&uarr;</kbd>
            <kbd>&darr;</kbd> navigate &middot; <kbd>H</kbd> heatmap &middot;{' '}
            <kbd>Esc</kbd> deselect
          </p>
        </div>
      )}
    </div>
  );
}

export default App;
