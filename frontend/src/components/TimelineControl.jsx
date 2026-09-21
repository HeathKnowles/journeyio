import { useEffect } from 'react';
import { formatDuration } from '../config';

export default function TimelineControl({
  durationMs,
  currentTimeMs,
  isPlaying,
  playbackSpeed,
  onTogglePlay,
  onSeek,
  onReset,
  onSpeedChange,
  aliveStats,
}) {
  // Global spacebar to play/pause
  useEffect(() => {
    const handleKeyDown = (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
      if (e.code === 'Space') {
        e.preventDefault();
        onTogglePlay();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [onTogglePlay]);

  if (!durationMs || durationMs <= 0) return null;

  const currentSec = Math.floor(currentTimeMs / 1000);
  const totalSec = Math.floor(durationMs / 1000);
  const progressPercent = Math.min(100, Math.max(0, (currentTimeMs / durationMs) * 100));

  const formatMinSec = (sec) => {
    const m = Math.floor(sec / 60);
    const s = sec % 60;
    return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  };

  const speeds = [0.5, 1, 2, 5, 10];

  return (
    <div className="timeline-dock">
      <div className="timeline-main">
        <button
          className={`timeline-btn play-btn ${isPlaying ? 'playing' : ''}`}
          onClick={onTogglePlay}
          title={isPlaying ? 'Pause (Space)' : 'Play (Space)'}
        >
          {isPlaying ? '⏸' : '▶'}
        </button>

        <button
          className="timeline-btn reset-btn"
          onClick={onReset}
          title="Reset to Start"
        >
          ⏹
        </button>

        <div className="timeline-scrubber-wrap">
          <div className="timeline-time-label">
            <span className="current-time">{formatMinSec(currentSec)}</span>
            <span className="time-divider">/</span>
            <span className="total-time">{formatMinSec(totalSec)}</span>
          </div>

          <div className="timeline-slider-container">
            <input
              type="range"
              min="0"
              max={durationMs}
              value={currentTimeMs}
              onChange={(e) => onSeek(Number(e.target.value))}
              className="timeline-slider"
            />
            <div
              className="timeline-progress-fill"
              style={{ width: `${progressPercent}%` }}
            />
          </div>
        </div>

        <div className="speed-selector">
          {speeds.map((s) => (
            <button
              key={s}
              className={`speed-chip ${playbackSpeed === s ? 'active' : ''}`}
              onClick={() => onSpeedChange(s)}
            >
              {s}x
            </button>
          ))}
        </div>

        {aliveStats && (
          <div className="timeline-alive-badge">
            <span className="badge-item human-item" title="Alive Humans">
              👤 {aliveStats.humans}
            </span>
            <span className="badge-item bot-item" title="Alive Bots">
              🤖 {aliveStats.bots}
            </span>
            {aliveStats.lootCount > 0 && (
              <span className="badge-item loot-item" title="Loot collected so far">
                📦 {aliveStats.lootCount}
              </span>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
