/////////////////////////////////////////////////////////////////////////////
/* 
This file contains the html and javascript code to display the measurements 
as charts and cards on the webserver
*/
/////////////////////////////////////////////////////////////////////////////


#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <title>Air Monitor Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
    integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr"
    crossorigin="anonymous">
  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.min.js"></script>
  <link rel="icon" href="data:,">
  <style>
    html { font-family: Arial; display: inline-block; text-align: center; }
    p { font-size: 1.2rem; }
    body { margin: 0; }
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 1rem; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); }
    .reading { font-size: 1rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.particle { color: rgb(0, 110, 255); }
    .charts { max-width: 900px; margin: 20px auto; display: grid; grid-gap: 2rem; }
    .chart-card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); padding: 20px; }
    .chart-card h4 { margin-bottom: 10px; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>AIR MONITOR WEB SERVER</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p>
        <span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> HUMIDITY</h4><p>
        <span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p>
        <span class="reading"><span id="pres">%PRESSURE%</span> hPa</span></p>
      </div>
      <div class="card particle">
        <h4><i class="fas fa-wind"></i> TYPICAL PARTICLE SIZE</h4><p>
        <span class="reading"><span id="particle">%PARTICLE%</span> &micro;m</span></p>
      </div>
    </div>
  </div>
  <div class="charts">
    <div class="chart-card gas">
      <h4><i class="fas fa-wind"></i> GAS RESISTANCE &amp; VOC INDEX</h4>
      <canvas id="gasChart" height="100"></canvas>
    </div>
    <div class="chart-card">
      <h4><i class="fas fa-wind"></i> PARTICLE MASS CONCENTRATION (&micro;g/m&sup3;)</h4>
      <canvas id="massChart" height="100"></canvas>
    </div>
    <div class="chart-card">
      <h4><i class="fas fa-wind"></i> PARTICLE NUMBER CONCENTRATION (#/cm&sup3;)</h4>
      <canvas id="numberChart" height="100"></canvas>
    </div>
  </div>
  <script>
  if (!!window.EventSource) {
    var source = new EventSource('/events');

    const MAX_POINTS = 50;

    var combinedChart = new Chart(document.getElementById('gasChart'), {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          {
            label: 'Gas Resistance (kΩ)',
            data: [],
            borderColor: '#d62246',
            backgroundColor: '#d6224622',
            borderWidth: 2,
            pointRadius: 3,
            tension: 0.3,
            yAxisID: 'yGas'
          },
          {
            label: 'VOC Index',
            data: [],
            borderColor: '#270faf',
            backgroundColor: '#270faf22',
            borderWidth: 2,
            pointRadius: 3,
            tension: 0.3,
            yAxisID: 'yVoc'
          }
        ]
      },
      options: {
        animation: false,
        scales: {
          x: { display: false },
          yGas: {
            type: 'linear',
            position: 'left',
            beginAtZero: false,
            title: { display: true, text: 'Gas (kΩ)', color: '#d62246' }
          },
          yVoc: {
            type: 'linear',
            position: 'right',
            beginAtZero: false,
            title: { display: true, text: 'VOC Index', color: '#270faf' },
            grid: { drawOnChartArea: false }
          }
        }
      }
    });

    var massChart = new Chart(document.getElementById('massChart'), {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          { label: 'PM1.0',  data: [], borderColor: '#e63946', backgroundColor: '#e6394622', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'PM2.5',  data: [], borderColor: '#f4a261', backgroundColor: '#f4a26122', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'PM4.0',  data: [], borderColor: '#2a9d8f', backgroundColor: '#2a9d8f22', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'PM10.0', data: [], borderColor: '#264653', backgroundColor: '#26465322', borderWidth: 2, pointRadius: 3, tension: 0.3 }
        ]
      },
      options: { animation: false, scales: { x: { display: false }, y: { beginAtZero: true } } }
    });

    var numberChart = new Chart(document.getElementById('numberChart'), {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          { label: 'NC0.5',  data: [], borderColor: '#7400b8', backgroundColor: '#7400b822', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'NC1.0',  data: [], borderColor: '#5e60ce', backgroundColor: '#5e60ce22', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'NC2.5',  data: [], borderColor: '#4ea8de', backgroundColor: '#4ea8de22', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'NC4.0',  data: [], borderColor: '#56cfe1', backgroundColor: '#56cfe122', borderWidth: 2, pointRadius: 3, tension: 0.3 },
          { label: 'NC10.0', data: [], borderColor: '#1b998b', backgroundColor: '#1b998b22', borderWidth: 2, pointRadius: 3, tension: 0.3 }
        ]
      },
      options: { animation: false, scales: { x: { display: false }, y: { beginAtZero: true } } }
    });

    combinedChart.data.labels = Array(MAX_POINTS).fill('');
    combinedChart.data.datasets[0].data = Array(MAX_POINTS).fill(null);
    combinedChart.data.datasets[1].data = Array(MAX_POINTS).fill(null);
    combinedChart.update();

    massChart.data.labels = Array(MAX_POINTS).fill('');
    massChart.data.datasets.forEach(function(ds) { ds.data = Array(MAX_POINTS).fill(null); });
    massChart.update();

    numberChart.data.labels = Array(MAX_POINTS).fill('');
    numberChart.data.datasets.forEach(function(ds) { ds.data = Array(MAX_POINTS).fill(null); });
    numberChart.update();

    function pushToChart(chart, datasetIndex, value) {
      if (datasetIndex === 0) {
        chart.data.labels.push(new Date().toLocaleTimeString());
        if (chart.data.labels.length > MAX_POINTS) {
          chart.data.labels.shift();
        }
      }
      chart.data.datasets[datasetIndex].data.push(value);
      if (chart.data.datasets[datasetIndex].data.length > MAX_POINTS) {
        chart.data.datasets[datasetIndex].data.shift();
      }
      chart.update();
    }

    source.addEventListener('open', function(e) {
      console.log("Events Connected");
    }, false);

    source.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    }, false);

    source.addEventListener('message', function(e) {
      console.log("message", e.data);
    }, false);

    source.addEventListener('temperature', function(e) {
      console.log("temperature", e.data);
      document.getElementById("temp").innerHTML = e.data;
    }, false);

    source.addEventListener('humidity', function(e) {
      console.log("humidity", e.data);
      document.getElementById("hum").innerHTML = e.data;
    }, false);

    source.addEventListener('pressure', function(e) {
      console.log("pressure", e.data);
      document.getElementById("pres").innerHTML = e.data;
    }, false);

    source.addEventListener('gas', function(e) {
      console.log("gas", e.data);
      pushToChart(combinedChart, 0, parseFloat(e.data));
    }, false);

    source.addEventListener('voc', function(e) {
      console.log("voc", e.data);
      pushToChart(combinedChart, 1, parseFloat(e.data));
    }, false);

    source.addEventListener('particle', function(e) {
      console.log("particle", e.data);
      document.getElementById("particle").innerHTML = e.data;
    }, false);

    source.addEventListener('mc1p0', function(e) {
      pushToChart(massChart, 0, parseFloat(e.data));
    }, false);

    source.addEventListener('mc2p5', function(e) {
      pushToChart(massChart, 1, parseFloat(e.data));
    }, false);

    source.addEventListener('mc4p0', function(e) {
      pushToChart(massChart, 2, parseFloat(e.data));
    }, false);

    source.addEventListener('mc10p0', function(e) {
      pushToChart(massChart, 3, parseFloat(e.data));
    }, false);

    source.addEventListener('nc0p5', function(e) {
      pushToChart(numberChart, 0, parseFloat(e.data));
    }, false);

    source.addEventListener('nc1p0', function(e) {
      pushToChart(numberChart, 1, parseFloat(e.data));
    }, false);

    source.addEventListener('nc2p5', function(e) {
      pushToChart(numberChart, 2, parseFloat(e.data));
    }, false);

    source.addEventListener('nc4p0', function(e) {
      pushToChart(numberChart, 3, parseFloat(e.data));
    }, false);

    source.addEventListener('nc10p0', function(e) {
      pushToChart(numberChart, 4, parseFloat(e.data));
    }, false);
  }
  </script>
</body>
</html>)rawliteral";

#endif
