export default function HeatmapLegend({ visible }) {
  if (!visible) return null;
  return (
    <div className="map-legend bottom-right">
      <h4>Heatmap</h4>
      <div className="gradient-bar" style={{
        background: 'linear-gradient(to right, rgba(0,0,255,0.4), rgba(0,200,255,0.6), rgba(0,255,100,0.7), rgba(255,200,0,0.8), rgba(255,0,0,0.95))',
      }} />
      <div className="gradient-labels">
        <span>Low</span>
        <span>High</span>
      </div>
    </div>
  );
}
