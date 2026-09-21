import { useState, useMemo } from 'react';
import { formatDuration, formatTime, shortId, HEATMAP_TYPES } from '../config';

export default function Sidebar({
  maps,
  dates,
  matches,
  matchDetail,
  selectedMap,
  selectedDate,
  selectedMatch,
  selectedPlayer,
  onMapChange,
  onDateChange,
  onMatchSelect,
  onPlayerSelect,
  loading,
  showHeatmap,
  onToggleHeatmap,
  heatmapType,
  onHeatmapTypeChange,
  panelOpen,
  onClose,
  stats,
  filters,
  onFilterChange,
}) {
  const [sortBy, setSortBy] = useState('events');
  const [playerSearch, setPlayerSearch] = useState('');
  const [matchSearch, setMatchSearch] = useState('');

  const players = matchDetail?.players || [];
  const humanPlayers = players.filter((p) => p.is_human);
  const botPlayers = players.filter((p) => !p.is_human);

  // Match Level Design telemetry metrics
  const matchMetrics = useMemo(() => {
    if (!matchDetail || !matchDetail.events) return null;
    const evts = matchDetail.events;
    const kills = evts.filter((e) => e.event === 'Kill' || e.event === 'BotKill').length;
    const deaths = evts.filter((e) => e.event === 'Killed' || e.event === 'BotKilled').length;
    const stormDeaths = evts.filter((e) => e.event === 'KilledByStorm').length;
    const loot = evts.filter((e) => e.event === 'Loot').length;
    const totalPlayers = players.length || 1;
    const stormPct = Math.round((stormDeaths / totalPlayers) * 100);

    return {
      kills,
      deaths,
      stormDeaths,
      stormPct,
      loot,
      totalPlayers,
      durationMs: matchDetail.duration_ms,
    };
  }, [matchDetail, players]);

  // Selected player stats
  const selectedPlayerStats = useMemo(() => {
    if (!matchDetail || !selectedPlayer) return null;
    const playerEvts = matchDetail.events.filter((e) => e.user_id === selectedPlayer);
    const summary = players.find((p) => p.user_id === selectedPlayer);
    const kills = playerEvts.filter((e) => e.event === 'Kill' || e.event === 'BotKill').length;
    const loot = playerEvts.filter((e) => e.event === 'Loot').length;
    const isStormDeath = playerEvts.some((e) => e.event === 'KilledByStorm');
    const isCombatDeath = playerEvts.some((e) => e.event === 'Killed' || e.event === 'BotKilled');

    let outcome = 'Survived / Extracted';
    let outcomeColor = 'var(--success)';
    if (isStormDeath) {
      outcome = 'Killed by Storm';
      outcomeColor = '#bc8cff';
    } else if (isCombatDeath) {
      outcome = 'Killed in Combat';
      outcomeColor = 'var(--danger)';
    }

    return {
      userId: selectedPlayer,
      isHuman: summary?.is_human ?? selectedPlayer.length > 6,
      eventsCount: playerEvts.length,
      kills,
      loot,
      outcome,
      outcomeColor,
    };
  }, [matchDetail, selectedPlayer, players]);

  const sortedMatches = useMemo(() => {
    let list = [...matches];
    if (matchSearch) {
      const q = matchSearch.toLowerCase();
      list = list.filter(
        (m) =>
          m.match_id.toLowerCase().includes(q) ||
          m.map_id.toLowerCase().includes(q)
      );
    }
    switch (sortBy) {
      case 'events':
        list.sort((a, b) => b.total_events - a.total_events);
        break;
      case 'duration':
        list.sort((a, b) => (b.end_ts - b.start_ts) - (a.end_ts - a.start_ts));
        break;
      case 'players':
        list.sort((a, b) => b.human_count - a.human_count);
        break;
      case 'time':
        list.sort((a, b) => a.start_ts - b.start_ts);
        break;
    }
    return list;
  }, [matches, sortBy, matchSearch]);

  const filteredHumans = useMemo(() => {
    if (!playerSearch) return humanPlayers;
    const q = playerSearch.toLowerCase();
    return humanPlayers.filter((p) => p.user_id.toLowerCase().includes(q));
  }, [humanPlayers, playerSearch]);

  const filteredBots = useMemo(() => {
    if (!playerSearch) return botPlayers;
    const q = playerSearch.toLowerCase();
    return botPlayers.filter((p) => p.user_id.toLowerCase().includes(q));
  }, [botPlayers, playerSearch]);

  const toggleFilter = (key) => {
    if (!onFilterChange) return;
    onFilterChange({ ...filters, [key]: !filters[key] });
  };

  const panelClass = `panel ${panelOpen ? '' : 'closed'}`;

  return (
    <div className={panelClass}>
      <div className="panel-header">
        <h2>LILA BLACK VISUALIZER</h2>
        {stats && (
          <span style={{ fontSize: 10, color: 'var(--text-secondary)' }}>
            {stats.total_events.toLocaleString()} ev &middot; {stats.total_matches} m
          </span>
        )}
        <button className="panel-close" onClick={onClose}>
          &times;
        </button>
      </div>

      <div className="panel-body">
        {/* Map & Date Filter */}
        <div className="panel-section">
          <h3>Map & Date</h3>
          <div className="control-row">
            <select value={selectedMap} onChange={(e) => onMapChange(e.target.value)}>
              <option value="">All Maps (3 Maps)</option>
              {maps.map((m) => (
                <option key={m.name} value={m.name}>
                  {m.name}
                </option>
              ))}
            </select>
          </div>
          <div className="control-row">
            <select value={selectedDate} onChange={(e) => onDateChange(e.target.value)}>
              <option value="">All Dates (5 Days)</option>
              {dates.map((d) => (
                <option key={d} value={d}>
                  {d.replace('_', ' ')}
                </option>
              ))}
            </select>
          </div>
        </div>

        {/* Heatmap Overlay Controls */}
        <div className="panel-section">
          <h3>Heatmap Analysis</h3>
          <div className="control-row">
            <select
              value={heatmapType}
              onChange={(e) => onHeatmapTypeChange(e.target.value)}
            >
              {HEATMAP_TYPES.map((t) => (
                <option key={t.value} value={t.value}>
                  {t.label}
                </option>
              ))}
            </select>
            <button
              className={`btn ${showHeatmap ? 'active' : ''}`}
              onClick={onToggleHeatmap}
            >
              {showHeatmap ? 'Visible' : 'Hidden'}
            </button>
          </div>
        </div>

        {/* Level Designer Layer Toggles */}
        <div className="panel-section">
          <h3>Level Designer Layers</h3>
          <div className="layer-toggles-grid">
            <button
              className={`layer-chip ${filters?.humans ? 'active human' : ''}`}
              onClick={() => toggleFilter('humans')}
            >
              👤 Humans
            </button>
            <button
              className={`layer-chip ${filters?.bots ? 'active bot' : ''}`}
              onClick={() => toggleFilter('bots')}
            >
              🤖 Bots
            </button>
            <button
              className={`layer-chip ${filters?.trails ? 'active trail' : ''}`}
              onClick={() => toggleFilter('trails')}
            >
              〰️ Paths
            </button>
            <button
              className={`layer-chip ${filters?.kills ? 'active kill' : ''}`}
              onClick={() => toggleFilter('kills')}
            >
              ⚔️ Kills
            </button>
            <button
              className={`layer-chip ${filters?.deaths ? 'active death' : ''}`}
              onClick={() => toggleFilter('deaths')}
            >
              💀 Deaths
            </button>
            <button
              className={`layer-chip ${filters?.storm ? 'active storm' : ''}`}
              onClick={() => toggleFilter('storm')}
            >
              ⚡ Storm
            </button>
            <button
              className={`layer-chip ${filters?.loot ? 'active loot' : ''}`}
              onClick={() => toggleFilter('loot')}
            >
              📦 Loot
            </button>
          </div>
        </div>

        {/* Selected Match Telemetry Card */}
        {matchMetrics && (
          <div className="panel-section metrics-card">
            <h3>Level Design Telemetry</h3>
            <div className="metrics-grid">
              <div className="metric-box">
                <span className="metric-label">Duration</span>
                <span className="metric-num">{formatDuration(matchMetrics.durationMs)}</span>
              </div>
              <div className="metric-box">
                <span className="metric-label">Kills</span>
                <span className="metric-num text-danger">{matchMetrics.kills}</span>
              </div>
              <div className="metric-box">
                <span className="metric-label">Storm Fatalities</span>
                <span className="metric-num text-storm">
                  {matchMetrics.stormDeaths} ({matchMetrics.stormPct}%)
                </span>
              </div>
              <div className="metric-box">
                <span className="metric-label">Loot Pickups</span>
                <span className="metric-num text-loot">{matchMetrics.loot}</span>
              </div>
            </div>
          </div>
        )}

        {/* Selected Player Focus Card */}
        {selectedPlayerStats && (
          <div className="panel-section player-focus-card">
            <div className="player-focus-header">
              <h3>Player Focus</h3>
              <button
                className="btn-clear-player"
                onClick={() => onPlayerSelect(null)}
                title="Deselect Player"
              >
                Clear Focus
              </button>
            </div>
            <div className="player-focus-body">
              <div className="player-focus-id">
                <span className={`player-dot ${selectedPlayerStats.isHuman ? 'human' : 'bot'}`} />
                <span>{selectedPlayerStats.userId}</span>
              </div>
              <div className="player-focus-outcome" style={{ color: selectedPlayerStats.outcomeColor }}>
                Outcome: <b>{selectedPlayerStats.outcome}</b>
              </div>
              <div className="player-focus-stats-row">
                <span>⚔️ {selectedPlayerStats.kills} kills</span>
                <span>📦 {selectedPlayerStats.loot} loot</span>
                <span>📍 {selectedPlayerStats.eventsCount} events</span>
              </div>
            </div>
          </div>
        )}

        {/* Matches Browser */}
        <div className="panel-section">
          <h3>
            Matches <span className="badge">{sortedMatches.length}</span>
          </h3>
          <input
            type="text"
            className="search-input"
            placeholder="Search match ID or map..."
            value={matchSearch}
            onChange={(e) => setMatchSearch(e.target.value)}
          />
          <div className="sort-bar">
            {[
              { key: 'events', label: 'Events' },
              { key: 'duration', label: 'Duration' },
              { key: 'players', label: 'Players' },
              { key: 'time', label: 'Time' },
            ].map((s) => (
              <button
                key={s.key}
                className={`btn ${sortBy === s.key ? 'active' : ''}`}
                onClick={() => setSortBy(s.key)}
              >
                {s.label}
              </button>
            ))}
          </div>

          {loading && (
            <div className="sidebar-loading">
              <div className="spinner" /> Loading matches...
            </div>
          )}

          {!loading && sortedMatches.length === 0 && (
            <div className="empty-msg">No matches found</div>
          )}

          {!loading && sortedMatches.length > 0 && (
            <div className="match-list">
              {sortedMatches.map((m) => {
                const origIdx = matches.indexOf(m);
                return (
                  <div
                    key={m.match_id}
                    className={`match-item ${selectedMatch === m.match_id ? 'selected' : ''}`}
                    onClick={() => onMatchSelect(m.match_id)}
                  >
                    <div className="match-row1">
                      <span className="match-label">#{origIdx + 1}</span>
                      <span className="match-time">{formatTime(m.start_ts)}</span>
                    </div>
                    <div className="match-row2">
                      <span>{m.map_id}</span>
                      <span className="humans">{m.human_count}H</span>
                      <span className="bots">{m.bot_count}B</span>
                      <span>{m.total_events}ev</span>
                      <span>{formatDuration(m.end_ts - m.start_ts)}</span>
                    </div>
                  </div>
                );
              })}
            </div>
          )}
        </div>

        {/* Match Players Browser */}
        {players.length > 0 && (
          <div className="panel-section">
            <h3>
              Match Roster <span className="badge">{players.length}</span>
            </h3>
            <input
              type="text"
              className="search-input"
              placeholder="Search player..."
              value={playerSearch}
              onChange={(e) => setPlayerSearch(e.target.value)}
            />

            <h3>
              Humans <span className="badge">{filteredHumans.length}</span>
            </h3>
            <div className="player-list">
              {filteredHumans.map((p) => (
                <div
                  key={p.user_id}
                  className={`player-item ${selectedPlayer === p.user_id ? 'selected' : ''}`}
                  onClick={() =>
                    onPlayerSelect(selectedPlayer === p.user_id ? null : p.user_id)
                  }
                >
                  <span className="player-dot human" />
                  <span className="player-name" title={p.user_id}>
                    {shortId(p.user_id)}
                  </span>
                  {p.death_event && p.death_event !== 'Unknown' && (
                    <span
                      className="player-death"
                      title={`Died: ${p.death_event}`}
                    >
                      &#10005;
                    </span>
                  )}
                  <span className="player-stats">{p.event_count}</span>
                </div>
              ))}
            </div>

            <h3>
              Bots <span className="badge">{filteredBots.length}</span>
            </h3>
            <div className="player-list">
              {filteredBots.map((p) => (
                <div
                  key={p.user_id}
                  className={`player-item ${selectedPlayer === p.user_id ? 'selected' : ''}`}
                  onClick={() =>
                    onPlayerSelect(selectedPlayer === p.user_id ? null : p.user_id)
                  }
                >
                  <span className="player-dot bot" />
                  <span className="player-name" title={p.user_id}>
                    {shortId(p.user_id)}
                  </span>
                  {p.death_event && p.death_event !== 'Unknown' && (
                    <span
                      className="player-death"
                      title={`Died: ${p.death_event}`}
                    >
                      &#10005;
                    </span>
                  )}
                  <span className="player-stats">{p.event_count}</span>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
