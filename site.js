const TANK_EMPTY = 70;
const TANK_FULL = 10;

var sensorData = [];
var latestData = {};

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

var sensorDataRef = database.ref('sensor-data').orderByChild('timestamp').limitToLast(500);
sensorDataRef.on('child_changed', function (snap) {
    if (!initFetch) return;
    latestData = convertToChartData(snap);
    if (latestData != null) {
        sensorData.push(latestData);
        updateChart();
    }
});
sensorDataRef.once('value', function (snapshot) {
    initFetch = true;
    sensorData = [];
    snapshot.forEach(function (item) {
        latestData = convertToChartData(item);
        if (latestData != null)
            sensorData.push(latestData);
    });
    updateChart();
});

function convertToChartData(snapshot) {
    var itemVal = snapshot.val();
    // console.log(itemVal);
    if (!itemVal || isNaN(itemVal.timestamp) || itemVal.timestamp <= 0 || itemVal.timestamp == "")
        return null;

    return {
        timestamp: new Date(itemVal.timestamp),
        value: itemVal.value
    };
}