const TANK_EMPTY = 100;
const TANK_FULL = 0;

var sensorData = [];
var latestData = {};

Chart.plugins.register({
	afterDraw: function(chart) {
		if (chart.data.datasets[0].data.length === 0) {
			// No data is present
			var ctx = chart.chart.ctx;
			var width = chart.chart.width;
			var height = chart.chart.height;
			chart.clear();

			ctx.save();
			ctx.textAlign = 'center';
			ctx.textBaseline = 'middle';
			ctx.font = "16px normal 'Helvetica Nueue'";
			// chart.options.title.text <=== gets title from chart 
			// width / 2 <=== centers title on canvas 
			// 18 <=== aligns text 18 pixels from top, just like Chart.js 
			ctx.fillText(chart.options.title.text, width / 2, 18); // <====   ADDS TITLE
			ctx.fillText('No data to display. Fetching data...', width / 2, height / 2);
			ctx.restore();
		}
	}
});

var tank = document.getElementById('tankChart').getContext('2d');
var tankChart = new Chart(tank, {
    // The type of chart we want to create
    type: 'bar',
    // The data for our dataset
    data: {
        labels: ['Water'],
        datasets: [{
            label: 'Water level',
            backgroundColor: 'rgb(179, 217, 255)',
            borderColor: 'rgb(9, 45, 93)',
            borderWidth: 2,
            data: []
        }]
    },

    options: {
        scales: {
            yAxes: [{
                ticks: {
                    max: TANK_EMPTY,
                    min: TANK_FULL,
                    callback: function (value) {
                        if (value == TANK_FULL)
                            return 'Empty';
                        if (value == TANK_EMPTY)
                            return 'Full';
                        return '';
                    }
                }
            }],
        }
    }
});

var ctx = document.getElementById('myChart').getContext('2d');
var chart = new Chart(ctx, {
    // The type of chart we want to create
    type: 'line',
    // The data for our dataset
    data: {
        labels: [],
        datasets: [{
            label: 'Water usage',
            backgroundColor: 'rgb(179, 217, 255)',
            borderColor: 'rgb(9, 45, 93)',
            data: []
        }]
    },

    options: {
        scales: {
            yAxes: [{
                ticks: {
                    max: TANK_EMPTY,
                    min: TANK_FULL,
                    callback: function (value) {
                        return (value / TANK_EMPTY * 100).toFixed(0) + '%';
                    }
                }
            }],
            xAxes: [{
                type: 'time'
            }]
        },
        elements: {
            point: {
                radius: 0
            }
        }
    }
});



function updateChart() {

    chart.data.labels = $.map(sensorData, function (n, i) { return n.timestamp });
    chart.data.datasets[0].data = $.map(sensorData, function (n, i) { return TANK_EMPTY - n.value });
    chart.update();

    tankChart.data.labels = ['Water level as of: ' + new Date(latestData.timestamp).toLocaleTimeString()];
    tankChart.data.datasets[0].data = [TANK_EMPTY - latestData.value];
    tankChart.update();
}

var firebaseConfig = {
    apiKey: "yA7qLGEgAdrgdKIKDAGcOqZQpumkn8IoAAgMtPi7",
    // authDomain: "projectId.firebaseapp.com",
    databaseURL: "https://rrc-ha.firebaseio.com",
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
var database = firebase.database();
var initFetch = false;

// window.db = database;
// var test = database.ref('sensor-data').orderByChild('timestamp').startAt(1599136200000).endAt(1599138000000);
// test.once('value', function (snapshot) {
//     snapshot.forEach(function (item) {
// console.log(item.key, item.val());
// // database.ref('sensor-data/'+item.key).remove();
//     });
// });

var sensorDataRef = database.ref('sensor-data').orderByChild('timestamp').limitToLast(1000);
sensorDataRef.on('child_changed', function (snap) {
    if (!initFetch) return;
    var val = convertToChartData(snap);
    if (val != null) {
        sensorData.push(val);
        latestData = val;
        updateChart();
    }
});
sensorDataRef.once('value', function (snapshot) {
    initFetch = true;
    sensorData = [];
    snapshot.forEach(function (item) {
        var val = convertToChartData(item);
        if (val != null)
            sensorData.push(val);
    });
    if (sensorData.length > 0) {
        latestData = sensorData[sensorData.length - 1];
        updateChart();
    }
});

function convertToChartData(snapshot) {
    var itemVal = snapshot.val();
    // console.log(itemVal);
    if (!itemVal || isNaN(itemVal.timestamp) || itemVal.timestamp <= 0 || itemVal.timestamp == "")
        return null;

    return {
        timestamp: new Date(itemVal.timestamp),
        value: map(itemVal.value, 4, 65, 0, 100)
    };
}

function map(x, in_min, in_max, out_min, out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}