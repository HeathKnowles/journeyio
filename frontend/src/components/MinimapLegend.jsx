import { EVENT_COLORS } from '../config';

export default function MinimapLegend() {
  return (
    <div className="map-legend top-right">
      <h4>Telemetry Legend</h4>
      <div className="legend-item">
        <span className="legend-dot" style={{ background: EVENT_COLORS.Position }} />
        <span>Human Player</span>
      </div>
      <div className="legend-item">
        <span className="legend-dot" style={{ background: EVENT_COLORS.BotPosition }} />
        <span>Bot AI</span>
      </div>
      <div className="legend-item">
        <span className="legend-dot" style={{ background: EVENT_COLORS.Kill }} />
        <span>Kill Hotspot</span>
      </div>
      <div className="legend-item">
        <span className="legend-dot" style={{ background: EVENT_COLORS.Killed }} />
        <span>Combat Death</span>
      </div>
      <div className="legend-item">
        <span className="legend-dot" style={{ background: EVENT_COLORS.KilledByStorm }} />
        <span>Storm Fatality</span>
      </div>
      <div className="legend-item">
        <span className="legend-dot" style={{ background: EVENT_COLORS.Loot }} />
        <span>Loot Pickup</span>
      </div>
    </div>
  );
}
