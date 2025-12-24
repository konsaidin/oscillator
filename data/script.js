var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var chart;

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    document.getElementById('connection-status').innerText = 'Connected';
    document.getElementById('connection-status').className = 'status connected';
}

function onClose(event) {
    console.log('Connection closed');
    document.getElementById('connection-status').innerText = 'Disconnected';
    document.getElementById('connection-status').className = 'status disconnected';
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    var data = JSON.parse(event.data);
    updateDashboard(data);
    updateChart(data);
}

function updateDashboard(data) {
    // Update Phase Voltages
    document.getElementById('val-a').innerText = data.phaseA.voltage.toFixed(1);
    document.getElementById('val-b').innerText = data.phaseB.voltage.toFixed(1);
    document.getElementById('val-c').innerText = data.phaseC.voltage.toFixed(1);

    // Update Bars (assuming 250V max for bar)
    document.getElementById('bar-a').style.width = (data.phaseA.voltage / 250 * 100) + '%';
    document.getElementById('bar-b').style.width = (data.phaseB.voltage / 250 * 100) + '%';
    document.getElementById('bar-c').style.width = (data.phaseC.voltage / 250 * 100) + '%';

    // Update Line Voltages
    document.getElementById('val-ab').innerText = data.line.AB.toFixed(1);
    document.getElementById('val-bc').innerText = data.line.BC.toFixed(1);
    document.getElementById('val-ca').innerText = data.line.CA.toFixed(1);

    // Update Quality
    document.getElementById('val-unbalance').innerText = data.unbalance.toFixed(2);
    document.getElementById('val-freq').innerText = data.frequency.toFixed(1);

    // Alerts
    const alertsList = document.getElementById('alerts-list');
    alertsList.innerHTML = '';
    let hasAlerts = false;

    if (data.alerts.phaseLoss) {
        let li = document.createElement('li');
        li.className = 'alert-item';
        li.innerText = '⚠️ PHASE LOSS DETECTED';
        alertsList.appendChild(li);
        hasAlerts = true;
    }
    if (data.alerts.unbalance) {
        let li = document.createElement('li');
        li.className = 'alert-item';
        li.innerText = '⚠️ HIGH VOLTAGE UNBALANCE';
        alertsList.appendChild(li);
        hasAlerts = true;
    }

    if (!hasAlerts) {
        alertsList.innerHTML = '<li>System OK</li>';
    }
}

// Chart.js Setup
const ctx = document.getElementById('voltageChart').getContext('2d');
const maxDataPoints = 50;

const chartConfig = {
    type: 'line',
    data: {
        labels: Array(maxDataPoints).fill(''),
        datasets: [
            { label: 'Phase A', data: [], borderColor: '#ff4444', tension: 0.4 },
            { label: 'Phase B', data: [], borderColor: '#44ff44', tension: 0.4 },
            { label: 'Phase C', data: [], borderColor: '#4444ff', tension: 0.4 }
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        scales: {
            y: {
                beginAtZero: false,
                suggestedMin: 180,
                suggestedMax: 260
            }
        }
    }
};

chart = new Chart(ctx, chartConfig);

function updateChart(data) {
    if (chart.data.datasets[0].data.length > maxDataPoints) {
        chart.data.datasets[0].data.shift();
        chart.data.datasets[1].data.shift();
        chart.data.datasets[2].data.shift();
    }

    chart.data.datasets[0].data.push(data.phaseA.voltage);
    chart.data.datasets[1].data.push(data.phaseB.voltage);
    chart.data.datasets[2].data.push(data.phaseC.voltage);
    
    chart.update();
}

window.addEventListener('load', initWebSocket);
